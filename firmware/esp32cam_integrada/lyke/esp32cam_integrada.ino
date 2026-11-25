#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <esp_ota_ops.h>
#include <esp_app_desc.h>
#include <esp_image_format.h>
#include <mbedtls/sha256.h>
#include "camera_bench.h"
#include "photo_queue.h"
#include "api_contract.h"
#include "vision_body.h"
#include "answer_pager.h"
#include "answer_json.h"
#include "answer_frame.h"
#include <esp_heap_caps.h>
#include <esp_system.h>
#include "power_policy.h"

struct AnswerAllocator:ArduinoJson::Allocator {
  void* allocate(size_t n) override{return heap_caps_malloc(n,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);}
  void* reallocate(void *p,size_t n) override{return heap_caps_realloc(p,n,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);}
  void deallocate(void *p) override{free(p);}
};
static AnswerAllocator answerAllocator;
static JsonDocument answerJson(&answerAllocator);
static lyke::AnswerDocument answerDocument;

static constexpr char projectName[] = "esp32cam_integrada";
static constexpr char apName[] = "Calc-Lyke";
static constexpr char defaultApiUrl[] = "https://vxjmoctaomihrmxmuxip.supabase.co/functions/v1/lyke-solve";
static constexpr unsigned MaxPhotos = 3, MaxNetworks = 8;
static const IPAddress apAddress(192, 168, 8, 1);
static WebServer web(80);
static HardwareSerial stmSerial(2);
static Preferences config;
static PhotoQueue photos;
static bool portal, otaMode, scanActive, connectActive, storageReady;
// OFF lasts until the next ESP boot. The portal now has an inactivity lease;
// router Wi-Fi remains in STA mode when no configuration portal is needed.
static bool wifiEnabled = true;
static PortalLease portalLease;
static bool captureBusy;
static uint32_t capturePendingAt;
static String captureResult;
static bool connectionFailed;
static uint32_t rebootAt, connectAt, portalAttemptAt;
static String sessionToken, activeSsid, lastAnswer, pendingRequestId;
static String scanned[MaxNetworks];
static unsigned scannedCount;
static unsigned mappedKeys;
static bool keyboardReady;
static String stmRevision;
static wl_status_t lastWifi = WL_IDLE_STATUS;

static bool local() {
  bool ok=portal && web.client().localIP() == WiFi.softAPIP();
  if(ok)portalLease.touch(millis());
  return ok;
}

static bool authorized() {
  return local() && sessionToken.length() && web.header("X-Update-Token") == sessionToken;
}

static void reply(int code, const String& message) {
  web.sendHeader("Cache-Control", "no-store");
  web.send(code, "text/plain; charset=utf-8", message);
}

#include "ota_receiver.h"
#include "stm_update.h"

static void sendLine(const String& text) {
  stmSerial.println(text);
}

static String printable(const String& text, size_t limit) {
  String out;
  for (size_t i = 0; i < text.length() && out.length() < limit; ++i) {
    const unsigned char c = text[i];
    if (c >= 32 && c < 127) out += char(c);
    else if (c < 128 || (c & 0xc0) == 0xc0) out += '?';
  }
  return out;
}

static void sendStatus() {
  stmSerial.printf("STATE %u %u %u %u\n", WiFi.status() == WL_CONNECTED, photos.count,
              portal, !lastAnswer.isEmpty());
  sendLine(String("LINK ")+(!wifiEnabled?"OFF":connectActive?"JOIN":WiFi.status()==WL_CONNECTED?"OK":connectionFailed?"FAIL":"IDLE"));
  sendLine("CAPS G1");
}

static uint32_t answerTextCRC(){return ~image_crc_update(UINT32_MAX,(const uint8_t*)lastAnswer.c_str(),lastAnswer.length());}

static void setAnswerDocument(JsonVariantConst value) {
  answerJson.clear();
  if(value.is<JsonObjectConst>()&&answerJson.set(value)&&loadAnswerDocument(answerJson.as<JsonVariantConst>(),answerDocument))return;
  answerJson.clear();answerDocument.plain(lastAnswer.c_str());
}

static void saveAnswerDrawing() {
  if(!storageReady)return;
  if(answerJson.isNull()){LittleFS.remove("/lyke-drawing.json");return;}
  answerJson["_text_crc32"]=answerTextCRC();
  File file=LittleFS.open("/lyke-drawing.tmp","w");
  if(!file)return;
  size_t expected=measureJson(answerJson),n=serializeJson(answerJson,file);file.close();
  if(n==expected)LittleFS.rename("/lyke-drawing.tmp","/lyke-drawing.json");
  else {LittleFS.remove("/lyke-drawing.tmp");LittleFS.remove("/lyke-drawing.json");}
}

static void sendAnswerWindow(unsigned x,unsigned y,unsigned sequence) {
  if(sequence>65535||x>65535||y>65535)return;
  if(!answerDocument.count)answerDocument.plain(lastAnswer.c_str());
  if(y>answerDocument.height-48)y=answerDocument.height-48;
  unsigned width=answerDocument.widthAt(y);if(x>width-126)x=width-126;
  static lyke::AnswerRaster raster;raster.render(answerDocument,x,y);
  stmSerial.printf("FRAME %u %u %u %u %u %u\n",sequence,x,y,width,answerDocument.height,frame_checksum(&raster.pixels[0][0],768));
  static const char hex[]="0123456789abcdef";
  for(unsigned row=0;row<48;++row) {
    char bytes[33];for(unsigned i=0;i<16;++i){unsigned b=raster.pixels[row][i];bytes[2*i]=hex[b>>4];bytes[2*i+1]=hex[b&15];}bytes[32]=0;
    stmSerial.printf("PIX %u %u %s\n",sequence,row,bytes);
  }
  stmSerial.printf("FRAME_END %u\n",sequence);
}

static bool startPortal() {
  wifiEnabled = true;
  portalLease.touch(millis());
  if (portal && (WiFi.getMode() & WIFI_AP) && WiFi.softAPIP() == apAddress) return true;
  portalAttemptAt = millis();
  if (portal) web.stop();
  portal = false;
  if (!WiFi.mode(WIFI_AP_STA) || !WiFi.softAPConfig(apAddress, apAddress, IPAddress(255,255,255,0)) ||
      !WiFi.softAP(apName, "CHANGE_ME_ESP_AP", 1, false, 2)) return false;
  if(sessionToken.isEmpty())for (unsigned i = 0; i < 4; ++i) sessionToken += String(esp_random(), HEX);
  web.begin();
  portal = true;
  return true;
}

static void cameraStop() {
  if (ready) {
    sensor_t* s = esp_camera_sensor_get();
    if (s && s->id.PID == 0x5640) s->set_reg(s, 0x3008, 0x40, 0x40);
    esp_camera_deinit();
  }
  ready = afReady = false;
  pinMode(32, OUTPUT);
  digitalWrite(32, HIGH);
  pinMode(0, OUTPUT);
  digitalWrite(0, LOW);
}

static bool join(unsigned index);
static String ssidAt(unsigned index);
struct CpuBurst {
  CpuBurst(){setCpuFrequencyMhz(160);}
  ~CpuBurst(){setCpuFrequencyMhz(80);}
};
struct CapturePause {
  bool enabled,restorePortal;
  unsigned network;
  CapturePause():enabled(wifiEnabled),restorePortal(portalLease.wanted),network(config.getUChar("last_net",0)) {
    for(unsigned i=0;i<MaxNetworks;++i)if(ssidAt(i)==activeSsid){network=i;break;}
    captureBusy=true;scanActive=connectActive=false;
    if(portal)web.stop();
    portal=false;
    WiFi.mode(WIFI_OFF);
    // Let the radio supply settle before sensor clocks/autofocus start.
    delay(80);
  }
  ~CapturePause(){
    cameraStop();
    // Restore only after camera power-down, including every error return.
    delay(30);captureBusy=false;
    if(enabled){
      if(restorePortal)startPortal();
      else {WiFi.mode(WIFI_STA);WiFi.setSleep(true);}
      join(network);
    }
    lastWifi=WiFi.status();
  }
};

static String takePhoto() {
  if (!keyboardReady) return "Conclua o mapeamento do teclado";
  if (photos.count >= MaxPhotos) return "Fila cheia (3 fotos)";
  if (otaMode) return "Saia da atualizacao";
  if(scanActive)return "Aguarde a busca Wi-Fi";
  if (ESP.getFreePsram() < 1024 * 1024) return "PSRAM insuficiente";
  sendLine("WORK FOTO");
  CpuBurst cpu;
  CapturePause radio;
  cameraBenchSetup();
  if (!ready) {
    cameraStop();
    return "Camera: " + cameraInitError;
  }
  sensor_t* sensor = esp_camera_sensor_get();
  if (config.isKey("mirror")) sensor->set_hmirror(sensor, config.getBool("mirror"));
  if (config.isKey("flip")) sensor->set_vflip(sensor, config.getBool("flip"));
  for (uint32_t begin = millis(); millis() - begin < 380;) {
    regulate();
    delay(20);
  }
  if (afMode && config.getBool("focus", true) && sensor->id.PID == 0x5640) {
    focusOnce();
    if (!afFocusConfirmed) {
      cameraStop();
      return "Foco nao confirmado";
    }
  }
  discardFrames(2);
  bool saved = false;
  for (unsigned attempt = 0; attempt < 3; ++attempt) {
    if (attempt == 1) {
      sensor->set_quality(sensor, max(quality, 16));
      discardFrames(2);
    } else if (attempt == 2) {
      sensor->set_framesize(sensor, FRAMESIZE_UXGA);
      sensor->set_quality(sensor, max(quality, 19));
      discardFrames(2);
    }
    camera_fb_t* frame = esp_camera_fb_get();
    if (!frame) break;
    bool oversized = frame->len > PhotoQueue::ramBudget / MaxPhotos;
    if (!oversized && frame->format == PIXFORMAT_JPEG) saved = photos.save(frame->buf, frame->len);
    esp_camera_fb_return(frame);
    if (!oversized) break;
  }
  cameraStop();
  if (saved) pendingRequestId = "";
  return saved ? String() : photos.error.isEmpty() ? "Falha JPEG" : photos.error;
}

static String netKey(const char* prefix, unsigned index) {
  return String(prefix) + String(index);
}

static String ssidAt(unsigned index) {
  return config.getString(netKey("ssid", index).c_str(), "");
}

static bool storeNetwork(const String& ssid, const String& password, unsigned& index) {
  if (ssid.isEmpty() || ssid.length() > 32 ||
      (password.length() && (password.length() < 8 || password.length() > 63))) return false;
  index = MaxNetworks;
  for (unsigned i = 0; i < MaxNetworks; ++i) {
    String name = ssidAt(i);
    if (name == ssid) { index = i; break; }
    if (name.isEmpty() && index == MaxNetworks) index = i;
  }
  if (index == MaxNetworks) return false;
  String passKey = netKey("pass", index), ssidKey = netKey("ssid", index);
  config.putString(passKey.c_str(), password);
  if (config.getString(passKey.c_str(), "") != password) return false;
  return config.putString(ssidKey.c_str(), ssid) == ssid.length();
}

static bool join(unsigned index) {
  if (index >= MaxNetworks || scanActive || otaMode) return false;
  const String ssid = ssidAt(index);
  if (ssid.isEmpty()) return false;
  if(!wifiEnabled)return false;
  if(!portal&&!WiFi.mode(WIFI_STA))return false;
  WiFi.setSleep(true);
  WiFi.disconnect(false, false);
  String password = config.getString(netKey("pass", index).c_str(), "");
  WiFi.begin(ssid.c_str(), password.isEmpty() ? nullptr : password.c_str());
  activeSsid = ssid;
  connectAt = millis();
  connectActive = true;
  connectionFailed=false;
  sendLine("NOTE CONECTANDO");
  sendStatus();
  return true;
}

static bool startScan() {
  if (scanActive || connectActive || otaMode) return false;
  if (!startPortal()) return false;
  WiFi.scanDelete();
  scannedCount = 0;
  scanActive = WiFi.scanNetworks(true, false, false, 250) != WIFI_SCAN_FAILED;
  return scanActive;
}

static void wifiTick() {
  // No idle timeout. Retry a failed/lost AP, unless the STM requested OFF.
  if (wifiEnabled && portalLease.wanted && (!portal || !(WiFi.getMode() & WIFI_AP) || WiFi.softAPIP() != apAddress) &&
      millis() - portalAttemptAt >= 3000) {
    if (startPortal()) sendStatus();
  }
  if(portalLease.expired(millis(),otaMode||ota.receiving||captureBusy||capturePendingAt||scanActive||connectActive)) {
    web.stop();portal=false;portalLease.close();
    WiFi.softAPdisconnect(false);WiFi.mode(WIFI_STA);WiFi.setSleep(true);
    sendStatus();
  }
  wl_status_t status = WiFi.status();
  if (status != lastWifi) {
    if(lastWifi==WL_CONNECTED&&status!=WL_CONNECTED&&wifiEnabled&&!connectActive)connectionFailed=true;
    lastWifi = status;
    sendStatus();
  }
  if (connectActive && (status == WL_CONNECTED || millis() - connectAt > 22000)) {
    connectActive = false;
    if (status == WL_CONNECTED && WiFi.SSID() == activeSsid) {
      connectionFailed=false;
      for (unsigned i = 0; i < MaxNetworks; ++i)
        if (ssidAt(i) == activeSsid) { config.putUChar("last_net", i); break; }
      configTime(0, 0, "pool.ntp.org", "time.nist.gov");
      sendLine("NOTE WIFI CONECTADO");
    } else {
      connectionFailed=true;
      WiFi.disconnect(false, false);
      sendLine("NOTE FALHA NO WIFI");
    }
    sendStatus();
  }
  if (!scanActive) return;
  int found = WiFi.scanComplete();
  if (found == WIFI_SCAN_RUNNING) return;
  scanActive = false;
  for (int i = 0; i < found && scannedCount < MaxNetworks; ++i) {
    String ssid = WiFi.SSID(i);
    bool duplicate = false;
    for (unsigned j = 0; j < scannedCount; ++j) duplicate |= scanned[j] == ssid;
    if (!duplicate && ssid.length()) scanned[scannedCount++] = ssid;
  }
  WiFi.scanDelete();
  sendLine("SCAN " + String(scannedCount));
  for (unsigned i = 0; i < scannedCount; ++i)
    sendLine("NET " + String(i) + " " + printable(scanned[i], 32));
}

class BodyStream : public Stream {
public:
  calc::VisionBody data;
  size_t write(uint8_t) override { return 0; }
  int available() override { return data.remaining(); }
  int read() override { return data.read(); }
  int peek() override { return -1; }
  void flush() override {}
};

class BoundedResponse : public Stream {
public:
  static constexpr size_t Limit = 64 * 1024;
  char* text = static_cast<char*>(heap_caps_malloc(Limit, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  size_t length = 0;
  ~BoundedResponse() { free(text); }
  size_t write(uint8_t byte) override { return write(&byte, 1); }
  size_t write(const uint8_t* p, size_t n) override {
    if (!text || n > Limit - length) return 0;
    memcpy(text + length, p, n);
    length += n;
    return n;
  }
  int available() override { return 0; }
  int read() override { return -1; }
  int peek() override { return -1; }
  void flush() override {}
};

extern const uint8_t certStart[] asm("_binary_x509_crt_bundle_start");
extern const uint8_t certEnd[] asm("_binary_x509_crt_bundle_end");

static String askAI() {
  if (otaMode || !photos.count) return "Tire 1 a 3 fotos";
  if (WiFi.status() != WL_CONNECTED) return "Conecte o Wi-Fi";
  if (time(nullptr) < 1735689600) return "Aguarde ajuste da hora";
  CpuBurst cpu;
  if (ESP.getFreePsram() < 512 * 1024 ||
      heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT) < 60 * 1024 ||
      heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT) < 32 * 1024)
    return "Memoria insuficiente para HTTPS";
  String key = config.getString("api", "");
  if (key.isEmpty()) return "Configure acesso no celular";
  const String endpoint = config.getString("api_url", defaultApiUrl);
  JsonDocument request;
  const bool economy = config.getBool("economy", false);
  calc::makeApiRequest(request, economy, photos.count);
  request["model"] = economy ? "gpt-6-luna" : "gpt-6.1-terra";
  request["max_output_tokens"] = 10000;
  request["display"] = "lyke-graphics-v1";
  request["reasoning"]["effort"] = economy ? "medium" : "high";
  BodyStream stream;
  {
    String json;
    serializeJson(request, json);
    unsigned cursor = 0;
    for (unsigned i = 0; i < photos.count; ++i) {
      const String marker = "\"__PHOTO_" + String(i) + "__\"";
      int at = json.indexOf(marker, cursor);
      if (at < 0 || !photos.entries[i].ram) return "Falha preparando o JPEG";
      stream.data.text((json.substring(cursor, at) + "\"data:image/jpeg;base64,").c_str());
      stream.data.image(photos.entries[i].ram, photos.entries[i].bytes);
      stream.data.text("\"");
      cursor = at + marker.length();
    }
    stream.data.text(json.substring(cursor).c_str());
    request.clear();
  }
  NetworkClientSecure tls;
  tls.setCACertBundle(certStart, certEnd - certStart);
  tls.setHandshakeTimeout(16);
  HTTPClient http;
  http.setConnectTimeout(12000);
  // HTTPClient takes uint16_t milliseconds in Arduino ESP32 3.3.6.
  // Keep the request ID on a timeout so a retry can recover the cached answer.
  http.setTimeout(65000);
  if (!http.begin(tls, endpoint)) return "Falha HTTPS";
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + key);
  if (pendingRequestId.isEmpty()) {
    char id[33];
    snprintf(id, sizeof(id), "%08lx%08lx%08lx%08lx", (unsigned long)esp_random(),
             (unsigned long)esp_random(), (unsigned long)esp_random(), (unsigned long)esp_random());
    pendingRequestId = id;
  }
  http.addHeader("X-Lyke-Request-ID", pendingRequestId);
  key = "";
  sendLine("WORK IA");
  int status = http.sendRequest("POST", &stream, stream.data.length());
  BoundedResponse response;
  bool received = status > 0 && response.text && http.writeToStream(&response) >= 0;
  http.end();
  if (status >= 400 && status != 409) pendingRequestId = "";
  if (!received) return "Falha HTTPS ou resposta grande";
  JsonDocument parsed(&answerAllocator);
  JsonDocument filter;
  filter["status"] = true;
  filter["error"] = true;
  filter["output"][0]["type"] = true;
  filter["output"][0]["content"][0] = true;
  filter["usage"] = true;
  filter["archive"]["request_id"] = true;
  filter["lyke_document"] = true;
  if (deserializeJson(parsed, response.text, response.length, DeserializationOption::Filter(filter)))
    return "Falha lendo resposta da IA";
  auto answer = calc::parseApiResponse(parsed, status);
  if (!answer.ok) return String(answer.error.c_str());
  lastAnswer = answer.text.c_str();
  setAnswerDocument(parsed["lyke_document"]);
  saveAnswerDrawing();
  const String archiveId = parsed["archive"]["request_id"] | "";
  if (archiveId.length()) config.putString("last_req", archiveId);
  if (storageReady) {
    File file = LittleFS.open("/lyke-response.tmp", "w");
    if (file) {
      size_t n = file.print(lastAnswer);
      file.close();
      if (n == lastAnswer.length()) LittleFS.rename("/lyke-response.tmp", "/lyke-response.txt");
      else LittleFS.remove("/lyke-response.tmp");
    }
  }
  photos.clear();
  pendingRequestId = "";
  return String();
}

static void sendAnswerPage(unsigned page, unsigned columns, unsigned rows, bool compact = false) {
  char output[6][22];
  bool more = answer_page_rows(lastAnswer.c_str(), lastAnswer.length(), page, columns, rows, compact, output);
  for (unsigned i = 0; i < rows; ++i)
    sendLine("TEXT " + String(i) + " " + printable(output[i], columns));
  sendLine(more ? "TEXT_MORE" : "TEXT_END");
}

static void runCommand(const String& command) {
  if (command == "STATUS" || command == "LYKE APP V1") {
    sendStatus();
  } else if(command.startsWith("READG ")) {
    unsigned x,y,sequence;char extra;
    if(sscanf(command.c_str(),"READG %u %u %u %c",&x,&y,&sequence,&extra)==3)sendAnswerWindow(x,y,sequence);
  } else if (command.startsWith("STM32 ")) {
    stmRevision = printable(command.substring(6), 31);
  } else if (command.startsWith("MAP ")) {
    mappedKeys = constrain(command.substring(4).toInt(), 0L, 50L);
    int separator = command.indexOf(' ', 4);
    keyboardReady = separator < 0 ? mappedKeys == 50 : command.substring(separator + 1) == "1";
  } else if (command == "PORTAL") {
    sendLine(startPortal() ? "PORTAL OK" : "NOTE FALHA NO AP");
    sendStatus();
  } else if (command == "SCAN") {
    if (!startScan()) sendLine("SCAN 0");
  } else if (command == "LIST") {
    for (unsigned i = 0; i < MaxNetworks; ++i) {
      String name = ssidAt(i);
      if (name.length()) sendLine("SAVED " + String(i) + " " + printable(name, 32));
    }
    sendLine("LIST_END");
  } else if (command.startsWith("JOIN ") && command.length() == 6 && isDigit(command[5])) {
    wifiEnabled=true;
    if (!join(command[5] - '0')) sendLine("NOTE REDE INDISPONIVEL");
  } else if (command == "CAPTURE") {
    String error = takePhoto();
    sendLine(error.isEmpty() ? "NOTE FOTO GUARDADA" : "NOTE " + error);
    sendStatus();
  } else if (command == "SEND") {
    String error = askAI();
    sendStatus();
    sendLine(error.isEmpty() ? "ANSWER OK" : "NOTE " + printable(error, 120));
  } else if (command == "CLEAR") {
    photos.clear();
    pendingRequestId = "";
    sendStatus();
  } else if (command == "REMOVE") {
    pendingRequestId = "";
    if (photos.count) {
      auto& photo = photos.entries[--photos.count];
      free(photo.ram);
      photo = {};
    }
    sendStatus();
  } else if (command.startsWith("READ6 ")) {
    sendAnswerPage(constrain(command.substring(6).toInt(), 0L, 999L), 21, 6, true);
  } else if (command.startsWith("READ ") || command.startsWith("READ4 ")) {
    const bool wide = command.startsWith("READ4 ");
    sendAnswerPage(constrain(command.substring(wide ? 6 : 5).toInt(), 0L, 999L),
                   wide ? 21 : 18, wide ? 4 : 3);
  } else if (command == "OFF") {
    if (ota.receiving || ota.complete || rebootAt) { sendLine("NOTE ATUALIZACAO ATIVA"); return; }
    wifiEnabled = false;
    portalLease.close();
    connectionFailed=false;
    otaMode = false;
    scanActive = connectActive = false;
    web.stop();
    WiFi.mode(WIFI_OFF);
    portal = false;
    sessionToken = "";
    sendStatus();
  }
}

#include "portal.h"

void setup() {
  Serial.begin(460800);
  setCpuFrequencyMhz(80);
  pinMode(33, INPUT);
  stmSerial.setRxBufferSize(512);
  stmSerial.begin(115200, SERIAL_8N1, 13, 14);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  config.begin("lyke", false);
  storageReady = LittleFS.begin(false, "/littlefs", 6, "spiffs");
  if (storageReady) {
    File file = LittleFS.open("/lyke-response.txt", "r");
    if (file && file.size() <= 13000) lastAnswer = file.readString();
    File drawing=LittleFS.open("/lyke-drawing.json","r");
    if(drawing&&drawing.size()<=32768&&!deserializeJson(answerJson,drawing)&&
       answerJson["_text_crc32"].is<uint32_t>()&&answerJson["_text_crc32"].as<uint32_t>()==answerTextCRC()&&
       loadAnswerDocument(answerJson.as<JsonVariantConst>(),answerDocument)){}
    else {answerJson.clear();answerDocument.plain(lastAnswer.c_str());}
  }
  if(!answerDocument.count)answerDocument.plain(lastAnswer.c_str());
  cameraStop();
  routes();
  startPortal();
  // Preserve the update AP while joining the last successful saved network.
  join(config.getUChar("last_net", 0));
  stmSerial.print("LYKE-HELLO\n");
  sendStatus();
}

void loop() {
  if (portal) web.handleClient();
  wifiTick();
  if(capturePendingAt&&int32_t(millis()-capturePendingAt)>=0) {
    capturePendingAt=0;captureResult=takePhoto();
    sendLine(captureResult.isEmpty()?"NOTE FOTO GUARDADA":"NOTE "+captureResult);
    sendStatus();
  }
  static String input;
  static bool tooLong;
  for (unsigned i = 0; i < 256 && stmSerial.available(); ++i) {
    int c = stmSerial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      if (!tooLong) runCommand(input);
      input = "";
      tooLong = false;
    } else if (input.length() < 96) input += char(c);
    else tooLong = true;
  }
  // Keep a recovery path on the already connected bench USB for the ESP itself.
  static String console;
  while (Serial.available()) {
    int c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      if (console == "OTA" || console == "WIFI") {
        bool ok = startPortal();
        otaMode = ok && console == "OTA";
        Serial.println(ok ? "OK Calc-Lyke http://192.168.8.1" : "ERR AP");
      } else if (console == "STATUS") {
        JsonDocument state;
        describeImage(state);
        Serial.print("INFO ");
        serializeJson(state, Serial);
        Serial.println();
      } else if (console == "OFF") {
        otaMode = false;
        runCommand("OFF");
        Serial.println("OK Wi-Fi off");
      }
      console = "";
    } else if (console.length() < 64) console += char(c);
  }
  if (rebootAt && (int32_t)(millis() - rebootAt) >= 0) ESP.restart();
  delay(10);
}

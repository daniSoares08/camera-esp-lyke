#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <NetworkClientSecure.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#include <time.h>
#include <mbedtls/base64.h>
#include <esp_heap_caps.h>
#include <memory>
#include <atomic>
#include "config_store.h"
#include "camera_service.h"
#include "api_contract.h"
#include "camera_session.h"
#include "photo_store.h"
#include "vision_body.h"

extern const uint8_t rootBundleStart[] asm("_binary_x509_crt_bundle_start");
extern const uint8_t rootBundleEnd[] asm("_binary_x509_crt_bundle_end");

namespace networkservice {
enum class JobKind:uint8_t { CONNECT, SCAN, PHOTO, SNAPSHOT, CAPTURE_SLOT, SEND_BATCH };
struct Job {JobKind kind;char ssid[33]={},password[64]={};bool economy=false,mirror=false,flip=false;};
enum class Kind:uint8_t { STATUS, CONNECTED, SCANNED, RESPONSE, ERROR, PHOTO_SAVED };
struct Update {Kind kind;char* text;};
class PsramAllocator:public ArduinoJson::Allocator {
public:
  void* allocate(size_t n)override{return heap_caps_malloc(n,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);}
  void deallocate(void* p)override{heap_caps_free(p);}
  void* reallocate(void* p,size_t n)override{return heap_caps_realloc(p,n,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);}
};
class BoundedResponse:public Stream {
public:
  uint8_t* bytes;size_t length=0,capacity;bool overflow=false;
  explicit BoundedResponse(size_t n):capacity(n){bytes=(uint8_t*)heap_caps_malloc(n+1,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);}
  ~BoundedResponse(){heap_caps_free(bytes);}
  size_t write(uint8_t b)override{return write(&b,1);}
  size_t write(const uint8_t* data,size_t n)override{if(!bytes||length+n>capacity){overflow=true;return 0;}memcpy(bytes+length,data,n);length+=n;bytes[length]=0;return n;}
  int available()override{return 0;}int read()override{return -1;}int peek()override{return -1;}void flush()override{}
};
class Service {
  QueueHandle_t jobs=nullptr,updates=nullptr;
  CameraService camera;
  PhotoStore photos;
  std::atomic<uint16_t> disconnectReason{0};
  std::atomic<bool> recordingConnection{false};
  static void entry(void* self){((Service*)self)->run();}
  void publish(Kind kind,const String& text){
    char* data=(char*)malloc(text.length()+1);if(!data)return;memcpy(data,text.c_str(),text.length()+1);Update update{kind,data};if(xQueueSend(updates,&update,pdMS_TO_TICKS(100))!=pdTRUE)free(data);
  }
  void connect(const Job& job){
    publish(Kind::STATUS,"Conectando a "+String(job.ssid)+"...");
    WiFi.mode(WiFi.getMode()==WIFI_MODE_APSTA?WIFI_AP_STA:WIFI_STA);
    // End the previous station attempt without closing the local photo AP.
    recordingConnection=false;WiFi.setAutoReconnect(false);WiFi.disconnect(false,false,1000);delay(200);
    disconnectReason=0;recordingConnection=true;WiFi.setAutoReconnect(true);WiFi.begin(job.ssid,job.password);
    uint32_t start=millis();while(WiFi.status()!=WL_CONNECTED&&millis()-start<30000)delay(100);
    recordingConnection=false;
    if(WiFi.status()!=WL_CONNECTED){uint16_t reason=disconnectReason.load();int state=WiFi.status();WiFi.setAutoReconnect(false);WiFi.disconnect(false,false);publish(Kind::ERROR,"Wi-Fi nao conectou. Estado "+String(state)+", motivo "+String(reason)+". Confira senha, sinal e rede 2.4 GHz.");return;}
    disconnectReason=0;
    Preferences p;p.begin("calcnet",false);p.putString("last_ssid",job.ssid);p.end();WiFi.setSleep(true);configTime(0,0,"pool.ntp.org","time.cloudflare.com");publish(Kind::CONNECTED,String(job.ssid));
  }
  void scan(){
    WiFi.mode(WiFi.getMode()==WIFI_MODE_APSTA?WIFI_AP_STA:WIFI_STA);WiFi.setSleep(false);delay(250);WiFi.scanDelete();
    int count=WiFi.scanNetworks(true,true,false,400);uint32_t start=millis();
    while(count==WIFI_SCAN_RUNNING&&millis()-start<30000){delay(100);count=WiFi.scanComplete();}
    if(count<0){WiFi.scanDelete();publish(Kind::ERROR,"Busca Wi-Fi falhou ou expirou. Tente novamente; somente redes 2.4 GHz.");return;}
    if(WiFi.status()==WL_CONNECTED)WiFi.setSleep(true);
    JsonDocument doc;JsonArray a=doc.to<JsonArray>();for(int i=0;i<count&&i<20;i++){String s=WiFi.SSID(i);bool duplicate=false;for(JsonVariant v:a)if(v.as<String>()==s)duplicate=true;if(!s.isEmpty()&&!duplicate)a.add(s);}WiFi.scanDelete();String raw;serializeJson(doc,raw);publish(Kind::SCANNED,raw);
  }
  void photo(const Job& job){
    bool localOnly=job.kind==JobKind::SNAPSHOT||job.kind==JobKind::CAPTURE_SLOT;
    std::vector<PhotoStore::Snapshot> images;
    if(job.kind==JobKind::SEND_BATCH){images=photos.selected();if(images.empty()){publish(Kind::ERROR,"Capture de 1 a 3 fotos com SPC antes de enviar.");return;}}
    if(job.kind==JobKind::CAPTURE_SLOT&&photos.count()>=3){publish(Kind::ERROR,"Limite de 3 fotos. Envie ou limpe pelo menu.");return;}
    String key;
    if(!localOnly){key=deviceconfig::apiKey();if(key.isEmpty()){publish(Kind::ERROR,"Configure a chave OpenAI no menu Configurar pelo celular ou pelo configurador USB.");return;}
    if(WiFi.status()!=WL_CONNECTED){publish(Kind::ERROR,"Conecte uma rede Wi-Fi pelo menu antes de enviar a foto.");return;}
    publish(Kind::STATUS,"Sincronizando relogio para HTTPS...");configTime(0,0,"pool.ntp.org","time.cloudflare.com");
    uint32_t started=millis();while(time(nullptr)<1735689600&&millis()-started<20000)delay(100);
    if(time(nullptr)<1735689600){publish(Kind::ERROR,"Relogio nao sincronizou. Confira a internet da rede e tente de novo.");return;}
    }
    if(job.kind!=JobKind::SEND_BATCH){
    publish(Kind::STATUS,"Preparando camera. Mantenha o exercicio parado e bem iluminado.");
    CameraSession<CameraService> session(camera);
    if(!camera.prepareCapture(job.mirror,job.flip)){publish(Kind::ERROR,camera.error);return;}
    camera_fb_t* fb=camera.capture();if(!fb){publish(Kind::ERROR,camera.error);return;}
    auto jpeg=photos.save(*fb);esp_camera_fb_return(fb);
    if(!jpeg.bytes){publish(Kind::ERROR,"Memoria insuficiente para guardar a foto.");return;}
    session.close(); // Stop the sensor/clock as soon as the JPEG copy exists.
    photos.setStandby(jpeg.sequence,camera.standby());
    if(job.kind==JobKind::CAPTURE_SLOT)photos.append(jpeg);
    if(localOnly){publish(Kind::PHOTO_SAVED,"Foto local pronta. Veja no navegador. Nenhum envio a API.");return;}
    images.push_back(jpeg);
    }
    JsonDocument request;calc::makeApiRequest(request,job.economy,images.size());String serialized;serializeJson(request,serialized);
    class BodyStream:public Stream {
    public:calc::VisionBody body;size_t write(uint8_t)override{return 0;}int available()override{return (int)body.remaining();}int read()override{return body.read();}int peek()override{return -1;}void flush()override{}
    } stream;
    size_t start=0;
    for(size_t i=0;i<images.size();i++){
      String marker="\"__PHOTO_"+String(i)+"__\"";int at=serialized.indexOf(marker,start);
      if(at<0){publish(Kind::ERROR,"Falha ao preparar fotos.");return;}
      String prefix=serialized.substring(start,at)+"\"data:image/jpeg;base64,";stream.body.text(prefix.c_str());stream.body.image(images[i].bytes.get(),images[i].length);stream.body.text("\"");start=at+marker.length();
    }
    stream.body.text(serialized.substring(start).c_str());
    NetworkClientSecure tls;tls.setCACertBundle(rootBundleStart,(size_t)(rootBundleEnd-rootBundleStart));tls.setHandshakeTimeout(20);
    HTTPClient http;http.setConnectTimeout(15000);http.setTimeout(65000);http.setReuse(false);
    if(!http.begin(tls,"https://api.openai.com/v1/responses")){publish(Kind::ERROR,"Nao foi possivel iniciar HTTPS.");return;}
    http.addHeader("Content-Type","application/json");http.addHeader("Authorization","Bearer "+key);
    publish(Kind::STATUS,camera.standby()?"Camera em standby. Enviando foto a OpenAI. ON permite voltar a calcular.":"Interface da camera encerrada. Enviando foto. Standby do sensor nao confirmado.");
    for(const auto& image:images)photos.markApi(image.sequence);
    int status=http.sendRequest("POST",&stream,stream.body.length());
    for(const auto& image:images)photos.markApi(image.sequence,status);key="";
    if(status<=0){String reason=HTTPClient::errorToString(status);http.end();publish(Kind::ERROR,"Falha de conexao HTTPS: "+reason+". Nenhuma repeticao automatica foi feita.");return;}
    BoundedResponse raw(192*1024);
    if(!raw.bytes||http.writeToStream(&raw)<0||raw.overflow){http.end();publish(Kind::ERROR,"Resposta grande demais ou conexao interrompida.");return;}
    http.end();PsramAllocator allocator;JsonDocument response(&allocator);
    auto jsonError=deserializeJson(response,raw.bytes,raw.length);
    if(jsonError){publish(Kind::ERROR,"Resposta da API nao pode ser lida. Confira rede, chave e saldo.");return;}
    auto answer=calc::parseApiResponse(response,status);if(!answer.ok){publish(Kind::ERROR,String(answer.error.c_str()));return;}
    double cost=(answer.inputTokens*(job.economy?0.10:2.0)+answer.outputTokens*(job.economy?0.50:10.0))/1000000.0;
    String finalText=answer.text.c_str();finalText+="\n\nUSO DA API\nEntrada: "+String(answer.inputTokens)+" tokens\nSaida: "+String(answer.outputTokens)+" tokens\nEstimativa USD "+String(cost,6)+"\nPreco de referencia: 27/10/2026. Confira o painel de uso para o valor faturado.";
    if(finalText.length()>12000)finalText=finalText.substring(0,12000);
    File f=LittleFS.open("/last_response.tmp","w");if(f){size_t count=f.print(finalText);f.close();if(count==finalText.length()){
      if(LittleFS.exists("/last_response.txt")){LittleFS.remove("/last_response.bak");LittleFS.rename("/last_response.txt","/last_response.bak");}
      if(LittleFS.rename("/last_response.tmp","/last_response.txt"))LittleFS.remove("/last_response.bak");
    }}
    if(job.kind==JobKind::SEND_BATCH)photos.clear();
    publish(Kind::RESPONSE,finalText);
  }
  volatile bool working=false;
  void run(){Job job;for(;;){if(xQueueReceive(jobs,&job,portMAX_DELAY)!=pdTRUE)continue;working=true;if(job.kind==JobKind::CONNECT)connect(job);else if(job.kind==JobKind::SCAN)scan();else photo(job);memset(job.password,0,sizeof(job.password));working=false;}}
public:
  std::vector<PhotoStore::Snapshot> selectedPhotos(){return photos.selected();}
  unsigned photoCount(){return photos.count();}
  void clearPhotos(bool lastOnly=false){photos.clear(lastOnly);}
  PhotoStore::Snapshot lastPhoto(){return photos.snapshot();}
  bool busy()const{return working||uxQueueMessagesWaiting(jobs)>0;}
  bool cameraActive()const{return camera.active();}
  bool cameraStandbyConfirmed()const{return camera.standby();}
  uint16_t wifiDisconnectReason()const{return disconnectReason.load();}
  void begin(){photos.begin();WiFi.onEvent([this](WiFiEvent_t,WiFiEventInfo_t info){if(recordingConnection.load())disconnectReason=info.wifi_sta_disconnected.reason;},ARDUINO_EVENT_WIFI_STA_DISCONNECTED);jobs=xQueueCreate(1,sizeof(Job));updates=xQueueCreate(16,sizeof(Update));xTaskCreatePinnedToCore(entry,"network",16384,this,1,nullptr,0);}
  bool enqueue(const Job& job){return xQueueSend(jobs,&job,0)==pdTRUE;}
  bool poll(Update& update){return xQueueReceive(updates,&update,0)==pdTRUE;}
};
} // namespace networkservice

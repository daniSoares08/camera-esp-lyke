#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_ota_ops.h>
#include <esp_app_desc.h>
#include <esp_image_format.h>
#include <mbedtls/sha256.h>
#include "camera_bench.h" // staged from the preserved camera r6 source
#include "photo_queue.h"
#include "board_io.h"
#include "calculator_core.h"

static constexpr char projectName[]="esp32cam_integrada";
WebServer web(80);
PhotoQueue photos;
BoardIO board;
bool portal=false,otaMode=false,benchImport=false;
String sessionToken,apName,serialLine,operation="ready";
uint32_t rebootAt=0,lastDisplay=0;

void cameraStop(){
  if(ready){sensor_t* s=esp_camera_sensor_get();if(s&&s->id.PID==0x5640)s->set_reg(s,0x3008,0x40,0x40);esp_camera_deinit();}
  ready=false;afReady=false;pinMode(32,OUTPUT);digitalWrite(32,HIGH);pinMode(0,OUTPUT);digitalWrite(0,LOW);
}
bool local(){return portal&&web.client().localIP()==WiFi.softAPIP();}
bool authorized(){return local()&&web.header("X-Update-Token")==sessionToken;}
void reply(int code,const String& text){web.sendHeader("Cache-Control","no-store");web.send(code,"text/plain; charset=utf-8",text);}
String state(){
  auto current=esp_ota_get_running_partition();auto next=esp_ota_get_next_update_partition(nullptr);
  uint8_t sha[32]={};esp_partition_get_sha256(current,sha);char hex[65];for(unsigned i=0;i<32;i++)sprintf(hex+i*2,"%02x",sha[i]);
  String s; s.reserve(1600);
  s="{\"project\":\""+String(projectName)+"\",\"revision\":\""+esp_app_get_description()->version+"\",\"slot\":\""+current->label+"\",\"address\":"+String(current->address);
  s+=",\"capacity\":"+String(next?next->size:0)+",\"sketch_bytes\":"+String(ESP.getSketchSize())+",\"image_sha256\":\""+hex+"\"";
  s+=",\"chip\":\""+String(ESP.getChipModel())+"\",\"chip_revision\":"+String(ESP.getChipRevision())+",\"cpu_mhz\":"+String(ESP.getCpuFreqMHz())+",\"flash_bytes\":"+String(ESP.getFlashChipSize());
  s+=",\"psram_bytes\":"+String(ESP.getPsramSize())+",\"free_psram\":"+String(ESP.getFreePsram())+",\"free_heap\":"+String(ESP.getFreeHeap())+",\"largest_internal\":"+String(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));
  s+=",\"camera_ready\":"+String(ready?"true":"false")+",\"camera_error\":\""+cameraInitError+"\",\"pcf_present\":"+String(board.present?"true":"false")+",\"lcd_enabled\":"+String(board.lcdEnabled?"true":"false");
  s+=",\"battery_enabled\":"+String(board.batteryEnabled?"true":"false")+",\"ota_active\":"+String(otaMode?"true":"false")+",\"bench_import\":"+String(benchImport?"true":"false")+",\"fs_mounted\":"+String(photos.mounted?"true":"false");
  s+=",\"fs_total\":"+String(photos.mounted?LittleFS.totalBytes():0)+",\"fs_used\":"+String(photos.mounted?LittleFS.usedBytes():0)+",\"ram_photo_bytes\":"+String(photos.ramUsed())+",\"flash_photo_bytes\":"+String(photos.flashUsed())+",\"photos\":[";
  for(unsigned i=0;i<photos.count;i++){if(i)s+=',';auto& p=photos.entries[i];s+="{\"id\":"+String(p.id)+",\"bytes\":"+String(p.bytes)+",\"storage\":\""+(p.ram?"psram":"flash")+"\"}";}
  s+= "],\"uptime_ms\":"+String(millis())+",\"reset_reason\":"+String((int)esp_reset_reason())+"}";return s;
}
bool startPortal(bool update){
  if(update){auto next=esp_ota_get_next_update_partition(nullptr);if(!next||next->address==esp_ota_get_running_partition()->address){operation="Second OTA slot missing";return false;}cameraStop();if(!photos.clear()){operation="Cannot erase photo queue";return false;}}
  if(!portal){
    sessionToken=String(esp_random(),HEX)+String(esp_random(),HEX)+String(esp_random(),HEX)+String(esp_random(),HEX);
    apName="EspCam-"+String((uint32_t)ESP.getEfuseMac()&0xffff,HEX);
    WiFi.persistent(false);WiFi.mode(WIFI_AP);
    if(!WiFi.softAPConfig(IPAddress(192,168,8,1),IPAddress(192,168,8,1),IPAddress(255,255,255,0))||!WiFi.softAP(apName.c_str(),"CHANGE_ME_ESP_AP",1,false,1)){operation="AP failed";return false;}
    web.begin();portal=true;
  }
  otaMode=update;operation=update?"OTA ready; photos erased":"Wi-Fi gallery ready";return true;
}
bool saveCamera(){
  if(otaMode){operation="Exit update mode first";return false;}
  if(!ready)cameraBenchSetup();
  if(!ready){operation="Camera absent/error "+cameraInitError;cameraStop();return false;}
  if(afMode&&esp_camera_sensor_get()->id.PID==0x5640){focusOnce();if(!afFocusConfirmed){operation="Autofocus unconfirmed";cameraStop();return false;}}
  discardFrames(1);camera_fb_t* f=esp_camera_fb_get();bool ok=false;
  if(f){ok=f->format==PIXFORMAT_JPEG&&photos.save(f->buf,f->len);esp_camera_fb_return(f);}
  operation=ok?"Photo saved":photos.error.isEmpty()?"Capture failed":photos.error;cameraStop();return ok;
}

#include "ota_receiver.h"

const char page[] PROGMEM=R"HTML(<!doctype html><html lang="pt-BR"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>ESP-CAM</title><style>body{font:17px system-ui;max-width:680px;margin:24px auto;padding:20px;background:#f4f5f0;color:#183632}button,input{font:inherit;padding:10px;margin:8px 4px 8px 0}pre{white-space:pre-wrap;overflow-wrap:anywhere}a{display:block;margin:12px 0}</style><h1>ESP-CAM</h1><pre id="state">Conectando...</pre><button id="capture">Tirar foto</button><button id="gallery">Atualizar lista</button><div id="photos"></div><hr><h2>Atualizar firmware</h2><p>Entrar na atualizacao apaga as fotos. Escolha esp32cam_integrada.ino.bin.</p><button id="mode">Entrar na atualizacao</button><input id="file" type="file" accept=".bin"><button id="send">Enviar firmware</button><button id="exit">Desligar Wi-Fi</button><pre id="result"></pre><script>
let token='',state=document.getElementById('state'),result=document.getElementById('result');
async function refresh(){let j=await(await fetch('/info',{cache:'no-store'})).json();token=j.token;state.textContent='Versao: '+j.revision+'\nSlot: '+j.slot+'\nFotos: '+j.photos.length+'\nCamera: '+(j.camera_ready?'ativa':'em espera / desconectada')+'\nAtualizacao: '+(j.ota_active?'pronta':'inativa');let box=document.getElementById('photos');box.replaceChildren();j.photos.forEach(p=>{let a=document.createElement('a');a.textContent='Foto '+p.id+' - '+Math.round(p.bytes/1024)+' KiB ('+p.storage+')';a.href='/photo?id='+p.id;box.appendChild(a)})}
async function action(path){let r=await fetch(path,{method:'POST',headers:{'X-Update-Token':token}});result.textContent=await r.text();if(path!='/api/off')await refresh()}
document.getElementById('capture').onclick=()=>action('/api/capture');document.getElementById('gallery').onclick=refresh;document.getElementById('mode').onclick=()=>action('/api/ota');document.getElementById('exit').onclick=()=>action('/api/off');
document.getElementById('send').onclick=()=>{let f=document.getElementById('file').files[0];if(!f){result.textContent='Escolha o arquivo.';return}let x=new XMLHttpRequest();x.open('POST','/update');x.setRequestHeader('X-Update-Token',token);x.setRequestHeader('X-Firmware-Size',f.size);x.upload.onprogress=e=>result.textContent='Enviando: '+Math.round(100*e.loaded/e.total)+'%';x.onload=()=>result.textContent=x.responseText;x.onerror=()=>result.textContent='Conexao interrompida; confira a versao apos reiniciar.';let b=new FormData();b.append('firmware',f);x.send(b)};refresh().catch(e=>state.textContent='Sem conexao com ESP-CAM');</script></html>)HTML";

bool importAccepted=false,importComplete=false;String importError;
void routes(){
  const char* headers[]={"X-Update-Token","X-Firmware-Size","X-Firmware-SHA256","X-Photo-Size"};web.collectHeaders(headers,4);
  web.on("/",HTTP_GET,[]{if(!local()){reply(403,"Local AP only");return;}web.sendHeader("Cache-Control","no-store");web.send_P(200,"text/html; charset=utf-8",page);});
  web.on("/info",HTTP_GET,[]{if(!local()){reply(403,"Local AP only");return;}String json=state();json.remove(json.length()-1);json+=",\"token\":\""+sessionToken+"\"}";web.sendHeader("Cache-Control","no-store");web.send(200,"application/json",json);});
  web.on("/api/capture",HTTP_POST,[]{if(!authorized()){reply(403,"Unauthorized");return;}bool ok=saveCamera();reply(ok?200:503,operation);});
  web.on("/api/ota",HTTP_POST,[]{if(!authorized()){reply(403,"Unauthorized");return;}bool ok=startPortal(true);reply(ok?200:409,operation);});
  web.on("/api/off",HTTP_POST,[]{if(!authorized()||ota.receiving||ota.complete){reply(403,"Cannot exit now");return;}reply(200,"Wi-Fi off");delay(20);web.stop();WiFi.softAPdisconnect(true);WiFi.mode(WIFI_OFF);portal=otaMode=false;});
  web.on("/photo",HTTP_GET,[]{
    if(!local()){reply(403,"Local AP only");return;}auto p=photos.get(strtoul(web.arg("id").c_str(),nullptr,10));if(!p){reply(404,"Photo not found");return;}
    web.sendHeader("Cache-Control","no-store");
    if(p->ram){web.setContentLength(p->bytes);web.send(200,"image/jpeg","");size_t at=0;while(at<p->bytes&&web.client().connected()){size_t n=web.client().write(p->ram+at,min((size_t)4096,p->bytes-at));if(!n)break;at+=n;delay(0);}}
    else{File f=photos.open(p->id);if(!f){reply(404,"Photo unavailable");return;}web.streamFile(f,"image/jpeg");f.close();}
  });
  // USB-enabled bench facility: exercise real JPEG storage without a sensor.
  web.on("/import",HTTP_POST,[]{if(!authorized()){reply(403,"Unauthorized");return;}reply(importComplete?200:400,importComplete?"Photo imported":importError);},[]{
    auto& u=web.upload();if(u.status==UPLOAD_FILE_START){importAccepted=importComplete=false;importError="";if(!authorized()||!benchImport||otaMode){importError="Bench import disabled";return;}importAccepted=photos.start(strtoul(web.header("X-Photo-Size").c_str(),nullptr,10),web.arg("storage")=="flash");if(!importAccepted)importError=photos.error;}
    else if(u.status==UPLOAD_FILE_WRITE&&importAccepted){if(!photos.write(u.buf,u.currentSize)){importAccepted=false;importError=photos.error;}}
    else if(u.status==UPLOAD_FILE_END&&importAccepted){importComplete=photos.finish();if(!importComplete)importError=photos.error;importAccepted=false;}
    else if(u.status==UPLOAD_FILE_ABORTED){photos.abort();importAccepted=false;importError="Interrupted photo";}
  });
  web.on("/update",HTTP_POST,[]{if(!authorized()){reply(403,"Unauthorized");return;}if(ota.complete){reply(200,"Firmware verified; restarting");rebootAt=millis()+1800;}else reply(400,ota.error.isEmpty()?"Incomplete firmware":ota.error);},[]{
    auto& u=web.upload();if(u.status==UPLOAD_FILE_START)ota.start();else if(u.status==UPLOAD_FILE_WRITE)ota.write(u.buf,u.currentSize);else if(u.status==UPLOAD_FILE_END)ota.finish();else if(u.status==UPLOAD_FILE_ABORTED)ota.fail("Upload interrupted");
  });
  web.onNotFound([]{reply(404,"Not found");});
}

void serialCommand(String c){
  c.trim();if(c=="STATUS"){Serial.println("INFO "+state());return;}
  if(c=="REBOOT"){if(ota.receiving){Serial.println("ERR update in progress");return;}Serial.println("OK reboot");Serial.flush();ESP.restart();return;}
  if(c=="OTA"||c=="WIFI"||c=="BENCH"){if(c=="BENCH")benchImport=true;bool ok=startPortal(c=="OTA");Serial.println(ok?"OK "+apName+" http://192.168.8.1":"ERR "+operation);return;}
  if(c=="OFF"){if(ota.receiving||ota.complete){Serial.println("ERR update in progress");return;}web.stop();WiFi.softAPdisconnect(true);WiFi.mode(WIFI_OFF);portal=otaMode=false;Serial.println("OK Wi-Fi off");return;}
  if(c=="FORMAT"){Serial.println(!ota.receiving&&photos.format()?"OK formatted":"ERR format failed");return;}
  if(c=="SAVE"){bool ok=saveCamera();Serial.println((ok?"OK ":"ERR ")+operation);return;}
  if(c=="CLEAR"){Serial.println(photos.clear()?"OK cleared":"ERR clear failed");return;}
  if(c=="LCD ON"){Serial.println(board.displayBegin()?"OK LCD initialized, physical test pending":"ERR PCF absent");return;}
  if(c=="ADC ON"){board.batteryEnabled=true;Serial.println("OK ADC enabled; GPIO33 LED must be isolated");return;}
  if(c=="ADC"){Serial.printf("BATTERY_MV %u\n",board.batteryMillivolts());return;}
  if(c.startsWith("CALC ")){auto r=calc::Parser(c.substring(5).c_str(),true,0,{}).run();Serial.println(r.ok?String("RESULT ")+calc::format(r.value).c_str():String("ERR ")+r.error.c_str());return;}
  if(c=="I"&&!ready){Serial.println("INFO "+state());return;}
  if(otaMode){Serial.println("ERR update mode active");return;}
  if(c=="CAMERA"){cameraStop();cameraBenchSetup();Serial.println(ready?"OK camera ready":"ERR camera "+cameraInitError);return;}
  if(!ready){Serial.println("CAMERA_ERROR "+cameraInitError);return;}
  if(c=="P")capture(true);else if(c=="V")capture(false);else if(c=="I")info();else if(c=="F"){focusOnce();info();}else if(c=="S"){profile(true);Serial.println("OK saved");}else if(c.startsWith("C "))control(c);else if(c.startsWith("Q"))capture(true,c.substring(1).toInt());else Serial.println("ERR unknown command");
}
void setup(){
  Serial.begin(460800);pinMode(33,INPUT);delay(100);WiFi.persistent(false);WiFi.mode(WIFI_OFF);
  photos.begin();board.begin();routes();cameraBenchSetup();if(!ready)cameraStop();
  Serial.println("INFO "+state());
}
void loop(){
  if(ready&&!otaMode)regulate();if(portal)web.handleClient();
  for(unsigned n=0;n<256&&Serial.available();n++){char c=Serial.read();if(c=='\r')continue;if(c=='\n'){serialCommand(serialLine);serialLine="";}else if(serialLine.length()<256)serialLine+=c;else serialLine="";}
  uint32_t before=board.keys,keys=board.scan();if(keys!=before)Serial.printf("KEY_MASK %lu\n",(unsigned long)keys);
  if(board.lcdEnabled&&millis()-lastDisplay>1000){lastDisplay=millis();board.line(0,"ESP-CAM");board.line(1,esp_app_get_description()->version);board.line(2,operation.c_str());board.line(3,apName.c_str());char text[32];snprintf(text,sizeof(text),"Fotos: %u",photos.count);board.line(4,text);board.show();}
  if(rebootAt&&(int32_t)(millis()-rebootAt)>=0){Serial.flush();ESP.restart();}delay(2);
}

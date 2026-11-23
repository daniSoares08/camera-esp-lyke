#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <esp_app_desc.h>
#include <esp_image_format.h>
#include <functional>

// Explicit, temporary local AP. No internet service, saved home credentials or
// API key are involved. Only the inactive application partition is written.
class WifiUpdate {
#if CONFIG_IDF_TARGET_ESP32S3
  static constexpr uint16_t expectedChip=9;
#else
  static constexpr uint16_t expectedChip=0;
#endif
  WebServer server{80};
  String token,expectedProject,failure;
  bool running=false,accepted=false,complete=false;
  wifi_mode_t previousMode=WIFI_OFF;
  uint32_t until=0,rebootAt=0,lastActivity=0,exitAt=0;
  size_t received=0;
  std::function<void(const String&)> notify;
  bool authorized(){return running&&server.client().localIP()==WiFi.softAPIP()&&server.header("X-Update-Token")==token;}
  void fail(const String& why){failure=why;accepted=false;if(Update.isRunning())Update.abort();}
public:
  bool active()const{return running;}
  String ssid,password;
  void beginRoutes(const String& project,std::function<void(const String&)> callback){
    expectedProject=project;notify=callback;
    const char* headers[]={"X-Update-Token","X-Firmware-MD5"};server.collectHeaders(headers,2);
    server.on("/exit",HTTP_POST,[this]{if(!authorized()){server.send(403);return;}server.send(200,"text/plain","Modo de atualizacao encerrado.");exitAt=millis()+250;});
    server.on("/",HTTP_GET,[this]{
      if(!running||server.client().localIP()!=WiFi.softAPIP()){server.send(403);return;}
      String page=R"HTML(<!doctype html><html lang="pt-BR"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Atualizar calculadora</title><style>body{font:17px system-ui;max-width:600px;margin:30px auto;padding:20px;background:#17241b;color:#e5efdc}button,input{font:inherit;margin:14px 0;padding:12px;max-width:100%}button{background:#d3e990}pre{white-space:pre-wrap}</style><h1>Atualizar __PROJECT__</h1><p>Escolha somente o arquivo <b>__PROJECT__.ino.bin</b>. Nao use merged, bootloader ou partitions. Mantenha a alimentacao ate reiniciar.</p><input type="file" id="file" accept=".bin"><br><button id="go">Enviar firmware</button><pre id="status">Rede local temporaria; nao precisa de internet.</pre><script>const token='__TOKEN__',go=document.getElementById('go'),file=document.getElementById('file'),state=document.getElementById('status');go.onclick=()=>{const f=file.files[0];if(!f){state.textContent='Escolha o arquivo.';return}go.disabled=true;let x=new XMLHttpRequest();x.open('POST','/update');x.setRequestHeader('X-Update-Token',token);x.upload.onprogress=e=>{state.textContent='Enviando: '+Math.round(100*e.loaded/e.total)+'%'};x.onload=()=>{state.textContent=x.responseText;go.disabled=false};x.onerror=()=>{state.textContent='Conexao interrompida. Reconecte e confira se o aparelho reiniciou.';go.disabled=false};let b=new FormData();b.append('firmware',f);x.send(b)};</script></html>)HTML";
      page.replace("__PROJECT__",expectedProject);page.replace("__TOKEN__",token);
      server.sendHeader("Cache-Control","no-store");server.send(200,"text/html; charset=utf-8",page);
    });
    server.on("/info",HTTP_GET,[this]{
      if(!running||server.client().localIP()!=WiFi.softAPIP()){server.send(403);return;}
      auto slot=esp_ota_get_next_update_partition(nullptr);auto current=esp_ota_get_running_partition();
      String body="{\"project\":\""+expectedProject+"\",\"slot\":\""+current->label+"\",\"capacity\":"+String(slot?slot->size:0)+",\"token\":\""+token+"\"}";
      server.sendHeader("Cache-Control","no-store");server.send(200,"application/json",body);
    });
    server.on("/update",HTTP_POST,[this]{
      if(!authorized()){server.send(403,"text/plain","Acesso negado");return;}
      if(complete){server.send(200,"text/plain; charset=utf-8","Firmware verificado. Reiniciando em 2 segundos.");rebootAt=millis()+2000;}
      else server.send(400,"text/plain; charset=utf-8",failure.isEmpty()?"Arquivo incompleto; firmware anterior preservado.":failure);
    },[this]{
      auto& upload=server.upload();lastActivity=millis();
      if(upload.status==UPLOAD_FILE_START){
        failure="";complete=false;accepted=false;received=0;
        if(!authorized()){failure="Acesso negado";return;}
        if(upload.filename!=expectedProject+".ino.bin"){failure="Arquivo incorreto. Use "+expectedProject+".ino.bin";return;}
        auto next=esp_ota_get_next_update_partition(nullptr);auto current=esp_ota_get_running_partition();
        if(!next||next->address==current->address){failure="Segunda particao OTA ausente; grave a versao inicial por USB.";return;}
        accepted=true;
      }else if(upload.status==UPLOAD_FILE_WRITE&&accepted){
        if(!received){
          const size_t at=sizeof(esp_image_header_t)+sizeof(esp_image_segment_header_t);
          if(upload.currentSize<at+sizeof(esp_app_desc_t)){fail("Cabecalho incompleto.");return;}
          esp_image_header_t image;esp_app_desc_t desc;
          memcpy(&image,upload.buf,sizeof(image));memcpy(&desc,upload.buf+at,sizeof(desc));
          if(image.magic!=ESP_IMAGE_HEADER_MAGIC||image.chip_id!=expectedChip||desc.magic_word!=ESP_APP_DESC_MAGIC_WORD){fail("Firmware de outra placa ou arquivo invalido. Atualizacao recusada.");return;}
          if(!Update.begin(UPDATE_SIZE_UNKNOWN,U_FLASH)){fail(Update.errorString());return;}
          String md5=server.header("X-Firmware-MD5");if(md5.length()&&!Update.setMD5(md5.c_str())){fail("MD5 invalido.");return;}
        }
        if(Update.write(upload.buf,upload.currentSize)!=upload.currentSize){fail(Update.errorString());return;}
        received+=upload.currentSize;
      }else if(upload.status==UPLOAD_FILE_END&&accepted){
        if(!received||!Update.end(true)){fail(Update.errorString());return;}
        complete=true;accepted=false;if(notify)notify("Atualizacao verificada. Reiniciando...");
      }else if(upload.status==UPLOAD_FILE_ABORTED){fail("Envio interrompido; firmware anterior preservado.");}
    });
  }
  bool start(const String& name,const String& pass){
    stop();ssid=name;password=pass;token=String(esp_random(),HEX)+String(esp_random(),HEX)+String(esp_random(),HEX);
    auto next=esp_ota_get_next_update_partition(nullptr);if(!next||next->address==esp_ota_get_running_partition()->address)return false;
    previousMode=WiFi.getMode();if(previousMode==WIFI_AP||previousMode==WIFI_AP_STA)previousMode=WIFI_STA;
    WiFi.mode(WIFI_AP_STA);
    if(!WiFi.softAPConfig(IPAddress(192,168,8,1),IPAddress(192,168,8,1),IPAddress(255,255,255,0))||!WiFi.softAP(ssid.c_str(),password.c_str(),1,false,1))return false;
    running=true;until=millis()+600000;lastActivity=millis();server.begin();return true;
  }
  void stop(){
    if(Update.isRunning())Update.abort();accepted=complete=false;rebootAt=0;exitAt=0;
    if(running){server.stop();WiFi.softAPdisconnect(true);WiFi.mode(previousMode);}
    running=false;
  }
  void loop(){
    if(!running)return;server.handleClient();uint32_t now=millis();
    if(exitAt&&(int32_t)(now-exitAt)>=0){stop();return;}
    if(rebootAt&&(int32_t)(now-rebootAt)>=0){delay(100);ESP.restart();}
    if(accepted&&now-lastActivity>30000)fail("Tempo de envio esgotado; tente novamente.");
    // Explicit exit or successful update ends OTA; no idle expiration.
  }
};

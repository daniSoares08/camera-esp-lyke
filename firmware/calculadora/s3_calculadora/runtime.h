#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <esp_log.h>
#include "calculator_app.h"
#include "link_protocol.h"
#include "keyboard_scanner.h"
#include "network_service.h"
#include "wifi_update.h"

#if ARDUINO_USB_CDC_ON_BOOT
#error "Use UART0: CDCOnBoot=default. OTG pins 19/20 belong to the keypad."
#endif
namespace calculator_runtime {
calc::App app;
networkservice::Service network;
WebServer web(80);
WifiUpdate ota;
SemaphoreHandle_t uartMutex;
QueueHandle_t frameQueue;
struct Frame {uint8_t bytes[1024];};
bool portalActive=false;
bool portalPhotoMode=false;
int pendingControl=0,pendingConnect=-1;
uint32_t pendingAt=0;
uint32_t portalUntil=0,lastRender=0,lastStatus=0,repeatAt=0;
int heldArrow=-1;
String portalToken,portalPassword,portalSSID,serialLine;

inline void reloadProfiles(){app.profiles.clear();for(const auto& p:deviceconfig::profiles())app.profiles.push_back(p.ssid.c_str());app.hasKey=!deviceconfig::apiKey().isEmpty();app.dirty=true;}
inline void ack(JsonDocument& doc){String out;serializeJson(doc,out);if(xSemaphoreTake(uartMutex,pdMS_TO_TICKS(500))==pdTRUE){Serial.print("\nCFG ");Serial.println(out);Serial.flush();xSemaphoreGive(uartMutex);}}
inline void frameTask(void*){
  Frame latest{};bool available=false;uint16_t seq=0;
  for(;;){Frame next;if(xQueueReceive(frameQueue,&next,pdMS_TO_TICKS(1000))==pdTRUE){memcpy(&latest,&next,sizeof(latest));available=true;}if(!available)continue;
    uint8_t header[10];linkproto::makeHeader(header,++seq,latest.bytes);
    if(xSemaphoreTake(uartMutex,pdMS_TO_TICKS(500))==pdTRUE){Serial.write(header,sizeof(header));Serial.write(latest.bytes,sizeof(latest.bytes));Serial.flush();xSemaphoreGive(uartMutex);}
  }
}
inline bool fromPortal(){bool ok=portalActive&&web.client().localIP()==WiFi.softAPIP();if(ok)portalUntil=millis()+600000;return ok;}
inline void jsonReply(int code,const String& body){web.sendHeader("Cache-Control","no-store");web.sendHeader("X-Content-Type-Options","nosniff");web.send(code,"application/json; charset=utf-8",body);}
inline String configPage(){
  String page=R"PAGE(<!doctype html><html lang="pt-BR"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Configurar calculadora</title><style>body{font:16px system-ui;background:#152019;color:#e6efde;max-width:560px;padding:24px;margin:auto}input,select,button{font:inherit;box-sizing:border-box;width:100%;padding:12px;border-radius:6px;border:1px solid #6f826c;margin:6px 0 16px;background:#223327;color:inherit}button{background:#d7ef94;color:#203020;font-weight:bold;cursor:pointer}small,p{color:#b0c1aa}a{color:#d7ef94}h1{font-size:25px}fieldset{border:1px solid #596a52;border-radius:10px;margin:20px 0;padding:18px}</style><h1>Configurar calculadora</h1><p>Rede local temporária do aparelho. As configurações ficam salvas nele.</p><p id="state">Lendo configuração…</p><form id="form"><fieldset><legend>OpenAI API</legend><label>Chave de API<input id="key" type="password" autocomplete="off" placeholder="sk-…; deixe vazio para manter"></label><label>Modelo<select id="model"><option value="precise">Preciso — GPT-6.1 Sol</option><option value="economy">Econômico — GPT-6 Luna</option></select></label><small>Use uma chave própria de projeto. A foto só será enviada quando você selecionar a consulta no aparelho.</small><p><a href="https://platform.openai.com/api-keys" target="_blank" rel="noreferrer">Gerenciar chaves</a> · <a href="https://platform.openai.com/settings/organization/billing/overview" target="_blank" rel="noreferrer">Créditos e cobrança</a></p></fieldset><fieldset><legend>Adicionar ou atualizar Wi-Fi</legend><label>Nome da rede (opcional)<input id="ssid" maxlength="32" autocomplete="off"></label><label>Senha<input id="password" type="password" maxlength="63" autocomplete="off"></label><small>Use uma rede de 2,4 GHz. Repetir o nome atualiza sua senha. Para rede aberta, deixe a senha vazia.</small></fieldset><button>Salvar no aparelho</button></form><p id="result" role="status"></p><p>Depois de salvar, volte à calculadora e selecione a rede no menu Wi-Fi. Esta rede de configuração fecha após dez minutos.</p><script>const token='__TOKEN__';fetch('/api/status').then(r=>r.json()).then(s=>{document.getElementById('state').textContent=(s.api_key_set?'Chave configurada. ':'Chave ainda não configurada. ')+'Redes: '+s.profiles.join(', ');document.getElementById('model').value=s.economy?'economy':'precise'});document.getElementById('form').onsubmit=async e=>{e.preventDefault();const body={key:document.getElementById('key').value,ssid:document.getElementById('ssid').value,password:document.getElementById('password').value,economy:document.getElementById('model').value==='economy'};try{let r=await fetch('/api/save',{method:'POST',headers:{'Content-Type':'application/json','X-Config-Token':token},body:JSON.stringify(body)});let j=await r.json();document.getElementById('result').textContent=j.message;if(r.ok){document.getElementById('key').value='';document.getElementById('password').value=''}}catch(e){document.getElementById('result').textContent='A conexão local foi interrompida.'}}</script></html>)PAGE";
  page.replace("__TOKEN__",portalToken);return page;
}
inline String photoPage(){
  String page=R"PAGE(<!doctype html><html lang="pt-BR"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Foto da calculadora</title><style>body{font:16px system-ui;background:#152019;color:#e6efde;max-width:1100px;margin:auto;padding:22px}button,a,select{font:inherit;padding:10px;margin:5px;border-radius:6px}button{background:#d7ef94;color:#203020;border:0;cursor:pointer}a{color:#d7ef94;display:inline-block}img{max-width:100%;height:auto;background:#303c22}img.full{max-width:none}#frame{overflow:auto;max-height:75vh;border:1px solid #6f826c}pre{white-space:pre-wrap}small{color:#bac8b1}</style><h1>Foto da calculadora</h1><p>Nas consultas, a imagem abaixo e o mesmo JPEG enviado a API. A camera entra em standby entre as capturas. Nao e video ao vivo. Foto 1/2/3 escolhe apenas o que voce ve; a consulta envia todas as fotos dos quadradinhos. Pode enviar uma, duas ou tres fotos; nao precisa preencher os tres.</p><div><button id="capture">Tirar foto sem API</button><button id="consult">Consultar API (fotos selecionadas)</button><button id="zoom">Alternar tamanho original</button><a id="download" href="/foto.jpg?download=1" download="ultima-foto.jpg">Baixar JPEG original</a></div><div id="gallery"></div><pre id="info">Lendo estado...</pre><div id="frame"><img id="photo" alt="Nenhuma foto capturada nesta inicializacao" hidden></div><h2>Rede e manutencao</h2><select id="networks"></select><button id="connect">Conectar calculadora ao Wi-Fi</button><button id="update">Atualizar S3</button><button id="display">Atualizar tela</button><small>Para usar a API, selecione uma rede salva para a calculadora. Seu celular pode permanecer nesta rede local. A ultima foto fica em memoria ate reiniciar ou substituir por outra.</small><pre id="message"></pre><script>
const token='__TOKEN__',get=id=>document.getElementById(id),photo=get('photo'),info=get('info'),message=get('message');let seq=-1,loadedNetworks=false,selected=-1,slots=[],galleryKey="";
function showPhoto(){let p=selected<0?null:slots[selected];let suffix=p?"slot="+selected+"&seq="+p.sequence:"seq="+seq;photo.src="/foto.jpg?"+suffix;photo.hidden=false;get("download").href="/foto.jpg?download=1&"+suffix;}
async function action(path,body){let r=await fetch(path,{method:'POST',headers:{'Content-Type':'application/json','X-Config-Token':token},body:JSON.stringify(body)});let j=await r.json();message.textContent=j.message||JSON.stringify(j);if(!r.ok)throw Error(message.textContent);return j}
get('capture').onclick=()=>action('/api/capture',{api:false,slot:true}).catch(e=>message.textContent=e.message);
get('consult').onclick=()=>action('/api/capture',{api:true,batch:true}).catch(e=>message.textContent=e.message);
get('zoom').onclick=()=>photo.classList.toggle('full');get('connect').onclick=()=>action('/api/control',{action:'connect',index:Number(get('networks').value)}).catch(e=>message.textContent=e.message);
get('update').onclick=()=>action('/api/control',{action:'ota_s3'}).catch(e=>message.textContent=e.message);get('display').onclick=()=>action('/api/control',{action:'ota_display'}).catch(e=>message.textContent=e.message);
async function refresh(){try{let r=await fetch('/api/photo',{cache:'no-store'}),j=await r.json();slots=j.photos||[];let shown=selected<0?j:(slots[selected]||j);info.textContent=(j.available?`${shown.width} x ${shown.height} | JPEG ${shown.bytes} bytes | Foto #${shown.sequence}`:'Nenhuma foto nesta inicializacao.')+`\nFotos para consulta: ${j.photo_count||0}/3\n${j.api_attempted?"JPEG da consulta a API (HTTP "+j.api_http_status+")":"Captura local, sem envio a API"}\nCamera: ${j.camera_active?'capturando':j.camera_sensor_standby_now?'standby confirmado; captura encerrada':'sem captura em andamento'} | Wi-Fi da calculadora: ${j.wifi_connected?'conectado':'desconectado'}\n${j.status}${!j.wifi_connected&&j.wifi_disconnect_reason?" (motivo Wi-Fi "+j.wifi_disconnect_reason+")":""}`;let key=slots.map(p=>p.sequence).join(',');if(key!==galleryKey){galleryKey=key;get('gallery').replaceChildren();let all=[{label:'Ultima captura',index:-1},...slots.map((p,i)=>({label:'Foto '+(i+1),index:i}))];all.forEach(item=>{let b=document.createElement('button');b.textContent=item.label;b.onclick=()=>{selected=item.index;showPhoto()};get('gallery').appendChild(b)});if(selected>=slots.length)selected=-1;}if(j.available&&j.sequence!==seq){seq=j.sequence;showPhoto()}if(!loadedNetworks){j.profiles.forEach((name,index)=>{let o=document.createElement('option');o.value=index;o.textContent=name;get('networks').appendChild(o)});loadedNetworks=true}}catch(e){info.textContent='Sem conexao com a calculadora. Reconecte a rede local ou reabra o modo Ver foto no Wi-Fi.'}setTimeout(refresh,2000)}refresh();
</script></html>)PAGE";
  page.replace("__TOKEN__",portalToken);return page;
}
inline void configureWeb(){
  const char* headers[]={"X-Config-Token"};web.collectHeaders(headers,1);
  web.on("/",HTTP_GET,[]{if(!fromPortal()){web.send(403,"text/plain","Use a rede mostrada no LCD.");return;}web.sendHeader("Cache-Control","no-store");web.send(200,"text/html; charset=utf-8",portalPhotoMode?photoPage():configPage());});
  web.on("/foto",HTTP_GET,[]{if(!fromPortal()){web.send(403);return;}web.sendHeader("Cache-Control","no-store");web.send(200,"text/html; charset=utf-8",photoPage());});
  web.on("/api/photo",HTTP_GET,[]{
    if(!fromPortal()){web.send(403);return;}auto photo=network.lastPhoto();JsonDocument doc;
    JsonArray items=doc["photos"].to<JsonArray>();auto selected=network.selectedPhotos();for(const auto& p:selected){JsonObject item=items.add<JsonObject>();item["sequence"]=p.sequence;item["width"]=p.width;item["height"]=p.height;item["bytes"]=p.length;}
    doc["photo_count"]=selected.size();JsonArray found=doc["scan_results"].to<JsonArray>();for(const auto& name:app.scanned)found.add(name);doc["scan_done"]=app.view==calc::View::SCANNED;doc["available"]=(bool)photo.bytes;doc["sequence"]=photo.sequence;doc["width"]=photo.width;doc["height"]=photo.height;doc["bytes"]=photo.length;doc["captured_ms"]=photo.capturedAt;doc["camera_active"]=network.cameraActive();doc["camera_standby"]=photo.standby;doc["camera_sensor_standby_now"]=network.cameraStandbyConfirmed();doc["api_attempted"]=photo.apiAttempted;doc["api_http_status"]=photo.apiHttpStatus;doc["busy"]=network.busy();doc["wifi_connected"]=WiFi.status()==WL_CONNECTED;doc["wifi_disconnect_reason"]=network.wifiDisconnectReason();doc["wifi_status"]=(int)WiFi.status();doc["status"]=app.status;doc["firmware"]="calculadora-s3-photo-v1";doc["ota_slot"]=esp_ota_get_running_partition()->label;
    JsonArray names=doc["profiles"].to<JsonArray>();for(const auto& p:app.profiles)names.add(p);String raw;serializeJson(doc,raw);jsonReply(200,raw);
  });
  web.on("/foto.jpg",HTTP_GET,[]{
    if(!fromPortal()){web.send(403);return;}auto photo=network.lastPhoto();if(web.hasArg("slot")){String requested=web.arg("slot");auto selected=network.selectedPhotos();if(requested.length()!=1||requested[0]<'0'||requested[0]>'2'||(unsigned)(requested[0]-'0')>=selected.size()){web.send(404);return;}photo=selected[requested[0]-'0'];}if(!photo.bytes){web.send(404,"text/plain","Tire uma foto primeiro.");return;}
    web.sendHeader("Cache-Control","no-store");web.sendHeader("X-Photo-Sequence",String(photo.sequence));if(web.hasArg("download"))web.sendHeader("Content-Disposition","attachment; filename=ultima-foto.jpg");web.setContentLength(photo.length);web.send(200,"image/jpeg","");
    size_t sent=0;uint32_t start=millis();while(sent<photo.length&&web.client().connected()&&millis()-start<15000){size_t n=web.client().write(photo.bytes.get()+sent,photo.length-sent);if(!n)break;sent+=n;delay(1);}
  });
  web.on("/api/capture",HTTP_POST,[]{
    if(!fromPortal()||web.header("X-Config-Token")!=portalToken){web.send(403);return;}
    if(network.busy()||ota.active()){jsonReply(409,"{\"message\":\"Aguarde terminar a operacao atual.\"}");return;}
    JsonDocument body;if(web.arg("plain").length()>128||deserializeJson(body,web.arg("plain"))){web.send(400);return;}
    networkservice::Job job{};job.kind=(body["api"]|false)?((body["batch"]|false)?networkservice::JobKind::SEND_BATCH:networkservice::JobKind::PHOTO):((body["slot"]|false)?networkservice::JobKind::CAPTURE_SLOT:networkservice::JobKind::SNAPSHOT);job.economy=app.economy;job.mirror=app.mirror;job.flip=app.flip;
    if(!network.enqueue(job)){web.send(409);return;}app.busy=true;app.photoCapturing=job.kind==networkservice::JobKind::CAPTURE_SLOT;app.view=calc::View::BUSY;app.setStatus(job.kind==networkservice::JobKind::SEND_BATCH?"Preparando envio das fotos...":"Preparando foto...");jsonReply(202,job.kind==networkservice::JobKind::SEND_BATCH?"{\"message\":\"Consulta solicitada com as fotos dos quadradinhos.\"}":"{\"message\":\"Captura solicitada. A imagem sera atualizada abaixo.\"}");
  });
  web.on("/api/control",HTTP_POST,[]{
    if(!fromPortal()||web.header("X-Config-Token")!=portalToken){web.send(403);return;}if(network.busy()){jsonReply(409,"{\"message\":\"Aguarde terminar a operacao atual.\"}");return;}
    JsonDocument body;if(web.arg("plain").length()>128||deserializeJson(body,web.arg("plain"))){web.send(400);return;}String action=body["action"]|"";
    if(action=="scan"){networkservice::Job job{};job.kind=networkservice::JobKind::SCAN;if(!network.enqueue(job)){web.send(409);return;}app.view=calc::View::BUSY;app.setStatus("Buscando redes Wi-Fi...");jsonReply(202,"{\"message\":\"Buscando redes.\"}");return;}
    if(action=="normal"){app.view=calc::View::CALCULATOR;app.dirty=true;jsonReply(200,"{\"message\":\"Calculadora na tela normal.\"}");return;}
    if(action=="ota_s3"||action=="ota_display"){pendingControl=action=="ota_s3"?1:2;pendingAt=millis()+250;jsonReply(202,"{\"message\":\"Abrindo rede de atualizacao. Confira o LCD e reconecte ao Wi-Fi mostrado.\"}");return;}
    if(action=="connect"){int index=body["index"]|-1;auto saved=deviceconfig::profiles();if(index<0||index>=(int)saved.size()){web.send(400);return;}networkservice::Job job{};job.kind=networkservice::JobKind::CONNECT;strlcpy(job.ssid,saved[index].ssid.c_str(),sizeof(job.ssid));strlcpy(job.password,saved[index].password.c_str(),sizeof(job.password));if(!network.enqueue(job)){web.send(409);return;}jsonReply(202,"{\"message\":\"Conectando a calculadora a rede salva.\"}");return;}web.send(400);
  });
  web.on("/api/status",HTTP_GET,[]{if(!fromPortal()){jsonReply(403,"{}");return;}JsonDocument doc;doc["api_key_set"]=app.hasKey;doc["economy"]=app.economy;JsonArray names=doc["profiles"].to<JsonArray>();for(const auto& p:app.profiles)names.add(p);String raw;serializeJson(doc,raw);jsonReply(200,raw);});
  web.on("/api/save",HTTP_POST,[]{
    if(!fromPortal()||web.header("X-Config-Token")!=portalToken){jsonReply(403,"{\"message\":\"Origem inválida\"}");return;}
    String raw=web.arg("plain");if(raw.length()>2048){jsonReply(400,"{\"message\":\"Dados grandes demais\"}");return;}
    JsonDocument doc;if(deserializeJson(doc,raw)){jsonReply(400,"{\"message\":\"Dados inválidos\"}");return;}
    String key=doc["key"]|"",ssid=doc["ssid"]|"",password=doc["password"]|"";
    if(!key.isEmpty()&&!deviceconfig::putKey(key)){jsonReply(400,"{\"message\":\"Chave inválida; use uma chave de API sk-…\"}");return;}
    if(!ssid.isEmpty()&&!deviceconfig::putProfile(ssid,password)){jsonReply(400,"{\"message\":\"Rede inválida ou limite de 8 redes atingido. Senha: 8–63 caracteres ou vazia.\"}");return;}
    app.economy=doc["economy"]|false;deviceconfig::saveSettings(app);reloadProfiles();jsonReply(200,"{\"message\":\"Configuração salva no aparelho.\"}");app.setStatus("Configuracao salva. ON volta; selecione sua rede no menu Wi-Fi.");
  });
  web.onNotFound([]{web.send(404,"text/plain","Nao encontrado");});
}
inline void startPortal(bool photosOnly=false){
  if(ota.active()||(!photosOnly&&network.busy())){app.setError("Aguarde a operacao atual antes de configurar.");return;}
  portalToken=String(esp_random(),HEX)+String(esp_random(),HEX)+String(esp_random(),HEX)+String(esp_random(),HEX);
  portalPassword="CHANGE_ME_ESP_AP";portalPhotoMode=photosOnly;
  portalSSID=String(photosOnly?"CalcFoto-":"CalcSetup-")+String((uint32_t)ESP.getEfuseMac()&0xFFFF,HEX);WiFi.mode(WIFI_AP_STA);
  if(!WiFi.softAPConfig(IPAddress(192,168,8,1),IPAddress(192,168,8,1),IPAddress(255,255,255,0))||!WiFi.softAP(portalSSID.c_str(),portalPassword.c_str(),1,false,1)){app.setError("Nao foi possivel abrir a configuracao Wi-Fi.");return;}
  web.begin();portalActive=true;portalUntil=millis()+600000;
  app.view=calc::View::BUSY;app.setStatus(std::string(portalSSID.c_str())+"\nSenha: "+portalPassword.c_str()+"\nAbra 192.168.8.1\nF ou ON: calculadora");
}
inline void startUpdate(bool display){
  if(network.busy()){app.setError("Aguarde terminar a operacao de rede antes de atualizar.");return;}
  if(portalActive&&!display){web.stop();WiFi.softAPdisconnect(true);portalActive=false;portalPassword="";portalToken="";}
  ota.stop();
  String pass="CHANGE_ME_ESP_AP";
  app.view=calc::View::BUSY;
  if(display){
    // Keep the V1 transport compatible with the already installed display.
    // The updated DevKit uses the requested fixed password on its AP.
    pass="1234da111111";
    uint8_t payload[1024]={},header[10];String command="CALC_OTA_START_V1|"+pass;memcpy(payload,command.c_str(),command.length());linkproto::makeHeader(header,0,payload,2);
    if(xSemaphoreTake(uartMutex,pdMS_TO_TICKS(1000))==pdTRUE){Serial.write(header,10);Serial.write(payload,1024);Serial.flush();xSemaphoreGive(uartMutex);}
    else{app.setError("UART ocupada. Tente novamente.");return;}
    app.setStatus("Atualizar tela:\nRede CalcTela-xxxx\nSenha: "+std::string(pass.c_str())+"\nAbra 192.168.8.1");
  }else{
    String name="CalcS3-"+String((uint32_t)ESP.getEfuseMac()&0xffff,HEX);
    if(!ota.start(name,pass)){app.setError("Nao abriu OTA. Confira particoes e Wi-Fi.");return;}
    app.setStatus(std::string(name.c_str())+"\nSenha: "+pass.c_str()+"\nAbra 192.168.8.1\nON cancela");
  }
}
inline void serialControl(){
  for(int n=0;n<4096&&Serial.available();n++){
    char c=Serial.read();if(c=='\r')continue;
    if(c!='\n'){if(serialLine.length()<2048)serialLine+=c;else serialLine="";continue;}
    JsonDocument request;auto error=deserializeJson(request,serialLine);serialLine="";if(error)continue;
    JsonDocument reply;reply["id"]=request["id"]|"";String command=request["cmd"]|"";bool ok=true;
    if(command=="configure"){
      String key=request["key"]|"",ssid=request["ssid"]|"",password=request["password"]|"";
      if(!key.isEmpty())ok=deviceconfig::putKey(key);
      if(ok&&!ssid.isEmpty())ok=deviceconfig::putProfile(ssid,password);
      if(request["economy"].is<bool>()){app.economy=request["economy"].as<bool>();deviceconfig::saveSettings(app);}reloadProfiles();
      reply["message"]=ok?"Configuracao salva":"Confira chave, nome e senha; limite de 8 redes";
    }else if(command=="status"){
      reply["firmware"]="calculadora-s3-v1";reply["api_key_set"]=app.hasKey;reply["wifi_connected"]=app.online;reply["filter_selftest"]=filterSelfTest();reply["psram_bytes"]=ESP.getPsramSize();reply["economy"]=app.economy;reply["camera_interface_active"]=network.cameraActive();reply["camera_standby_confirmed"]=network.cameraStandbyConfirmed();
      reply["rgb_off_command"]=keyboard::rgbOffSent?"sent_at_boot":"skipped_gpio48_low";
      reply["camera_focus_mode"]="fixed";
      reply["ota_active"]=ota.active();reply["ota_slot"]=esp_ota_get_running_partition()->label;auto next=esp_ota_get_next_update_partition(nullptr);reply["ota_capacity"]=next?next->size:0;
      JsonArray names=reply["profiles"].to<JsonArray>();for(const auto& p:app.profiles)names.add(p);
    }else if(command=="key"){
      String key=request["key"]|"";if(key.length()>12)ok=false;else if(request["down"]|true)app.press(key.c_str(),millis());else app.release(key.c_str(),millis());
    }else if(command=="portal")startPortal();
    else if(command=="ota"){startUpdate(String(request["target"]|"s3")=="display");reply["setup_text"]=app.status;}
    else ok=false;
    reply["ok"]=ok;ack(reply);
  }
}
inline void effects(){
  for(auto& e:app.effects){
    if(e.kind==calc::EffectKind::UPDATE_S3||e.kind==calc::EffectKind::UPDATE_DISPLAY){startUpdate(e.kind==calc::EffectKind::UPDATE_DISPLAY);continue;}
    if(e.kind==calc::EffectKind::CLEAR_PHOTOS||e.kind==calc::EffectKind::REMOVE_PHOTO){if(!network.busy())network.clearPhotos(e.kind==calc::EffectKind::REMOVE_PHOTO);else app.notice="Aguarde o envio";continue;}
    if(e.kind==calc::EffectKind::CAPTURE_SLOT&&network.busy()){app.photoCapturing=false;app.notice="Aguarde a operacao atual";continue;}
    if(e.kind==calc::EffectKind::VIEW_PHOTO){startPortal(true);continue;}
    if(ota.active()){app.setError("ON encerra atualizacao antes de outra operacao.");continue;}
    if(e.kind==calc::EffectKind::SAVE_SETTINGS){deviceconfig::saveSettings(app);continue;}
    if(e.kind==calc::EffectKind::PORTAL){startPortal();continue;}
    if(e.kind==calc::EffectKind::ADD_WIFI){if(!deviceconfig::putProfile(e.first.c_str(),e.second.c_str()))app.notice="Senha invalida / 8 redes";reloadProfiles();continue;}
    networkservice::Job job{};
    if(e.kind==calc::EffectKind::CONNECT){auto saved=deviceconfig::profiles();if(e.index<0||e.index>=(int)saved.size()){app.setError("Rede nao encontrada.");continue;}job.kind=networkservice::JobKind::CONNECT;strlcpy(job.ssid,saved[e.index].ssid.c_str(),sizeof(job.ssid));strlcpy(job.password,saved[e.index].password.c_str(),sizeof(job.password));}
    else if(e.kind==calc::EffectKind::SCAN_WIFI)job.kind=networkservice::JobKind::SCAN;
    else{job.kind=e.kind==calc::EffectKind::CAPTURE_SLOT?networkservice::JobKind::CAPTURE_SLOT:e.kind==calc::EffectKind::SEND_PHOTOS?networkservice::JobKind::SEND_BATCH:networkservice::JobKind::PHOTO;job.economy=app.economy;job.mirror=app.mirror;job.flip=app.flip;}
    if(!network.enqueue(job))app.setError("Outra operacao de rede esta em andamento. Aguarde e tente de novo.");
  }
  app.effects.clear();
}
inline void updates(){
  networkservice::Update update;
  while(network.poll(update)){
    std::string text=update.text?update.text:"";free(update.text);
    if(update.kind==networkservice::Kind::STATUS)app.setStatus(text);
    else if(update.kind==networkservice::Kind::CONNECTED){app.online=true;app.network=text;app.setStatus("Conectado a "+text+". ON volta a calculadora.");}
    else if(update.kind==networkservice::Kind::ERROR)app.setError(text);
    else if(update.kind==networkservice::Kind::RESPONSE)app.setResponse(text);
    else if(update.kind==networkservice::Kind::PHOTO_SAVED){app.busy=false;app.photoCapturing=false;app.setStatus(text);}
    else if(update.kind==networkservice::Kind::SCANNED){JsonDocument doc;deserializeJson(doc,text);app.scanned.clear();for(JsonVariant v:doc.as<JsonArray>())app.scanned.push_back(v.as<std::string>());app.view=calc::View::SCANNED;app.selected=0;app.dirty=true;}
  }
}
inline void start(){
  Serial.begin(115200);esp_log_level_set("*",ESP_LOG_NONE);uartMutex=xSemaphoreCreateMutex();frameQueue=xQueueCreate(1,sizeof(Frame));
  deviceconfig::seed();deviceconfig::migratePhone();deviceconfig::loadSettings(app);reloadProfiles();WiFi.mode(WIFI_OFF);
  if(LittleFS.begin(true)){const char* saved=LittleFS.exists("/last_response.txt")?"/last_response.txt":"/last_response.bak";File f=LittleFS.open(saved,"r");if(f){String text=f.readString();f.close();if(text.length()<=12000)app.setResponse(text.c_str());}}
  app.view=calc::View::CALCULATOR;app.busy=false;
  if(!filterSelfTest())app.engine.error="Falha no filtro de teclas";
  configureWeb();ota.beginRoutes("s3_calculadora",[](const String& s){app.setStatus(s.c_str());});network.begin();keyboard::begin();xTaskCreatePinnedToCore(frameTask,"display_tx",4096,nullptr,1,nullptr,0);app.dirty=true;
  Preferences p;p.begin("calcprefs",false);bool opened=p.getBool("photo_ui_v1",false);if(!opened)p.putBool("photo_ui_v1",true);p.end();if(!opened)startPortal(true);
}
inline void step(){
  uint32_t now=millis();keyboard::Event event;
  while(xQueueReceive(keyboard::queue,&event,0)==pdTRUE){
    if(event.type==3){app.disconnectedKey();heldArrow=-1;continue;}
    if(event.key<0||event.key>=(int)hp48KeyCount)continue;std::string key=hp48Keys[event.key].key;
    if(event.type==1){app.press(key,event.ms);if(key=="K"||key=="Q"||key=="P"||key=="R"){heldArrow=event.key;repeatAt=now+500;}}
    else{app.release(key,event.ms);if(heldArrow==event.key)heldArrow=-1;}
  }
  if(heldArrow>=0&&(int32_t)(now-repeatAt)>=0){app.handle(hp48Keys[heldArrow].key);repeatAt=now+150;}
  app.tick(now);serialControl();effects();updates();
  unsigned photos=network.photoCount();if(photos!=app.photoCount){app.photoCount=photos;app.dirty=true;}
  if(app.photoCapturing&&!network.busy()){app.photoCapturing=false;app.dirty=true;}
  if(ota.active()&&app.view==calc::View::CALCULATOR)ota.stop();ota.loop();
  if(portalActive){web.handleClient();if((int32_t)(now-portalUntil)>=0){web.stop();WiFi.softAPdisconnect(true);WiFi.mode(WIFI_STA);portalActive=false;portalPassword="";portalToken="";}}
  if(pendingControl&&(int32_t)(now-pendingAt)>=0){int command=pendingControl;pendingControl=0;startUpdate(command==2);}
  if(now-lastStatus>=1000){lastStatus=now;bool connected=WiFi.status()==WL_CONNECTED;if(connected!=app.online){app.online=connected;app.dirty=true;}}
  if((app.dirty||app.photoCapturing)&&now-lastRender>=100){lastRender=now;app.render(now);Frame f;memcpy(f.bytes,app.screen.pixels,sizeof(f.bytes));xQueueOverwrite(frameQueue,&f);}
  delay(2);
}
} // namespace calculator_runtime

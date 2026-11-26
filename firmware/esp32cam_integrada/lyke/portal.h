#pragma once

static const char page[] PROGMEM = R"HTML(
<!doctype html><html lang="pt-BR"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Calc-Lyke</title><style>
body{font:16px system-ui,sans-serif;background:#f3f5ef;color:#18271f;max-width:42rem;margin:auto;padding:1.5rem}
section{background:white;border:1px solid #dce2d9;border-radius:12px;margin:1rem 0;padding:1.25rem}
h1,h2{margin-top:0}input,button,select{font:inherit;padding:.55rem;max-width:100%;box-sizing:border-box}
label{display:block;margin:.6rem 0}button{cursor:pointer}output{display:block;white-space:pre-wrap;overflow-wrap:anywhere}
img{max-width:100%;display:block;margin:.5rem 0}a{color:#205443}p{line-height:1.4}
</style></head><body>
<h1>Calc-Lyke</h1><output id="device">Carregando...</output>
<section><h2>Wi-Fi</h2>
<p>Para economizar bateria, feche este portal depois de configurar. Calc-Lyke desliga após dois minutos sem atividade; CONFIGURAR ou ATUALIZAR no menu da calculadora abre a rede novamente. A conexão ao roteador continua disponível.</p>
<button id="scan">Procurar redes</button> <select id="networks" aria-label="Redes encontradas"></select>
<form id="network"><label>Nome da rede <input name="ssid" id="ssid" maxlength="32" required></label>
<label>Senha <input name="password" type="password" maxlength="63"></label>
<button>Salvar e conectar</button></form>
<p id="connected"></p><label>Rede salva <select id="saved"></select></label>
<button id="join">Conectar</button></section>
<section><h2>Fotos</h2><button id="capture">Tirar foto</button> <button id="refresh">Recarregar lista</button>
<div id="gallery"></div>
<form id="camera"><label><input type="checkbox" name="mirror" value="1"> Espelhar</label>
<label><input type="checkbox" name="flip" value="1"> Inverter na vertical</label>
<label><input type="checkbox" name="focus" value="1"> Autofoco</label>
<button>Salvar câmera</button></form></section>
<section><h2>IA e arquivo de fotos</h2><form id="ai"><label>Endereço do serviço <input name="endpoint" type="url" maxlength="220"></label>
<label>Chave de acesso da calculadora <input name="api" type="password" maxlength="320"></label>
<label><input name="economy" value="1" type="checkbox"> Modo econômico</label>
<button>Salvar configuração</button></form>
<p>Modelo principal: GPT-6.1 Terra. As fotos enviadas para resolução ficam guardadas no arquivo privado do projeto.</p>
<button id="ask">Resolver fotos guardadas</button>
<p id="archive"></p>
<p><a href="/resposta" target="_blank">Abrir última resposta</a></p></section>
<section><h2>Atualizar firmware</h2>
<p>Envie o arquivo fornecido para sua calculadora. Mantenha a alimentação até concluir. Se faltar memória, as fotos temporárias podem ser descartadas; o arquivo da nuvem permanece.</p>
<label>STM32 <input type="file" id="stm" accept=".bin"></label><button id="sendStm">Atualizar STM32</button>
<label>ESP32-CAM <input type="file" id="esp" accept=".bin"></label><button id="sendEsp">Atualizar ESP32-CAM</button>
</section><progress id="uploadProgress" max="100" value="0" hidden style="width:100%"></progress><output id="message" aria-live="polite"></output>
<script>
const $=id=>document.getElementById(id);
let token='', updating=false, settingsLoaded=false;
async function request(path,body,headers={}) {
  const options={cache:'no-store'};
  if(body!==undefined){options.method='POST';options.body=body;options.headers={'X-Update-Token':token,...headers};}
  let r=await fetch(path,options);if(!r.ok)throw Error(await r.text());return r;
}
async function refresh(){
  if(updating)return;
  let info=await (await request('/info')).json();
  token=info.token;
  $('device').textContent='ESP32-CAM '+info.revision+' · '+info.slot+
    '\nSTM32 '+(info.stm_revision||'(versão legada)')+
    '\nTeclas mapeadas: '+info.mapped_keys+(info.keyboard_ready?' (concluído)':'')+
    ' · Fotos: '+info.photos.length+'/3'+
    '\nPSRAM livre: '+Math.round(info.free_psram/1024)+' KiB';
  $('capture').disabled=!info.keyboard_ready;
  $('connected').textContent=info.station?'Conectado a '+info.station:info.wifi_retry_paused?
    'Tentativas pausadas após 20 minutos sem conexão. Escolha uma rede e toque em Conectar.':'Sem conexão à rede externa';
  if(!settingsLoaded){
    $('ai').economy.checked=info.economy;
    $('ai').api.placeholder=info.key_set?'Acesso salvo':'Nenhum acesso salvo';
    $('ai').endpoint.value=info.api_url;
    for(let id of ['mirror','flip','focus'])$('camera')[id].checked=info.camera[id];
    settingsLoaded=true;
  }
  $('ask').disabled=!info.key_set||!info.photos.length;
  $('archive').textContent=info.last_archive_id?'Última consulta arquivada: '+info.last_archive_id:'';
  const previous=$('saved').value;
  $('saved').replaceChildren(...info.saved.map(n=>new Option(n.ssid,n.id)));
  if([...$('saved').options].some(o=>o.value===previous))$('saved').value=previous;
  $('gallery').replaceChildren();
  info.photos.forEach((p,i)=>{
    let a=document.createElement('a');a.href='/foto?id='+p.id;a.target='_blank';
    a.textContent='Abrir foto '+(i+1)+' · '+Math.round(p.bytes/1024)+' KiB';
    $('gallery').append(a,document.createElement('br'));
  });
}
async function action(fn){if(updating)return;try{await fn();}catch(e){$('message').textContent=e.message;}}
$('refresh').onclick=()=>action(refresh);
$('ask').onclick=()=>action(async()=>{$('message').textContent='Resolvendo as fotos. Aguarde...';
  $('message').textContent=await(await request('/api/send','')).text();await refresh();});
$('capture').onclick=()=>action(async()=>{
  $('message').textContent=await(await request('/api/capture','')).text();
  await new Promise(r=>setTimeout(r,2000));
  for(let i=0;i<35;i++){
    try{let info=await(await request('/info')).json();token=info.token;
      if(!info.capture_pending&&!info.capture_busy){$('message').textContent=info.capture_result||'Foto guardada';await refresh();return;}}
    catch(e){}
    await new Promise(r=>setTimeout(r,1000));
  }
  throw Error('Reconecte a Calc-Lyke e recarregue a lista de fotos.');
});
$('network').onsubmit=e=>{e.preventDefault();action(async()=>{await request('/api/network',new URLSearchParams(new FormData(e.target)));
  e.target.password.value='';$('message').textContent='Rede salva. Aguarde a conexão.';await refresh();});};
$('join').onclick=()=>action(async()=>{await request('/api/join',new URLSearchParams({id:$('saved').value}));$('message').textContent='Conectando...';});
$('ai').onsubmit=e=>{e.preventDefault();action(async()=>{await request('/api/ai',new URLSearchParams(new FormData(e.target)));
  e.target.api.value='';$('message').textContent='Configuração salva.';});};
$('camera').onsubmit=e=>{e.preventDefault();action(async()=>{await request('/api/camera',new URLSearchParams(new FormData(e.target)));
  $('message').textContent='Configuração de câmera salva.';});};
$('scan').onclick=()=>action(async()=>{
  await request('/api/scan','');$('message').textContent='Procurando redes...';
  for(let i=0;i<24;i++){
    await new Promise(r=>setTimeout(r,600));
    let results=await(await request('/api/scan')).json();
    if(results.running)continue;
    $('networks').replaceChildren(new Option('Selecione uma rede',''),...results.ssids.map(s=>new Option(s,s)));
    $('message').textContent=results.ssids.length+' rede(s) encontrada(s).';return;
  }throw Error('Busca expirou.');
});
$('networks').onchange=()=>{$('ssid').value=$('networks').value;};
async function upload(chip){
  if(updating)return;
  const file=$(chip).files[0];if(!file)throw Error('Escolha o arquivo de firmware.');
  if(chip==='stm'&&(file.name!=='lyke-stm32-ota.bin'||file.size<28||file.size>49152))throw Error('Use o pacote lyke-stm32-ota.bin desta versão, fornecido para sua unidade.');
  const data=new FormData();data.append('firmware',file);
  updating=true;$('message').textContent='Enviando arquivo...';
  $('uploadProgress').hidden=false;$('uploadProgress').value=0;
  for(const b of document.querySelectorAll('button'))b.disabled=true;
  try{
    if(chip==='esp')await request('/api/ota','');
    const response=await new Promise((resolve,reject)=>{
      const xhr=new XMLHttpRequest();xhr.open('POST',chip==='stm'?'/update/stm32':'/update');xhr.timeout=180000;
      xhr.setRequestHeader('X-Update-Token',token);xhr.setRequestHeader('X-Firmware-Size',String(file.size));
      xhr.upload.onprogress=e=>{if(e.lengthComputable){const n=Math.round(100*e.loaded/e.total);$('uploadProgress').value=n;
        $('message').textContent=n<100?'Enviando arquivo: '+n+'%':'Arquivo enviado. Verificando e instalando. Não desligue.';}};
      xhr.onload=()=>xhr.status===200?resolve(xhr.responseText):reject(Error(xhr.responseText||'Atualização recusada'));
      xhr.onerror=()=>reject(Error('Conexão interrompida. Confira o aparelho antes de repetir.'));
      xhr.ontimeout=()=>reject(Error('A confirmação demorou. Não desligue; confira a versão antes de repetir.'));
      xhr.send(data);
    });
    $('message').textContent=response;
  } catch(e) {
    if(chip==='esp')await request('/api/ota/cancel','').catch(()=>{});
    throw e;
  } finally{updating=false;for(const b of document.querySelectorAll('button'))b.disabled=false;refresh().catch(()=>{});}
}
$('sendStm').onclick=()=>action(()=>upload('stm'));
$('sendEsp').onclick=()=>action(()=>upload('esp'));
action(refresh);
setInterval(()=>refresh().catch(()=>{}),4000);
</script></body></html>
)HTML";

static void describeImage(JsonDocument& doc) {
  const auto* current = esp_ota_get_running_partition();
  const auto* next = esp_ota_get_next_update_partition(nullptr);
  uint8_t digest[32] = {};
  esp_partition_get_sha256(current, digest);
  char hash[65];
  for (unsigned i = 0; i < sizeof(digest); ++i) snprintf(hash + 2 * i, 3, "%02x", digest[i]);
  doc["project"] = projectName;
  doc["revision"] = esp_app_get_description()->version;
  doc["chip"] = ESP.getChipModel();
  doc["slot"] = current->label;
  doc["address"] = current->address;
  doc["capacity"] = next ? next->size : 0;
  doc["flash_bytes"] = ESP.getFlashChipSize();
  doc["image_sha256"] = hash;
  doc["ota_active"] = otaMode;
  doc["wifi_enabled"] = wifiEnabled;
  doc["portal_active"] = portal;
  doc["cpu_mhz"] = getCpuFrequencyMhz();
  doc["reset_reason"] = int(esp_reset_reason());
  doc["wake_reason"] = int(esp_sleep_get_wakeup_cause());
  doc["deep_sleep_supported"] = true;
#ifdef LYKE_SLEEP_SELFTEST
  doc["sleep_selftest"] = true;
#endif
  doc["power_policy"] = "sta-idle-portal-120s-camera-radio-pause";
  doc["wifi_retry_paused"] = radioWatchdog.paused;
  doc["wifi_disconnected_ms"] = radioWatchdog.watching?uint32_t(millis()-radioWatchdog.since):0;
  doc["wifi_retry_limit_ms"] = OfflineRadioWatchdog::limitMs;
  doc["wifi_mode"] = unsigned(WiFi.getMode());
  doc["stm_sleep_minutes"] = stmIdleMinutes;
  doc["stm_inactive_ms"] = stmIdleMs;
  doc["capture_pending"] = capturePendingAt!=0;
  doc["capture_busy"] = captureBusy;
  doc["capture_result"] = captureResult;
  doc["uptime_ms"] = millis();
}

static void routeInfo() {
  if (!local()) { reply(403, "Local AP only"); return; }
  JsonDocument doc;
  describeImage(doc);
  doc["camera_ready"] = ready;
  doc["key_set"] = config.isKey("api");
  doc["api_url"] = config.getString("api_url", defaultApiUrl);
  doc["last_archive_id"] = config.getString("last_req", "");
  doc["economy"] = config.getBool("economy", false);
  doc["camera"]["mirror"] = config.getBool("mirror", mirror);
  doc["camera"]["flip"] = config.getBool("flip", flip);
  doc["camera"]["focus"] = config.getBool("focus", true);
  doc["psram_bytes"] = ESP.getPsramSize();
  doc["free_psram"] = ESP.getFreePsram();
  doc["free_heap"] = ESP.getFreeHeap();
  doc["largest_internal"] = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  doc["ram_photo_bytes"] = photos.ramUsed();
  doc["photo_budget"] = PhotoQueue::ramBudget;
  doc["mapped_keys"] = mappedKeys;
  doc["keyboard_ready"] = keyboardReady;
  doc["stm_revision"] = stmRevision;
  doc["station"] = WiFi.status() == WL_CONNECTED ? WiFi.SSID() : String();
  if(WiFi.status()==WL_CONNECTED) {
    doc["station_ip"]=WiFi.localIP().toString();
    doc["station_rssi"]=WiFi.RSSI();
    doc["station_channel"]=WiFi.channel();
  }
  doc["token"] = sessionToken;
  JsonArray gallery = doc["photos"].to<JsonArray>();
  for (unsigned i = 0; i < photos.count; ++i) {
    JsonObject p = gallery.add<JsonObject>();
    p["id"] = photos.entries[i].id;
    p["bytes"] = photos.entries[i].bytes;
  }
  JsonArray saved = doc["saved"].to<JsonArray>();
  for (unsigned i = 0; i < MaxNetworks; ++i) {
    String name = ssidAt(i);
    if (name.isEmpty()) continue;
    JsonObject entry = saved.add<JsonObject>();
    entry["id"] = i;
    entry["ssid"] = name;
  }
  String json;
  serializeJson(doc, json);
  web.sendHeader("Cache-Control", "no-store");
  web.send(200, "application/json", json);
}

static void photoRoute() {
  if (!local()) { reply(403, "Local AP only"); return; }
  if (!web.hasArg("id")) { web.sendHeader("Location", "/"); web.send(303); return; }
  auto p = photos.get(strtoul(web.arg("id").c_str(), nullptr, 10));
  if (!p || !p->ram) { reply(404, "Foto indisponivel"); return; }
  web.sendHeader("Cache-Control", "no-store");
  web.setContentLength(p->bytes);
  web.send(200, "image/jpeg", "");
  for (size_t offset = 0; offset < p->bytes && web.client().connected();) {
    size_t n = web.client().write(p->ram + offset, min(size_t(4096), p->bytes - offset));
    if (!n) break;
    offset += n;
    delay(0);
  }
}

static void routes() {
  const char* headers[] = { "X-Update-Token", "X-Firmware-Size", "X-Firmware-SHA256" };
  web.collectHeaders(headers, 3);
  web.on("/", HTTP_GET, [] {
    if (!local()) { reply(403, "Local AP only"); return; }
    web.sendHeader("Cache-Control", "no-store");
    web.send_P(200, "text/html; charset=utf-8", page);
  });
  web.on("/info", HTTP_GET, routeInfo);
#ifdef LYKE_SLEEP_SELFTEST
  web.on("/api/selftest/esp-sleep",HTTP_POST,[]{
    if(!authorized()){reply(403,"Unauthorized");return;}
    if(otaMode||captureBusy||capturePendingAt){reply(409,"Busy");return;}
    stmSerial.println("TEST PULSE");testEspSleepAt=millis()+400;
    reply(202,"One-shot diagnostic: STM pulse with timer fallback");
  });
#endif
  web.on("/foto", HTTP_GET, photoRoute);
  web.on("/photo", HTTP_GET, photoRoute);
  web.on("/resposta", HTTP_GET, [] {
    if (!local()) { reply(403, "Local AP only"); return; }
    reply(lastAnswer.length() ? 200 : 404, lastAnswer.length() ? lastAnswer : "Nenhuma resposta guardada");
  });
  web.on("/api/scan", HTTP_POST, [] {
    if (!authorized()) { reply(403, "Unauthorized"); return; }
    bool started = startScan();
    reply(started ? 200 : 409, started ? "Procurando" : "Wi-Fi ocupado");
  });
  web.on("/api/scan", HTTP_GET, [] {
    if (!local()) { reply(403, "Local AP only"); return; }
    JsonDocument doc; doc["running"] = scanActive;
    JsonArray list = doc["ssids"].to<JsonArray>();
    for (unsigned i = 0; i < scannedCount; ++i) list.add(scanned[i]);
    String json; serializeJson(doc, json);
    web.send(200, "application/json", json);
  });
  web.on("/api/network", HTTP_POST, [] {
    if (!authorized()) { reply(403, "Unauthorized"); return; }
    if (otaMode || scanActive) { reply(409, "Wi-Fi ocupado"); return; }
    unsigned index;
    if (!storeNetwork(web.arg("ssid"), web.arg("password"), index)) {
      reply(400, "Nome/senha invalidos, armazenamento indisponivel ou lista cheia"); return;
    }
    radioWatchdog.resume(millis());WiFi.setAutoReconnect(true);
    bool started = join(index);
    reply(started ? 200 : 409, started ? "Conectando" : "Rede salva; conexao indisponivel");
  });
  web.on("/api/join", HTTP_POST, [] {
    if (!authorized()) { reply(403, "Unauthorized"); return; }
    String index = web.arg("id");
    radioWatchdog.resume(millis());WiFi.setAutoReconnect(true);
    bool ok = index.length() == 1 && isDigit(index[0]) && join(index[0] - '0');
    reply(ok ? 200 : 409, ok ? "Conectando" : "Rede indisponivel");
  });
  web.on("/api/ai", HTTP_POST, [] {
    if (!authorized()) { reply(403, "Unauthorized"); return; }
    String key = web.arg("api");
    key.trim();
    String endpoint = web.arg("endpoint");
    endpoint.trim();
    if (endpoint.length()) {
      const int hostEnd = endpoint.indexOf('/', 8);
      const String host = hostEnd > 8 ? endpoint.substring(8, hostEnd) : String();
      if (endpoint.length() > 220 || !endpoint.startsWith("https://") ||
          !host.endsWith(".supabase.co") || host.indexOf('@') >= 0 || host.indexOf(':') >= 0 ||
          !endpoint.endsWith("/functions/v1/lyke-solve") || endpoint.indexOf('\r') >= 0 || endpoint.indexOf('\n') >= 0) {
        reply(400, "Use o endereco HTTPS da funcao lyke-solve no Supabase"); return;
      }
    }
    if (key.length() > 320 || key.indexOf('\r') >= 0 || key.indexOf('\n') >= 0) {
      reply(400, "Chave invalida"); return;
    }
    if (key.length() && config.putString("api", key) != key.length()) {
      reply(500, "Falha salvando configuracao"); return;
    }
    if (endpoint.length() && config.putString("api_url", endpoint) != endpoint.length()) {
      reply(500, "Falha salvando endereco do servico"); return;
    }
    bool saved = config.putBool("economy", web.hasArg("economy")) != 0;
    reply(saved ? 200 : 500, saved ? "Configuracao salva" : "Falha salvando configuracao");
  });
  web.on("/api/capture", HTTP_POST, [] {
    if (!authorized()) { reply(403, "Unauthorized"); return; }
    if(otaMode||scanActive||capturePendingAt||captureBusy){reply(409,"Aguarde a operacao atual");return;}
    if(!keyboardReady||photos.count>=MaxPhotos){reply(409,"Confira teclado e fila de fotos");return;}
    captureResult="";capturePendingAt=millis()+500;
    reply(202,"Capturando. O Wi-Fi pausa e retorna automaticamente; aguarde.");
  });
  web.on("/api/send", HTTP_POST, [] {
    if (!authorized()) { reply(403, "Unauthorized"); return; }
    String result = askAI();
    sendStatus();
    reply(result.isEmpty() ? 200 : 503, result.isEmpty() ? "Resposta pronta. Abra a ultima resposta." : result);
  });
  web.on("/api/camera", HTTP_POST, [] {
    if (!authorized()) { reply(403, "Unauthorized"); return; }
    bool ok = config.putBool("mirror", web.hasArg("mirror")) &&
              config.putBool("flip", web.hasArg("flip")) &&
              config.putBool("focus", web.hasArg("focus"));
    reply(ok ? 200 : 500, ok ? "Camera configurada" : "Falha salvando configuracao");
  });
  web.on("/api/ota", HTTP_POST, [] {
    if (!authorized()) { reply(403, "Unauthorized"); return; }
    otaMode = true;
    cameraStop();
    reply(200, "Atualizacao do ESP pronta");
  });
  web.on("/api/ota/cancel", HTTP_POST, [] {
    if (!authorized()) { reply(403, "Unauthorized"); return; }
    if (ota.receiving || rebootAt) { reply(409, "Atualizacao em andamento"); return; }
    otaMode = false;
    reply(200, "Modo de atualizacao encerrado");
  });
  web.on("/update", HTTP_POST, [] {
    if (!authorized()) { reply(403, "Unauthorized"); return; }
    if (!ota.complete) { reply(400, ota.error.isEmpty() ? "Envio incompleto" : ota.error); return; }
    reply(200, "ESP verificado. Reiniciando.");
    rebootAt = millis() + 1500;
  }, [] {
    auto& upload = web.upload();
    if (rebootAt) return;
    if (upload.status == UPLOAD_FILE_START) ota.start();
    else if (upload.status == UPLOAD_FILE_WRITE) ota.write(upload.buf, upload.currentSize);
    else if (upload.status == UPLOAD_FILE_END) ota.finish();
    else if (upload.status == UPLOAD_FILE_ABORTED) ota.fail("Envio interrompido");
  });
  web.on("/update/stm32", HTTP_POST, [] {
    if (!authorized()) { reply(403, "Unauthorized"); return; }
    bool ok = stmUpdate.finish();
    reply(ok ? 200 : 400, ok ? "STM32 atualizado e reiniciado" :
          stmUpdate.error.isEmpty() ? "Envio incompleto" : stmUpdate.error);
  }, [] {
    auto& upload = web.upload();
    if (upload.status == UPLOAD_FILE_START) {
      bool allowed=authorized() && !rebootAt && !ota.receiving && !ota.complete;
      if (allowed) cameraStop();
      stmUpdate.start(allowed, upload.filename);
      // Preserve the queue unless reclaiming its RAM is actually necessary.
      if (allowed && stmUpdate.error=="PSRAM insuficiente" && photos.count) {
        photos.clear();pendingRequestId="";sendStatus();
        stmUpdate.start(allowed, upload.filename);
      }
    }
    else if (upload.status == UPLOAD_FILE_WRITE) stmUpdate.write(upload.buf, upload.currentSize);
    else if (upload.status == UPLOAD_FILE_ABORTED) stmUpdate.reset();
  });
  web.onNotFound([] { reply(404, "Pagina indisponivel"); });
}

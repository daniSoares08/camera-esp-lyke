#include <Arduino.h>
#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include "credentials.h"

WebServer server(80);
framesize_t stillSize = FRAMESIZE_5MP;
int quality = 10;
bool ready = false;

const char PAGE[] PROGMEM = R"rawliteral(
<!doctype html><html lang="pt-BR"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Controle da câmera ESP32</title><style>
body{font:16px system-ui;max-width:1000px;margin:auto;padding:16px;background:#111827;color:#f3f4f6}h1{font-size:1.5rem}
img{width:100%;max-height:65vh;object-fit:contain;background:#000;border-radius:8px}button,select,input{font:inherit}button,select{padding:8px;border-radius:6px}fieldset{border:1px solid #4b5563;border-radius:8px;margin:12px 0;display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:10px}
label{display:flex;flex-direction:column;gap:4px}.check{display:block}small,#msg{color:#9ca3af}.row{display:flex;gap:10px;align-items:center;flex-wrap:wrap}a{color:#93c5fd}
</style><h1>ESP32-S3-CAM · OV5640</h1>
<div class="row"><button onclick="shot()">Tirar foto 5 MP</button><a id="download" hidden>Baixar última foto</a><span id="msg">Carregando visualização…</span></div>
<p><img id="preview" alt="Prévia da câmera"></p>
<fieldset><legend>Imagem</legend>
<label>Resolução da foto<select data-key="resolution"><option value="5">2592 × 1944 · 5 MP</option><option value="4">2048 × 1536 · 3 MP</option><option value="3">1600 × 1200 · 2 MP</option><option value="2">800 × 600</option><option value="1">640 × 480</option></select></label>
<label>Compressão JPEG <small>Menor = arquivo maior. Q6 é o menor nível validado.</small><input data-key="quality" type="range" min="6" max="63" value="10"><output></output></label>
<label>Brilho<input data-key="brightness" type="range" min="-2" max="2" value="0"><output></output></label>
<label>Contraste<input data-key="contrast" type="range" min="-2" max="2" value="0"><output></output></label>
<label>Saturação<input data-key="saturation" type="range" min="-2" max="2" value="0"><output></output></label>
<label>Nitidez<input data-key="sharpness" type="range" min="-2" max="2" value="0"><output></output></label>
<label>Exposição automática: nível<input data-key="ae" type="range" min="-2" max="2" value="0"><output></output></label>
</fieldset><fieldset><legend>Exposição e cor</legend>
<label class="check"><input data-key="autoExp" type="checkbox" checked> Exposição automática</label>
<label class="check"><input data-key="autoGain" type="checkbox" checked> Ganho automático</label>
<label>Exposição manual (AEC)<small>Desligue exposição automática para ajustar</small><input data-key="exposure" type="range" min="0" max="1200" value="300"><output></output></label>
<label>Ganho manual (aproxima ISO)<small>Desligue ganho automático para ajustar</small><input data-key="gain" type="range" min="0" max="30" value="0"><output></output></label>
<label>Balanço de branco<select data-key="wb"><option value="0">Automático</option><option value="1">Sol</option><option value="2">Nublado</option><option value="3">Escritório</option><option value="4">Casa / lâmpada</option></select></label>
</fieldset><p><small>As alterações são aplicadas ao sensor na hora. Os controles manuais só têm efeito quando o modo automático correspondente está desligado. A resolução da prévia é menor que a foto.</small></p>
<script>
const msg=document.querySelector('#msg'),img=document.querySelector('#preview');let busy=false;
function preview(){if(!busy){img.src='/preview?t='+Date.now()}}img.onerror=()=>{setTimeout(preview,1000)};img.onload=()=>{setTimeout(preview,700)};preview();
document.querySelectorAll('[data-key]').forEach(el=>{const out=el.parentElement.querySelector('output');if(out)out.value=el.value;el.addEventListener('input',()=>{if(out)out.value=el.value});el.addEventListener('change',async()=>{msg.textContent='Aplicando…';const v=el.type==='checkbox'?(el.checked?1:0):el.value;try{const r=await fetch(`/api/control?key=${el.dataset.key}&value=${encodeURIComponent(v)}`);msg.textContent=await r.text();setTimeout(preview,300)}catch(e){msg.textContent='Falha na conexão com a câmera'}})});
async function shot(){busy=true;msg.textContent='Capturando foto…';try{const r=await fetch('/capture?t='+Date.now());if(!r.ok)throw Error();const blob=await r.blob(),a=document.querySelector('#download');a.href=URL.createObjectURL(blob);a.download='esp32-camera.jpg';a.hidden=false;a.textContent=`Baixar foto (${(blob.size/1024).toFixed(0)} KB)`;msg.textContent='Foto pronta · '+(blob.size/1024).toFixed(0)+' KB'}catch(e){msg.textContent='Falha ao capturar'}finally{busy=false;setTimeout(preview,500)}}
</script></html>)rawliteral";

void sendJpeg(framesize_t size, int q) {
  sensor_t *s=esp_camera_sensor_get();
  bool changed=(size!=stillSize || q!=quality);
  if(size!=stillSize) s->set_framesize(s,size);
  if(q!=quality) s->set_quality(s,q);
  if(changed) for(int i=0;i<3;i++){camera_fb_t *old=esp_camera_fb_get();if(old)esp_camera_fb_return(old);delay(100);}
  camera_fb_t *fb=esp_camera_fb_get();
  if(!fb) server.send(503,"text/plain","Falha ao capturar. Tente resolução menor.");
  else {
    WiFiClient client=server.client();
    server.sendHeader("Cache-Control","no-store");
    server.setContentLength(fb->len);
    server.send(200,"image/jpeg","");
    size_t sent=0; uint32_t started=millis();
    while(sent<fb->len && client.connected() && millis()-started<20000) {
      size_t n=client.write(fb->buf+sent,fb->len-sent);
      if(n) sent+=n; else delay(1);
    }
    esp_camera_fb_return(fb);
  }
  if(size!=stillSize) s->set_framesize(s,stillSize);
  if(q!=quality) s->set_quality(s,quality);
  if(size!=stillSize || q!=quality) for(int i=0;i<3;i++){camera_fb_t *old=esp_camera_fb_get();if(old)esp_camera_fb_return(old);delay(100);}
}

void handleControl() {
  if(!server.hasArg("key") || !server.hasArg("value")) { server.send(400,"text/plain","Parâmetro ausente"); return; }
  String key=server.arg("key"); int v=server.arg("value").toInt(), rc=-1; sensor_t *s=esp_camera_sensor_get();
  if(key=="brightness") rc=s->set_brightness(s,constrain(v,-2,2));
  else if(key=="contrast") rc=s->set_contrast(s,constrain(v,-2,2));
  else if(key=="saturation") rc=s->set_saturation(s,constrain(v,-2,2));
  else if(key=="sharpness") rc=s->set_sharpness(s,constrain(v,-2,2));
  else if(key=="ae") rc=s->set_ae_level(s,constrain(v,-2,2));
  else if(key=="autoExp") rc=s->set_exposure_ctrl(s,v!=0);
  else if(key=="autoGain") rc=s->set_gain_ctrl(s,v!=0);
  else if(key=="exposure") rc=s->set_aec_value(s,constrain(v,0,1200));
  else if(key=="gain") rc=s->set_agc_gain(s,constrain(v,0,30));
  else if(key=="wb") { v=constrain(v,0,4); rc=s->set_whitebal(s,v==0); s->set_awb_gain(s,v==0); s->set_wb_mode(s,v); }
  else if(key=="quality") { quality=constrain(v,6,63); rc=s->set_quality(s,quality); }
  else if(key=="resolution") {
    switch(v) { case 5: stillSize=FRAMESIZE_5MP; break; case 4: stillSize=FRAMESIZE_QXGA; break; case 3: stillSize=FRAMESIZE_UXGA; break; case 2: stillSize=FRAMESIZE_SVGA; break; default: stillSize=FRAMESIZE_VGA; }
    rc=s->set_framesize(s,stillSize);
  }
  if(rc<0) server.send(400,"text/plain","Este ajuste não foi aceito pelo sensor");
  else server.send(200,"text/plain","Aplicado");
}

void setup() {
  Serial.begin(115200); delay(500);
  camera_config_t c = {};
  c.ledc_channel=LEDC_CHANNEL_0; c.ledc_timer=LEDC_TIMER_0;
  c.pin_d0=11; c.pin_d1=9; c.pin_d2=8; c.pin_d3=10;
  c.pin_d4=12; c.pin_d5=18; c.pin_d6=17; c.pin_d7=16;
  c.pin_xclk=15; c.pin_pclk=13; c.pin_vsync=6; c.pin_href=7;
  c.pin_sccb_sda=4; c.pin_sccb_scl=5; c.pin_pwdn=-1; c.pin_reset=-1;
  c.xclk_freq_hz=20000000; c.pixel_format=PIXFORMAT_JPEG;
  c.frame_size=stillSize; c.jpeg_quality=quality; c.fb_count=2;
  c.fb_location=CAMERA_FB_IN_PSRAM; c.grab_mode=CAMERA_GRAB_LATEST;
  esp_err_t err=esp_camera_init(&c);
  if(err!=ESP_OK) { Serial.printf("CAMERA_ERROR %08x PSRAM %u\n",err,ESP.getPsramSize()); return; }
  sensor_t *s=esp_camera_sensor_get();
  Serial.printf("CAMERA PID %04x PSRAM %u\n",s->id.PID,ESP.getPsramSize());
  WiFi.mode(WIFI_STA); WiFi.setSleep(false); WiFi.setTxPower(WIFI_POWER_19_5dBm); WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
  Serial.printf("Connecting to Wi-Fi: %s\n",WIFI_SSID);
  for(int i=0;i<60 && WiFi.status()!=WL_CONNECTED;i++) { delay(500); Serial.print('.'); }
  Serial.println();
  if(WiFi.status()!=WL_CONNECTED) { Serial.println("WIFI_FAILED"); return; }
  Serial.print("CAMERA_URL http://"); Serial.println(WiFi.localIP());
  server.on("/",HTTP_GET,[]{server.send_P(200,"text/html; charset=utf-8",PAGE);});
  server.on("/preview",HTTP_GET,[]{sendJpeg(FRAMESIZE_VGA,18);});
  server.on("/capture",HTTP_GET,[]{sendJpeg(stillSize,quality);});
  server.on("/api/control",HTTP_GET,handleControl);
  server.begin(); ready=true; Serial.println("WEB_READY");
}

void loop() { if(ready) server.handleClient(); delay(2); }

#include <Arduino.h>
#include <Preferences.h>
#include "esp_camera.h"
#include "ov5640_af_firmware.h"
#include "esp_system.h"

// Standalone document camera over native USB (S3) or USB/UART (AI Thinker).
int quality=10, brightness=0, contrast=0, saturation=0, sharpness=1;
int aeLevel=0, exposureLines=210, gainValue=2, wbMode=0, exposureCapMs=0, gainMax=8, afMode=1, lensPosition=-1, mirror=1, flip=0;
bool autoExp=true, autoGain=true, ready=false, afReady=false;
bool afCommandOK=false;
bool afFocusConfirmed=false;
int afZoneResult=-1;
uint32_t afRecoveries=0;
const char* afStage="idle";
uint32_t captureCount=0, captureErrors=0;
#if CONFIG_IDF_TARGET_ESP32
const char* firmwareVersion="uart-camera-esp32cam-20261108-r6";
constexpr uint32_t cameraBaud=460800;
#else
const char* firmwareVersion="usb-camera-s3-20261107-r3";
constexpr uint32_t cameraBaud=115200;
#endif
String cameraInitError="NOT_READY";
framesize_t stillSize=FRAMESIZE_QSXGA;
Preferences prefs;
int regRead(int r) { sensor_t*s=esp_camera_sensor_get();return s->get_reg(s,r,255); }
int regWrite(int r,int v) { sensor_t*s=esp_camera_sensor_get();return s->set_reg(s,r,255,v); }
int read16(int r) { int hi=regRead(r),lo=regRead(r+1);return hi<0||lo<0?-1:(hi<<8)|lo; }
int write16(int r,int v) { return regWrite(r,v>>8)|regWrite(r+1,v&255); }
float lineUs() {
 const float preMap[]={1,1,2,3,4,1.5,6,2.5,8};
 int r37=regRead(0x3037),r35=regRead(0x3035),r34=regRead(0x3034),mult=regRead(0x3036),r08=regRead(0x3108),hts=read16(0x380c);
 if(r37<0||r35<0||r34<0||mult<=0||r08<0||hts<=0)return 0;
 int pre=r37&15,root=((r37>>4)&1)+1;
 int sys=r35>>4;if(!sys)sys=16;
 int bits=r34&15,bitDiv=(bits==8||bits==10)?bits/2:4;
 if(pre>8)return 0;
 float clk=20000000.0f/preMap[pre]*mult/sys/root*2/bitDiv/(1<<(r08&3));
 return clk>0?hts*1000000.0f/clk:0;
}
int exposureNow() { return ((regRead(0x3500)&15)<<12)|(regRead(0x3501)<<4)|(regRead(0x3502)>>4); }
float gainNow() { return (((regRead(0x350a)&3)<<8)|regRead(0x350b))/16.0f; }
void discardFrames(int count=2) { for(int i=0;i<count;i++){camera_fb_t*f=esp_camera_fb_get();if(f)esp_camera_fb_return(f);} }
int exposureLimit() {
 float us=lineUs();int vts=read16(0x380e);if(us<=0||vts<=4)return -1;
 return exposureCapMs?constrain((int)(exposureCapMs*1000/us),1,vts-4):vts-4;
}
int applyCap() {
 int cap=exposureLimit();if(cap<1)return -1;
 int rc=write16(0x3a02,cap)|write16(0x3a14,cap);
 sensor_t*s=esp_camera_sensor_get();rc|=s->set_reg(s,0x3a00,4,0);
 rc|=write16(0x3a18,gainMax*16);
 bool builtin=autoExp&&exposureCapMs==0;
 rc|=s->set_exposure_ctrl(s,builtin);rc|=s->set_gain_ctrl(s,autoGain&&(builtin||!autoExp));
 if(autoExp&&!builtin&&exposureNow()>cap){float gx=gainNow()*exposureNow()/cap;rc|=s->set_aec_value(s,cap);if(autoGain)rc|=write16(0x350a,constrain((int)(gx*16),16,gainMax*16));}
 return rc;
}
// The hardware AEC limit registers are not respected by every OV5640 module.
// Capped mode therefore regulates exposure/gain from the sensor light meter,
// with hardware AEC/AGC off; the actual shutter is read back in INFO/META.
void regulate(){
 static uint32_t last=0;if(millis()-last<150)return;last=millis();
 if(!autoExp||exposureCapMs==0)return;
 sensor_t*s=esp_camera_sensor_get();int meter=regRead(0x56a1),cap=exposureLimit();
 if(meter<0||cap<1)return;meter=max(1,meter);
 float factor=constrain((aeLevel+5)*10.0f/meter,0.7f,1.35f);
 if(meter>=(aeLevel+5)*10-3&&meter<=(aeLevel+5)*10+3)return;
 float lines=max(1,exposureNow()),g=max(1.0f,gainNow()),product=lines*g*factor;
 if(autoExp){lines=autoGain?min((float)cap,product):constrain(product/g,1.0f,(float)cap);s->set_aec_value(s,(int)lines);}
 if(autoGain){int raw=constrain((int)(product/lines*16),16,gainMax*16);write16(0x350a,raw);}
}
bool waitReg(int r,int value,uint32_t timeout){uint32_t start=millis();while(millis()-start<timeout){if(regRead(r)==value)return true;delay(5);}return false;}
bool initAF(){
 afReady=false;afCommandOK=false;afFocusConfirmed=false;afZoneResult=-1;afStage="initializing";
 if(esp_camera_sensor_get()->id.PID!=0x5640)return false;
 if(regWrite(0x3000,0x20)<0)return false;
 for(size_t i=0;i<sizeof(ov5640_af_firmware);i++){if(regWrite(0x8000+i,ov5640_af_firmware[i])<0)return false;if((i&127)==0)yield();}
 for(int r=0x3022;r<=0x3028;r++)regWrite(r,0);
 regWrite(0x3029,0x7f);regWrite(0x3000,0);afReady=waitReg(0x3029,0x70,3000);afStage=afReady?"idle":"init_failed";return afReady;
}
int afCommand(int command,uint32_t timeout=2500){
 // CMD_ACK must be armed before EVERY command; do not overwrite a busy mailbox.
 if(!waitReg(0x3023,0,timeout)){afCommandOK=false;afStage="mailbox_busy";return -1;}
 if(regWrite(0x3023,1)<0||regWrite(0x3022,command)<0){afCommandOK=false;afStage="register_error";return -1;}
 afCommandOK=waitReg(0x3023,0,timeout);if(!afCommandOK)afStage="command_timeout";return afCommandOK?0:-1;
}
int focusStart(int mode){
 if(afCommand(0x08)<0)return -1;
 // Single-shot ACK can stay asserted for the entire optical search.
 return afCommand(mode==2?0x04:0x03,15000);
}
int focusMode(int mode){
 afFocusConfirmed=false;afZoneResult=-1;
 if(mode==0)return afReady?afCommand(0x06):0;
 if(!afReady||regRead(0x3023)!=0){afRecoveries++;afReady=initAF();}
 return afReady?focusStart(mode):-1;
}
void focusOnce(){
 afMode=1;afFocusConfirmed=false;afZoneResult=-1;afStage="starting";
 int rc=focusMode(1);
 // One bounded recovery resets only the AF microcontroller, not the ESP/camera.
 if(rc<0){afRecoveries++;afReady=initAF();rc=afReady?focusStart(1):-1;}
 if(rc<0)return;
 afStage="focusing";
 if(!waitReg(0x3029,0x10,12000)){afStage="not_completed";return;}
 // 0x10 means the AF cycle ended. Query the central zone to learn its result.
 if(afCommand(0x07)<0)return;
 afZoneResult=regRead(0x3024);afFocusConfirmed=afZoneResult==0;
 afStage=afZoneResult<0?"register_error":afFocusConfirmed?"focused":"no_focus";
}
void settings(){
 sensor_t*s=esp_camera_sensor_get();s->set_quality(s,quality);s->set_brightness(s,brightness);s->set_contrast(s,contrast);
 s->set_saturation(s,saturation);s->set_sharpness(s,sharpness);s->set_ae_level(s,aeLevel);
 s->set_hmirror(s,mirror);s->set_vflip(s,flip);
 s->set_exposure_ctrl(s,0);s->set_gain_ctrl(s,0);
 s->set_aec_value(s,exposureLines);s->set_agc_gain(s,gainValue);
 s->set_whitebal(s,wbMode==0);s->set_awb_gain(s,1);s->set_wb_mode(s,wbMode);applyCap();
}
void profile(bool save){
 prefs.begin("doccamera",!save);
 if(save){
 prefs.putInt("q",quality);prefs.putInt("br",brightness);prefs.putInt("co",contrast);prefs.putInt("sa",saturation);prefs.putInt("sh",sharpness);prefs.putInt("ae",aeLevel);
 prefs.putInt("cap",exposureCapMs);prefs.putInt("maxg",gainMax);prefs.putInt("af",afMode);prefs.putBool("aexp",autoExp);prefs.putBool("again",autoGain);
 prefs.putInt("exp",exposureLines);prefs.putInt("gain",gainValue);prefs.putInt("wb",wbMode);prefs.putInt("mirror",mirror);prefs.putInt("flip",flip);
 }else{
 quality=prefs.getInt("q",10);brightness=prefs.getInt("br",0);contrast=prefs.getInt("co",0);saturation=prefs.getInt("sa",0);sharpness=prefs.getInt("sh",1);aeLevel=prefs.getInt("ae",0);
 exposureCapMs=prefs.getInt("cap",0);gainMax=prefs.getInt("maxg",8);afMode=prefs.getInt("af",1);autoExp=prefs.getBool("aexp",true);autoGain=prefs.getBool("again",true);
 exposureLines=prefs.getInt("exp",210);gainValue=prefs.getInt("gain",2);wbMode=prefs.getInt("wb",0);mirror=prefs.getInt("mirror",1);flip=prefs.getInt("flip",0);
 }prefs.end();
}
void info(){
 sensor_t*s=esp_camera_sensor_get();
 Serial.printf("INFO {\"pid\":%u,\"quality\":%d,\"brightness\":%d,\"contrast\":%d,\"saturation\":%d,\"sharpness\":%d,\"ae\":%d,\"autoExp\":%d,\"autoGain\":%d,\"exposure\":%d,\"exposure_ms\":%.3f,\"line_us\":%.3f,\"gain_x\":%.3f,\"gainMax\":%d,\"cap_ms\":%d,\"wb\":%d,\"af_ready\":%d,\"af_mode\":%d,\"af_status\":%d,\"meter\":%d,\"lens_position\":%d,\"resolution\":%d,\"mirror\":%d,\"flip\":%d,\"gain\":%d,",
 s->id.PID,quality,brightness,contrast,saturation,sharpness,aeLevel,autoExp,autoGain,exposureNow(),exposureNow()*lineUs()/1000,lineUs(),gainNow(),gainMax,exposureCapMs,wbMode,afReady,afMode,regRead(0x3029),regRead(0x56a1),lensPosition,stillSize==FRAMESIZE_QSXGA?5:stillSize==FRAMESIZE_QXGA?4:stillSize==FRAMESIZE_UXGA?3:stillSize==FRAMESIZE_SVGA?2:1,mirror,flip,gainValue);
 Serial.printf("\"af_focus_confirmed\":%d,\"af_zone_result\":%d,\"af_stage\":\"%s\",\"af_recoveries\":%u,\"af_manual_supported\":false,",afFocusConfirmed,afZoneResult,afStage,afRecoveries);
 Serial.printf("\"firmware\":\"%s\",\"psram_bytes\":%u,\"free_psram\":%u,\"uptime_ms\":%u,\"reset_reason\":%d,\"af_ack\":%d,\"af_command_ok\":%d,\"captures\":%u,\"capture_errors\":%u,\"power_control_gpio\":-1,\"aec_agc_register\":%d}\n",firmwareVersion,ESP.getPsramSize(),ESP.getFreePsram(),millis(),(int)esp_reset_reason(),afReady?regRead(0x3023):-1,afCommandOK,captureCount,captureErrors,regRead(0x3503));
}
void capture(bool fresh,int requestedQuality=-1){
 sensor_t*s=esp_camera_sensor_get();
 if(requestedQuality>=6&&requestedQuality<=63&&requestedQuality!=quality){quality=requestedQuality;s->set_quality(s,quality);discardFrames();}
 uint32_t t=millis();if(fresh)discardFrames(1);camera_fb_t*fb=esp_camera_fb_get();uint32_t waitMs=millis()-t;
 if(!fb){captureErrors++;Serial.println("CAPTURE_ERROR");return;}
 captureCount++;
 Serial.printf("JPEG %u %u %d %u\n",fb->width,fb->height,quality,(unsigned)fb->len);
 float expMs=exposureNow()*lineUs()/1000,gx=gainNow();int af=regRead(0x3029);
 uint32_t txStart=millis(),stalled=txStart;size_t sent=0;
 while(sent<fb->len){size_t chunk=min((size_t)4096,fb->len-sent),n=Serial.write(fb->buf+sent,chunk);if(n){sent+=n;stalled=millis();}else delay(1);if(millis()-stalled>8000)break;yield();}
 Serial.flush();uint32_t sendMs=millis()-txStart;esp_camera_fb_return(fb);
 Serial.printf("\nMETA {\"wait_ms\":%u,\"send_ms\":%u,\"exposure_ms\":%.3f,\"gain_x\":%.3f,\"af_status\":%d,\"sent_bytes\":%u,\"firmware\":\"%s\"}\nDONE\n",waitMs,sendMs,expMs,gx,af,(unsigned)sent,firmwareVersion);Serial.flush();
}
void control(String command){
 int p=command.indexOf(' ',2);if(p<0){Serial.println("ERR format");return;}
 String key=command.substring(2,p);int v=command.substring(p+1).toInt(),rc=-1;sensor_t*s=esp_camera_sensor_get();
 if(key=="brightness"){brightness=constrain(v,-2,2);rc=s->set_brightness(s,brightness);}
 else if(key=="contrast"){contrast=constrain(v,-2,2);rc=s->set_contrast(s,contrast);}
 else if(key=="saturation"){saturation=constrain(v,-2,2);rc=s->set_saturation(s,saturation);}
 else if(key=="sharpness"){sharpness=constrain(v,-3,3);rc=s->set_sharpness(s,sharpness);}
 else if(key=="ae"){aeLevel=constrain(v,-2,2);rc=s->set_ae_level(s,aeLevel);}
 else if(key=="autoExp"){autoExp=v!=0;rc=applyCap();}
 else if(key=="autoGain"){autoGain=v!=0;rc=applyCap();}
 else if(key=="exposure"){exposureLines=constrain(v,1,1964);rc=s->set_aec_value(s,exposureLines);}
 else if(key=="gain"){gainValue=constrain(v,1,30);rc=s->set_agc_gain(s,gainValue);}
 else if(key=="gainMax"){gainMax=constrain(v,1,16);rc=applyCap();}
 else if(key=="cap"){exposureCapMs=constrain(v,0,100);rc=applyCap();}
 else if(key=="af"){afMode=constrain(v,0,2);rc=focusMode(afMode);}
 else if(key=="mirror"){mirror=v!=0;rc=s->set_hmirror(s,mirror);}
 else if(key=="flip"){flip=v!=0;rc=s->set_vflip(s,flip);}
 else if(key=="focusposition"){Serial.println("ERR manual_focus_not_supported");return;}
 else if(key=="wb"){wbMode=constrain(v,0,4);rc=s->set_whitebal(s,wbMode==0);s->set_awb_gain(s,1);s->set_wb_mode(s,wbMode);}
 else if(key=="quality"){quality=constrain(v,6,63);rc=s->set_quality(s,quality);discardFrames();}
 else if(key=="resolution"){
 framesize_t n=v==5?FRAMESIZE_QSXGA:v==4?FRAMESIZE_QXGA:v==3?FRAMESIZE_UXGA:v==2?FRAMESIZE_SVGA:FRAMESIZE_VGA;
 rc=s->set_framesize(s,n);if(rc==0){stillSize=n;afFocusConfirmed=false;afZoneResult=-1;rc=applyCap();if(afReady&&rc==0)rc=afCommand(0x12);}discardFrames();}
 if(rc==0&&key!="af"&&key!="quality"&&key!="resolution")discardFrames();Serial.println(rc<0?(key=="af"?"ERR autofocus_command_not_confirmed":"ERR unsupported"):"OK");
}
void setup(){
 Serial.begin(cameraBaud);Serial.setTimeout(250);delay(300);profile(false);
 if(!psramFound()){cameraInitError="PSRAM_NOT_FOUND";Serial.println("CAMERA_ERROR "+cameraInitError);return;}
 camera_config_t c={};c.ledc_channel=LEDC_CHANNEL_0;c.ledc_timer=LEDC_TIMER_0;
#if CONFIG_IDF_TARGET_ESP32
 // Espressif CameraWebServer CAMERA_MODEL_AI_THINKER pin map.
 c.pin_d0=5;c.pin_d1=18;c.pin_d2=19;c.pin_d3=21;c.pin_d4=36;c.pin_d5=39;c.pin_d6=34;c.pin_d7=35;
 c.pin_xclk=0;c.pin_pclk=22;c.pin_vsync=25;c.pin_href=23;c.pin_sccb_sda=26;c.pin_sccb_scl=27;
 c.pin_pwdn=32;c.pin_reset=-1;
 pinMode(4,OUTPUT);digitalWrite(4,LOW);
#else
 c.pin_d0=11;c.pin_d1=9;c.pin_d2=8;c.pin_d3=10;c.pin_d4=12;c.pin_d5=18;c.pin_d6=17;c.pin_d7=16;
 c.pin_xclk=15;c.pin_pclk=13;c.pin_vsync=6;c.pin_href=7;c.pin_sccb_sda=4;c.pin_sccb_scl=5;
 // Published GOOUUU schematic: PWDN has a pull-down, RESET follows board EN.
 // The 1.2 V / 2.8 V regulators are permanently supplied, with no GPIO enable.
 c.pin_pwdn=-1;c.pin_reset=-1;
#endif
 c.xclk_freq_hz=20000000;c.pixel_format=PIXFORMAT_JPEG;
 c.frame_size=FRAMESIZE_QSXGA;c.jpeg_quality=quality;c.fb_count=2;c.fb_location=CAMERA_FB_IN_PSRAM;c.grab_mode=CAMERA_GRAB_LATEST;
 esp_err_t err=esp_camera_init(&c);if(err!=ESP_OK){cameraInitError=String((uint32_t)err,HEX);Serial.println("CAMERA_ERROR "+cameraInitError);return;}
 // Single focus runs on demand, after the document is positioned and lit.
 ready=true;settings();if(afMode!=0)afReady=initAF();if(afReady&&afMode==2)focusMode(afMode);
 Serial.printf("READY PID %04x PSRAM %u AF %d\n",esp_camera_sensor_get()->id.PID,ESP.getPsramSize(),afReady);
}
void loop(){
 if(ready)regulate();
 if(!Serial.available()){delay(2);return;}
 String c=Serial.readStringUntil('\n');c.trim();
 if(!ready){Serial.println("CAMERA_ERROR "+cameraInitError);return;}
 if(c=="P")capture(true);else if(c=="V")capture(false);else if(c=="I")info();
 else if(c=="S"){profile(true);Serial.println("OK saved");}
 else if(c=="F"){focusOnce();info();}
 else if(c.startsWith("C "))control(c);else if(c.startsWith("Q"))capture(true,c.substring(1).toInt());else Serial.println("ERR unknown command");
}

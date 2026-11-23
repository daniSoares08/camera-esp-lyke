#pragma once
#include <Arduino.h>
#include "esp_camera.h"

class CameraService {
  volatile bool ready=false,standbyConfirmed=false;
public:
  String error;
  bool active()const{return ready;}
  bool standby()const{return standbyConfirmed;}
  void shutdown(){
    if(!ready){digitalWrite(15,LOW);pinMode(15,OUTPUT);return;}
    sensor_t* sensor=esp_camera_sensor_get();standbyConfirmed=false;
    if(sensor&&sensor->id.PID==0x5640){
      // OV5640 SYSTEM_CTROL0[6] is software power-down. Set it while SCCB/XCLK
      // are still available, then release DMA/framebuffers and stop XCLK.
      int before=sensor->get_reg(sensor,0x3008,255);
      if(before>=0&&sensor->set_reg(sensor,0x3008,0x40,0x40)>=0){int after=sensor->get_reg(sensor,0x3008,255);standbyConfirmed=after>=0&&(after&0x40);}
    }
    esp_camera_deinit();ready=false;
    digitalWrite(15,LOW);pinMode(15,OUTPUT); // camera XCLK stays stopped
  }
  bool begin(){
    if(ready)return true;error="";standbyConfirmed=false;
    if(!psramFound()){error="PSRAM ausente. Confira a placa e a compilacao OPI.";return false;}
    camera_config_t c={};c.ledc_channel=LEDC_CHANNEL_0;c.ledc_timer=LEDC_TIMER_0;
    c.pin_d0=11;c.pin_d1=9;c.pin_d2=8;c.pin_d3=10;c.pin_d4=12;c.pin_d5=18;c.pin_d6=17;c.pin_d7=16;
    c.pin_xclk=15;c.pin_pclk=13;c.pin_vsync=6;c.pin_href=7;c.pin_sccb_sda=4;c.pin_sccb_scl=5;
    c.pin_pwdn=-1;c.pin_reset=-1;c.xclk_freq_hz=20000000;c.pixel_format=PIXFORMAT_JPEG;
    c.frame_size=FRAMESIZE_QSXGA;c.jpeg_quality=10;c.fb_count=1;c.fb_location=CAMERA_FB_IN_PSRAM;c.grab_mode=CAMERA_GRAB_WHEN_EMPTY;
    esp_err_t result=esp_camera_init(&c);if(result!=ESP_OK){error="Camera nao iniciou: 0x"+String((unsigned)result,HEX)+". Confira o flat.";return false;}
    ready=true;sensor_t* s=esp_camera_sensor_get();s->set_quality(s,10);s->set_whitebal(s,1);s->set_exposure_ctrl(s,1);s->set_gain_ctrl(s,1);
    return true;
  }
  bool prepareCapture(bool mirror,bool flip){
    if(!begin())return false;sensor_t* s=esp_camera_sensor_get();s->set_hmirror(s,mirror);s->set_vflip(s,flip);
    // Fixed-focus lens: settle exposure/white balance without lens commands.
    for(int i=0;i<3;i++){camera_fb_t* fb=esp_camera_fb_get();if(fb)esp_camera_fb_return(fb);delay(20);}return true;
  }
  camera_fb_t* capture(){camera_fb_t* fb=esp_camera_fb_get();if(!fb||fb->format!=PIXFORMAT_JPEG||fb->len<100||fb->len>1500000){if(fb)esp_camera_fb_return(fb);error="Foto invalida ou grande demais.";return nullptr;}return fb;}
};

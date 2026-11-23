#pragma once
#include <Wire.h>
#include "font5x7.h"

// Exact logical map from ESP-CAM-esquema.html. Physical keyboard order unknown.
class BoardIO {
  // SDK 3.3.6's camera SCCB owns I2C1. Keep the PCF on I2C0, same GPIOs.
  TwoWire bus{0};uint16_t latch=0xffff;
  static constexpr uint16_t cs=1u<<6,reset=1u<<7,dc=1u<<9;
  static constexpr uint8_t rows[4]={5,4,3,2},cols[5]={1,0,15,14,13};
  uint8_t pixels[1024]={};uint32_t scannedAt=0,changedAt=0;uint32_t candidate=0,stable=0;
  bool write(uint16_t next){bus.beginTransmission(0x20);bus.write(next&255);bus.write(next>>8);if(bus.endTransmission()!=0){present=false;return false;}latch=next;return true;}
  bool bits(uint16_t mask,uint16_t value){uint16_t next=(latch&~mask)|(value&mask);return next==latch||write(next);}
  void byte(uint8_t b){for(int bit=7;bit>=0;bit--){digitalWrite(15,LOW);digitalWrite(2,(b>>bit)&1);delayMicroseconds(1);digitalWrite(15,HIGH);delayMicroseconds(1);}digitalWrite(15,LOW);}
  void command(uint8_t b){if(!bits(cs|dc,0))return;byte(b);bits(cs,cs);}
public:
  bool present=false,lcdEnabled=false,batteryEnabled=false;
  uint32_t keys=0;
  bool begin(){bus.begin(13,14,100000);bus.setTimeOut(15);bus.beginTransmission(0x20);present=bus.endTransmission()==0;if(present)write(0xffff);return present;}
  bool displayBegin(){
    if(!present)return false;pinMode(15,OUTPUT);pinMode(2,OUTPUT);digitalWrite(15,LOW);digitalWrite(2,LOW);
    bits(reset,0);delay(5);bits(reset,reset);delay(20);
    // ST7565R: ADC normal, COM reverse, 1/9 bias. Contrast/orientation need bench validation.
    const uint8_t init[]={0xe2,0xa0,0xc8,0xa2,0x2f,0x26,0x81,0x24,0x40,0xa4,0xa6,0xaf};
    for(uint8_t b:init)command(b);lcdEnabled=present;return lcdEnabled;
  }
  void line(unsigned row,const char* text){if(row>=8)return;memset(pixels+row*128,0,128);for(unsigned x=0;text[x]&&x<21;x++){uint8_t c=text[x];if(c<32||c>126)c='?';for(int k=0;k<5;k++)pixels[row*128+x*6+k]=font5x7[c-32][k];}}
  void show(){if(!lcdEnabled||!present)return;for(unsigned page=0;page<8&&present;page++){command(0xb0|page);command(0x10);command(0x00);if(!bits(cs|dc,dc))return;for(unsigned x=0;x<128;x++)byte(pixels[page*128+x]);bits(cs,cs);}}
  uint32_t scan(){
    if(!present||millis()-scannedAt<10)return stable;scannedAt=millis();uint32_t found=0;
    for(unsigned r=0;r<4&&present;r++){
      uint16_t next=latch;for(uint8_t b:rows)next|=1u<<b;for(uint8_t b:cols)next|=1u<<b;next&=~(1u<<rows[r]);
      if(!write(next))break;delayMicroseconds(100);
      if(bus.requestFrom((uint8_t)0x20,(uint8_t)2)!=2){present=false;break;}
      uint16_t value=bus.read();value|=bus.read()<<8;for(unsigned c=0;c<5;c++)if(!(value&(1u<<cols[c])))found|=1u<<(r*5+c);
    }
    uint16_t released=latch;for(uint8_t b:rows)released|=1u<<b;if(present)write(released);
    // No diodes: ignore ambiguous multi-key chords instead of guessing a key.
    if(found&&(found&(found-1)))found=0;
    if(found!=candidate){candidate=found;changedAt=millis();}
    if(millis()-changedAt>=30)stable=candidate;keys=stable;return stable;
  }
  unsigned batteryMillivolts(){if(!batteryEnabled)return 0;pinMode(33,INPUT);analogSetPinAttenuation(33,ADC_11db);uint32_t n=0;for(int i=0;i<8;i++){n+=analogReadMilliVolts(33);delay(2);}return n/4;}
};

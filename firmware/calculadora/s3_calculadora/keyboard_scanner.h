#pragma once
#include <Arduino.h>
#include "driver/gpio.h"
#include "esp32-hal-rgb-led.h"
#include "esp32-hal-rmt.h"
#include "keyboard_map.h"
#include "press_filter.h"
namespace keyboard {
const uint8_t pins[]={0,1,2,3,14,19,20,21,38,39,40,41,42,45,46,47,48};
constexpr int N=sizeof(pins);
struct Event {int16_t key;uint8_t type;uint32_t ms;}; // 1 down, 2 up, 3 ambiguous
QueueHandle_t queue=nullptr;
uint32_t idleLowMask=0,scans=0;
bool rgbOffSent=false;
inline void releasePins(){for(uint8_t pin:pins)pinMode(pin,INPUT_PULLUP);}
inline void turnOffRGB(){
  // GPIO48 also belongs to the keyboard. All contacts must be inputs before
  // the brief LED command; never drive HIGH against the known idle-LOW line.
  releasePins();delay(2);
  if(digitalRead(48)==HIGH){
    rgbLedWrite(48,0,0,0);delayMicroseconds(300);rmtDeinit(48);rgbOffSent=true;
  }
  pinMode(48,INPUT_PULLUP);
}
inline void calibrate(){releasePins();delay(150);idleLowMask=(1UL<<N)-1;for(int s=0;s<64;s++){uint32_t m=0;for(int i=0;i<N;i++)if(digitalRead(pins[i])==LOW)m|=1UL<<i;idleLowMask&=m;delay(2);}}
inline int findPair(int a,int b){
  for(unsigned k=0;k<hp48KeyCount;k++){if(hp48Keys[k].a<0)continue;if((hp48Keys[k].a==a&&hp48Keys[k].b==b)||(b>=0&&hp48Keys[k].a==b&&hp48Keys[k].b==a))return k;}
  if(b==-1){int match=-1;for(int i=0;i<N;i++)if(idleLowMask&(1UL<<i))for(unsigned k=0;k<hp48KeyCount;k++)if((hp48Keys[k].a==a&&hp48Keys[k].b==pins[i])||(hp48Keys[k].b==a&&hp48Keys[k].a==pins[i])){if(match>=0&&match!=(int)k)return -2;match=k;}return match;}
  return -1;
}
inline int scan(){
  bool low[N]={},seen[N][N]={};int grounded=0,ground=-1;delayMicroseconds(600);
  for(int i=0;i<N;i++)if(digitalRead(pins[i])==LOW){low[i]=true;if(!(idleLowMask&(1UL<<i))){grounded++;ground=i;}}
  if(grounded>1)return -2;
  int found=grounded?findPair(pins[ground],-1):-1,count=grounded;
  for(int i=0;i<N;i++){
    if(low[i]||!GPIO_IS_VALID_OUTPUT_GPIO(pins[i]))continue;
    digitalWrite(pins[i],LOW);pinMode(pins[i],OUTPUT_OPEN_DRAIN);delayMicroseconds(500);
    for(int j=0;j<N;j++)if(i!=j&&!low[j]&&digitalRead(pins[j])==LOW){int a=min(i,j),b=max(i,j);if(!seen[a][b]){seen[a][b]=true;int key=findPair(pins[a],pins[b]);if(key>=0){found=key;count++;}}}
    pinMode(pins[i],INPUT_PULLUP);delayMicroseconds(500);
  }
  scans++;return count>1?-2:found;
}
inline void send(uint8_t type,int key,uint32_t now){Event e{(int16_t)key,type,now};xQueueSend(queue,&e,0);}
inline void task(void*){
  PressFilter filter;int held=-1;uint32_t firstRaw=0;int previous=-1;
  for(;;){uint32_t now=millis();int raw=scan();if(raw!=previous){firstRaw=now;previous=raw;}
    auto result=filter.update(raw);
    if(result==PressFilter::DOWN){held=filter.eventCode;send(1,held,firstRaw);}
    if(result==PressFilter::UP){if(held>=0)send(2,held,now);held=-1;}
    if(result==PressFilter::MULTIPLE){send(3,-1,now);held=-1;}
    vTaskDelay(1);
  }
}
inline void begin(){queue=xQueueCreate(24,sizeof(Event));turnOffRGB();calibrate();xTaskCreatePinnedToCore(task,"keyboard",6144,nullptr,2,nullptr,1);}
} // namespace keyboard

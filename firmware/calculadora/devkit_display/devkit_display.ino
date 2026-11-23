#include <Arduino.h>
#include <U8g2lib.h>
#include "link_protocol.h"
#include "wifi_update.h"

// ESP32 DevKit V1 / ESP32-WROOM-32. RW on LCD pin 15 is tied to GND.
// User wiring: LCD DB1=GPIO14 and DB2=GPIO13 (swapped from the first guide).
constexpr uint8_t DB[8]={4,14,13,18,19,21,22,23};
constexpr uint8_t CS1=25,CS2=26,RST=27,RS=32,ENABLE=33;
// The LCD is physically mounted upside down. Rotate local drawings as well.
U8G2_KS0108_128X64_F lcd(U8G2_R2,DB[0],DB[1],DB[2],DB[3],DB[4],DB[5],DB[6],DB[7],ENABLE,RS,CS1,CS2,U8X8_PIN_NONE,RST);
u8x8_msg_cb originalGPIO;
linkproto::Decoder decoder;
uint8_t received[1024];
uint32_t lastFrameAt=0,lastByteAt=0,lastReport=0;
bool hasFrame=false,warning=false;
bool diagnostic=false;
uint32_t diagnosticAt=0;
int lastDiagnostic=-1;
WifiUpdate ota;
void startUpdate(const String& pass){
  const String fixedPassword="CHANGE_ME_ESP_AP";
  diagnostic=false;String name="CalcTela-"+String((uint32_t)ESP.getEfuseMac()&0xffff,HEX);
  if(!ota.start(name,fixedPassword)){Serial.println("OTA_ERROR could not start");return;}
  lcd.clearBuffer();lcd.setDrawColor(1);lcd.setFont(u8g2_font_5x8_tr);
  lcd.drawStr(1,9,"ATUALIZAR TELA");lcd.drawStr(1,22,name.c_str());lcd.drawStr(1,34,("Senha: "+fixedPassword).c_str());lcd.drawStr(1,46,"Abra 192.168.8.1");lcd.drawStr(1,60,"Envie devkit .ino.bin");lcd.sendBuffer();
}

uint8_t displayGPIO(u8x8_t* u,uint8_t msg,uint8_t arg,void* ptr){
  if(msg==U8X8_MSG_GPIO_CS||msg==U8X8_MSG_GPIO_CS1){digitalWrite(msg==U8X8_MSG_GPIO_CS?CS1:CS2,arg?LOW:HIGH);return 1;}
  // Leave generous setup/pulse/hold times for the hand-wired parallel bus.
  if(msg==U8X8_MSG_DELAY_NANO){if(arg)delayMicroseconds(5);return 1;}
  if(msg==U8X8_MSG_GPIO_E){digitalWrite(ENABLE,arg?HIGH:LOW);delayMicroseconds(5);return 1;}
  if(msg==U8X8_MSG_GPIO_RESET){digitalWrite(RST,arg?HIGH:LOW);delay(arg?30:20);return 1;}
  return originalGPIO(u,msg,arg,ptr);
}
void message(const char* a,const char* b,const char* c){lcd.clearBuffer();lcd.setFont(u8g2_font_5x8_tr);lcd.drawStr(1,12,a);lcd.drawStr(1,29,b);lcd.drawStr(1,46,c);lcd.sendBuffer();}
void showReceived(){
  // Raw UART frames bypass U8g2's drawing rotation. In page layout, 180 degrees
  // reverses both the byte order and the vertical bit order within each byte.
  uint8_t* target=lcd.getBufferPtr();
  for(unsigned i=0;i<sizeof(received);i++){
    uint8_t b=received[i];
    b=((b&0x55)<<1)|((b>>1)&0x55);
    b=((b&0x33)<<2)|((b>>2)&0x33);
    b=(b<<4)|(b>>4);
    target[sizeof(received)-1-i]=b;
  }
  lcd.sendBuffer();
}
void initLCD(){
  digitalWrite(ENABLE,LOW);digitalWrite(CS1,HIGH);digitalWrite(CS2,HIGH);
  lcd.begin();digitalWrite(CS1,HIGH);digitalWrite(CS2,HIGH);
}
void diagnosticFrame(int phase){
  lcd.clearBuffer();lcd.setDrawColor(1);lcd.setFont(u8g2_font_5x8_tr);
  if(phase==0){lcd.drawBox(0,0,64,64);lcd.setDrawColor(0);lcd.drawStr(5,32,"ESQUERDA");}
  else if(phase==1){lcd.drawBox(64,0,64,64);lcd.setDrawColor(0);lcd.drawStr(70,32,"DIREITA");}
  else if(phase==2){lcd.drawFrame(0,0,128,64);lcd.drawStr(12,19,"TESTE LCD 128x64");lcd.drawStr(12,34,"DB1=14 DB2=13");lcd.drawStr(12,49,"0123456789 ABC");}
  lcd.setDrawColor(1);lcd.sendBuffer();
  Serial.printf("LCD_TEST phase=%d %s\n",phase,phase==0?"LEFT":phase==1?"RIGHT":phase==2?"TEXT_BORDER":"BLANK");
}
void controlUSB(){
  while(Serial.available()){
    char c=Serial.read();
    if(c=='t'){ota.stop();diagnostic=true;diagnosticAt=millis();lastDiagnostic=-1;initLCD();Serial.println("LCD_TEST enabled; each phase lasts 4s");}
    else if(c=='n'){ota.stop();diagnostic=false;lastDiagnostic=-1;if(hasFrame)showReceived();Serial.println("LCD_TEST disabled; calculator restored");}
    else if(c=='r'){initLCD();if(hasFrame&&!diagnostic)showReceived();lastDiagnostic=-1;Serial.println("LCD_RESET complete");}
    else if(c=='s')Serial.printf("DISPLAY frames=%lu bad_crc=%lu sequence=%u test=%u rotation=180 gpio2_led=off firmware=devkit-display-v5 ota=%u slot=%s\n",(unsigned long)decoder.valid,(unsigned long)decoder.invalid,decoder.sequence,diagnostic,ota.active(),esp_ota_get_running_partition()->label);
  }
}
void setup(){
  // DevKit V1's user LED is active HIGH and separate from the LCD wiring.
  digitalWrite(2,LOW);pinMode(2,OUTPUT);
  Serial.begin(115200);
  WiFi.mode(WIFI_OFF);ota.beginRoutes("devkit_display",[](const String& s){message("ATUALIZACAO",s.c_str(),"Aguarde reiniciar");});
  Serial2.setRxBufferSize(4096);Serial2.begin(115200,SERIAL_8N1,16,17);
  for(uint8_t pin:DB){digitalWrite(pin,LOW);pinMode(pin,OUTPUT);}
  for(uint8_t pin:{CS1,CS2,RST,RS,ENABLE}){digitalWrite(pin,LOW);pinMode(pin,OUTPUT);}
  digitalWrite(CS1,HIGH);digitalWrite(CS2,HIGH);digitalWrite(RST,HIGH);
  originalGPIO=lcd.getU8x8()->gpio_and_delay_cb;lcd.getU8x8()->gpio_and_delay_cb=displayGPIO;
  delay(500);initLCD();
  message("CALCULADORA / TELA","Aguardando ESP32-S3","S3 TX43 -> RX2/GPIO16");
  Serial.println("DISPLAY_READY devkit-display-v5 RX16 TX17 DB1=14 DB2=13 CS_LOW ROT180 GPIO2_LED_OFF OTA; USB: t=test n=normal r=reset s=status");
}
void loop(){
  controlUSB();
  ota.loop();
  bool fresh=false;
  if(lastByteAt&&millis()-lastByteAt>250)decoder.reset();
  for(int count=0;count<8192&&Serial2.available();count++){
    lastByteAt=millis();if(decoder.feed((uint8_t)Serial2.read(),received)){
      if(decoder.type==2){
        const char prefix[]="CALC_OTA_START_V1|";
        if(!memcmp(received,prefix,sizeof(prefix)-1)){
          const char* pass=(const char*)received+sizeof(prefix)-1;bool valid=pass[12]==0;
          for(int i=0;i<12;i++)if(!((pass[i]>='0'&&pass[i]<='9')||(pass[i]>='a'&&pass[i]<='f')))valid=false;
          if(valid)startUpdate(pass);
        }
        fresh=false;
      }else fresh=true;
    }
  }
  if(fresh){if(!diagnostic&&!ota.active())showReceived();hasFrame=true;lastFrameAt=millis();warning=false;}
  if(diagnostic){int phase=((millis()-diagnosticAt)/4000)%4;if(phase!=lastDiagnostic){lastDiagnostic=phase;diagnosticFrame(phase);}}
  if(!diagnostic&&!ota.active()&&hasFrame&&!warning&&millis()-lastFrameAt>10000){message("Conexao com S3 parou","Confira TX43 -> RX16","e o GND comum");warning=true;}
  if(millis()-lastReport>10000){lastReport=millis();Serial.printf("DISPLAY frames=%lu bad_crc=%lu sequence=%u\n",(unsigned long)decoder.valid,(unsigned long)decoder.invalid,decoder.sequence);}
  delay(1);
}

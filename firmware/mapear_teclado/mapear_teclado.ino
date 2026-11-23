#include <Arduino.h>
#include "driver/gpio.h"
#include "press_filter.h"

// USB/UART connector only. Native OTG GPIO19/20 belong to the keyboard.
#if ARDUINO_USB_CDC_ON_BOOT
#error "Compile with CDCOnBoot=default (UART); disconnect the OTG cable."
#endif
const uint8_t pins[] = {0,1,2,3,14,19,20,21,38,39,40,41,42,45,46,47,48};
const int N = sizeof(pins);
const char *version = "hp48-mapper-uart-v3";
const int NONE = -1, AMBIGUOUS = -2;
uint32_t scans = 0, eventId = 0, lastBeat = 0, candidateSince = 0;
int candidate = NONE, stable = NONE;
bool latched = true, enabled = false;
bool lastPairs[N][N] = {};
uint32_t lowMask=0;
uint32_t idleLowMask=0;
String command;
PressFilter pressFilter;
bool filterOK=false;
int histCode[16]={};
uint16_t histCount[16]={};
int histUsed=0;

void releasePins() {
  for (int i=0;i<N;i++) pinMode(pins[i], INPUT_PULLUP);
}

void hello() {
  Serial.printf("{\"type\":\"hello\",\"firmware\":\"%s\",\"transport\":\"UART\",\"pins\":[",version);
  for (int i=0;i<N;i++) Serial.printf("%s%u",i?",":"",pins[i]);
  Serial.printf("],\"uptime_ms\":%lu,\"scans\":%lu,\"enabled\":%s,\"psram_bytes\":%u,\"contacts\":[",
    (unsigned long)millis(),(unsigned long)scans,enabled?"true":"false",ESP.getPsramSize());
  bool comma=false;
  for(int i=0;i<N;i++) {
    if(lowMask & (1UL<<i)) { Serial.printf("%s[%d,-1]",comma?",":"",pins[i]); comma=true; }
    for(int j=i+1;j<N;j++) if(lastPairs[i][j]) {
      Serial.printf("%s[%d,%d]",comma?",":"",pins[i],pins[j]); comma=true;
    }
  }
  Serial.print("],\"idle_low_pins\":[");
  comma=false;
  for(int i=0;i<N;i++) if(idleLowMask & (1UL<<i)) {
    Serial.printf("%s%d",comma?",":"",pins[i]); comma=true;
  }
  Serial.printf("],\"filter_selftest\":%s,\"candidate\":%d,\"candidate_ms\":%lu,\"stable\":%d,\"latched\":%s,\"sample_counts\":[",filterOK?"true":"false",candidate,(unsigned long)(millis()-candidateSince),stable,latched?"true":"false");
  for(int i=0;i<histUsed;i++) Serial.printf("%s[%d,%u]",i?",":"",histCode[i],histCount[i]);
  Serial.println("]}");
  histUsed=0;
}

// No row/column assumptions: one LOW open-drain output, all other pins inputs.
// Closed contacts to GND are supported as well as ordinary matrix pairs.
int scan() {
  bool low[N] = {};
  memset(lastPairs,0,sizeof(lastPairs)); lowMask=0;
  int grounded=0, groundIndex=-1;
  delayMicroseconds(600);
  for(int i=0;i<N;i++) if(digitalRead(pins[i])==LOW) {
    low[i]=true; lowMask |= 1UL<<i;
    // Some board loads (observed on GPIO48) hold a line LOW even at rest.
    // Keep that line high-impedance; only NEW LOW inputs count as a press.
    if(!(idleLowMask & (1UL<<i))) { grounded++; groundIndex=i; }
  }
  if(grounded>1) return AMBIGUOUS;
  int found = grounded ? 512+groundIndex : NONE, count=grounded;
  for(int i=0;i<N;i++) {
    if(low[i] || !GPIO_IS_VALID_OUTPUT_GPIO(pins[i])) continue;
    digitalWrite(pins[i],LOW);
    pinMode(pins[i],OUTPUT_OPEN_DRAIN);
    delayMicroseconds(500);
    for(int j=0;j<N;j++) {
      if(i==j || low[j]) continue;
      if(digitalRead(pins[j])==LOW) {
        int a=min(i,j), b=max(i,j);
        // Scan both directions to support contacts with diodes, counting each
        // physical pair only once, including both directions on GPIO46.
        if(!lastPairs[a][b]) { lastPairs[a][b]=true; found=a*N+b; count++; }
      }
    }
    pinMode(pins[i],INPUT_PULLUP);
    delayMicroseconds(500);
  }
  scans++;
  return count>1 ? AMBIGUOUS : found;
}

void event(const char *type, int code) {
  int a=code>=512 ? code-512 : code/N;
  int b=code>=512 ? -1 : code%N;
  Serial.printf("{\"type\":\"%s\",\"id\":%lu,\"a\":%d,\"b\":%d,\"ms\":%lu}\n",
    type,(unsigned long)++eventId,pins[a],b<0?-1:pins[b],(unsigned long)millis());
}

void commands() {
  while(Serial.available()) {
    char c=Serial.read();
    if(c=='\r') continue;
    if(c!='\n') { if(command.length()<40) command+=c; continue; }
    command.trim();
    if(command=="HELLO") hello();
    else if(command=="START" || command=="STOP") {
      enabled=command=="START" && filterOK;
      pressFilter.reset();
      releasePins(); candidate=stable=NONE; candidateSince=millis(); latched=true;
      hello();
    } else Serial.println("{\"type\":\"error\",\"message\":\"unknown_command\"}");
    command="";
  }
}

void setup() {
  Serial.begin(115200);
  filterOK=filterSelfTest();
  // Disable the native USB Serial/JTAG peripheral before using its two GPIOs.
  releasePins();
  delay(500);
  // Calibrate only once at boot with all keys RELEASED. START never learns a
  // held key as baseline. A grounded baseline pin remains a GND reference;
  // a key connecting another pin to it is emitted as that input <-> GND.
  idleLowMask=(1UL<<N)-1;
  for(int sample=0;sample<64;sample++) {
    uint32_t mask=0;
    for(int i=0;i<N;i++) if(digitalRead(pins[i])==LOW) mask |= 1UL<<i;
    idleLowMask &= mask;
    delay(2);
  }
  hello();
}

void loop() {
  commands();
  if(millis()-lastBeat>=1000) { lastBeat=millis(); hello(); }
  if(!enabled) { delay(2); return; }
  int current=scan();
  int slot=0;
  while(slot<histUsed && histCode[slot]!=current) slot++;
  if(slot==histUsed && histUsed<16) { histCode[slot]=current; histCount[slot]=0; histUsed++; }
  if(slot<16) histCount[slot]++;
  if(current!=candidate) { candidate=current; candidateSince=millis(); }
  auto result=pressFilter.update(current);
  stable=pressFilter.stable; latched=pressFilter.latched;
  if(result==PressFilter::UP) event("up",pressFilter.eventCode);
  else if(result==PressFilter::DOWN) event("down",pressFilter.eventCode);
  else if(result==PressFilter::MULTIPLE) Serial.println("{\"type\":\"ambiguous\"}");
  delay(1);
}

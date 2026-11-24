#pragma once
#include "answer_document.h"
#include <ArduinoJson.h>

static bool loadAnswerDocument(JsonVariantConst value,lyke::AnswerDocument& out) {
  out.count=out.commandCount=0;
  if((value["version"]|0)!=1||!value["blocks"].is<JsonArrayConst>())return false;
  size_t textBytes=0,figures=0;
  for(JsonObjectConst block:value["blocks"].as<JsonArrayConst>()) {
    if(out.count>=32)return false;
    auto& b=out.blocks[out.count++];b={};
    const char *kind=block["kind"]|"";
    if(!strcmp(kind,"text")) {
      b.text=block["text"]|"";textBytes+=strlen(b.text);if(textBytes>12000)return false;
      for(const char *p=b.text;*p;++p)if(((uint8_t)*p<32&&*p!='\n'&&*p!='\r')||(uint8_t)*p>126)return false;
      b.width=126;continue;
    }
    if(strcmp(kind,"drawing")||++figures>6)return false;
    int width=block["width"]|0,height=block["height"]|0;
    if((width!=126&&width!=192&&width!=256)||height<48||height>512||!block["commands"].is<JsonArrayConst>())return false;
    b.drawing=true;b.width=width;b.height=height;b.first=out.commandCount;
    for(JsonObjectConst cmd:block["commands"].as<JsonArrayConst>()) {
      if(out.commandCount>=384||b.count>=128)return false;
      const char *name=cmd["op"]|"";
      static const char *names[]={"line","rect","circle","arc","text","resistor","capacitor","source","arrow"};
      unsigned op=0;while(op<9&&strcmp(name,names[op]))++op;if(op==9)return false;
      unsigned count=op==lyke::D_CIRCLE?3:op==lyke::D_ARC?5:op==lyke::D_TEXT?2:4;
      JsonArrayConst params=cmd["p"].as<JsonArrayConst>();if(params.size()!=count)return false;
      auto& c=out.commands[out.commandCount++];c={};c.op=op;
      for(unsigned i=0;i<count;++i){if(!params[i].is<int>())return false;int n=params[i].as<int>();if(n<-1024||n>1024)return false;c.p[i]=n;}
      c.text=cmd["text"]|"";if(strlen(c.text)>32)return false;
      for(const char *p=c.text;*p;++p)if((uint8_t)*p<32||(uint8_t)*p>126)return false;
      if((op==lyke::D_CIRCLE||op==lyke::D_ARC)&&(c.p[2]<1||c.p[2]>256))return false;
      if(op==lyke::D_ARC&&(abs(c.p[3])>360||abs(c.p[4])>360||abs(c.p[4]-c.p[3])>360))return false;
      if(op==lyke::D_RECT&&(c.p[2]<1||c.p[3]<1||c.p[2]>512||c.p[3]>512))return false;
      ++b.count;
    }
  }
  return textBytes && out.layout();
}

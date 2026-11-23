#pragma once
#include <string>
#include <vector>
#include <stdint.h>
namespace calc {
// Incremental JSON/base64 body: three large JPEGs never require a second,
// expanded copy of all images in PSRAM. Owners are retained by the caller.
class VisionBody {
  struct Part {std::string text;const uint8_t* bytes=nullptr;size_t size=0;};
  std::vector<Part> parts;size_t part=0,offset=0,total=0,used=0;
public:
  void text(std::string s){total+=s.size();parts.push_back({std::move(s),nullptr,0});}
  void image(const uint8_t* p,size_t n){total+=4*((n+2)/3);parts.push_back({{},p,n});}
  size_t length()const{return total;}
  size_t remaining()const{return total-used;}
  int read(){
    static const char table[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    while(part<parts.size()){
      auto& p=parts[part];size_t n=p.bytes?4*((p.size+2)/3):p.text.size();
      if(offset==n){part++;offset=0;continue;}
      int value;
      if(!p.bytes)value=(uint8_t)p.text[offset];
      else{size_t base=(offset/4)*3;unsigned k=offset%4;uint32_t v=(uint32_t)p.bytes[base]<<16;if(base+1<p.size)v|=(uint32_t)p.bytes[base+1]<<8;if(base+2<p.size)v|=p.bytes[base+2];value=(k==2&&base+1>=p.size)||(k==3&&base+2>=p.size)?'=':table[(v>>(18-6*k))&63];}
      offset++;used++;return value;
    }return -1;
  }
};
}

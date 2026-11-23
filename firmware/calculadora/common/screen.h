#pragma once
#include "font5x7.h"
#include <stdint.h>
#include <string.h>
#include <algorithm>
#include <array>
#include <string>
#include <vector>

namespace calc {
inline std::string ascii(const std::string& text){
  std::string normalized=text;
  const std::pair<const char*,const char*> replacements[]={
    {"\xE2\x88\x92","-"},{"\xE2\x80\x93","-"},{"\xE2\x80\x94","-"},
    {"\xCF\x80","pi"},{"\xE2\x88\x9A","sqrt"},{"\xE2\x89\xA4","<="},{"\xE2\x89\xA5",">="},
    {"\xE2\x89\xA0","!="},{"\xE2\x89\x88","~"},{"\xE2\x88\x9E","inf"},
    {"\xC2\xB2","^2"},{"\xC2\xB3","^3"},{"\xC2\xB0","deg"},{"\xC2\xB7","*"}
  };
  for(const auto& replacement:replacements){size_t at=0;while((at=normalized.find(replacement.first,at))!=std::string::npos){normalized.replace(at,strlen(replacement.first),replacement.second);at+=strlen(replacement.second);}}
  std::string out;out.reserve(normalized.size());
  for(size_t i=0;i<normalized.size();i++){
    unsigned char c=normalized[i];
    if(c<128){if(c=='\n'||c=='\t'||c>=32)out+=c;continue;}
    if(c==0xC3&&i+1<normalized.size()){
      unsigned char d=normalized[++i];char a='?';
      if(d>=0x80&&d<=0x85)a='A';else if(d>=0xA0&&d<=0xA5)a='a';
      else if(d==0x87)a='C';else if(d==0xA7)a='c';else if(d>=0x88&&d<=0x8B)a='E';else if(d>=0xA8&&d<=0xAB)a='e';
      else if(d>=0x8C&&d<=0x8F)a='I';else if(d>=0xAC&&d<=0xAF)a='i';else if(d==0x91)a='N';else if(d==0xB1)a='n';
      else if(d>=0x92&&d<=0x96)a='O';else if(d>=0xB2&&d<=0xB6)a='o';else if(d>=0x99&&d<=0x9C)a='U';else if(d>=0xB9&&d<=0xBC)a='u';
      else if(d==0x97)a='*';else if(d==0xB7)a='/';
      out+=a;
    }else{while(i+1<normalized.size()&&((unsigned char)normalized[i+1]&0xC0)==0x80)i++;out+='?';}
  }
  return out;
}
inline std::vector<std::string> wrap(const std::string& utf8,size_t width=21){
  std::string text=ascii(utf8);std::vector<std::string> lines;size_t start=0;
  while(start<text.size()&&lines.size()<700){
    size_t nl=text.find('\n',start),end=std::min(start+width,text.size());
    if(nl!=std::string::npos&&nl<=end){lines.push_back(text.substr(start,nl-start));start=nl+1;continue;}
    if(end<text.size()) {size_t space=text.rfind(' ',end);if(space!=std::string::npos&&space>start)end=space;}
    lines.push_back(text.substr(start,end-start));start=end;while(start<text.size()&&text[start]==' ')start++;
  }
  if(lines.empty())lines.push_back("");
  return lines;
}
class Screen {
public:
  uint8_t pixels[1024]={};
  void clear(){memset(pixels,0,sizeof(pixels));}
  void pixel(int x,int y,bool on=true){if(x<0||x>=128||y<0||y>=64)return;uint8_t& b=pixels[(y/8)*128+x];uint8_t m=1<<(y%8);if(on)b|=m;else b&=~m;}
  void rect(int x,int y,int w,int h,bool on=true){for(int yy=y;yy<y+h;yy++)for(int xx=x;xx<x+w;xx++)pixel(xx,yy,on);}
  void text(int x,int y,const std::string& utf8,bool on=true,int scale=1){
    std::string prepared=utf8;size_t at=0;while((at=prepared.find("\xCF\x80",at))!=std::string::npos){prepared.replace(at,2,1,'\x7f');at++;}
    std::string s=ascii(prepared);
    for(char c:s){bool pi=(unsigned char)c==127;if(!pi&&(c<32||c>126))c='?';for(int col=0;col<5;col++){const uint8_t glyph[]={0x04,0x7c,0x04,0x7c,0x44};uint8_t b=pi?glyph[col]:font5x7[c-32][col];for(int row=0;row<7;row++)if(b&(1<<row))rect(x+col*scale,y+row*scale,scale,scale,on);}x+=6*scale;if(x>=128)break;}
  }
  void header(const std::string& s){rect(0,0,128,9);text(1,1,s.substr(0,21),false);}
  void footer(const std::array<std::string,6>& labels){rect(0,55,128,1);for(int i=0;i<6;i++)text(i*21+1,57,i==4?"MNU":labels[i].substr(0,3));}
  void menu(const std::string& title,const std::vector<std::string>& items,int selected){
    clear();header(title);int first=selected>=5?selected-4:0;
    for(int row=0;row<5&&first+row<(int)items.size();row++){int index=first+row,y=11+row*8;bool chosen=index==selected;if(chosen)rect(0,y-1,128,8);text(1,y,items[index].substr(0,21),!chosen);}
    footer({"UP","DN","","","OK","VOL"});
  }
};
} // namespace calc

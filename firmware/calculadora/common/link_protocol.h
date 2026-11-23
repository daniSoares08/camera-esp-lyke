#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
namespace linkproto {
constexpr size_t SCREEN_BYTES=1024, HEADER_BYTES=10;
inline uint16_t crc16(const uint8_t* data,size_t size,uint16_t crc=0xFFFF){
  while(size--){crc^=(uint16_t)*data++<<8;for(int i=0;i<8;i++)crc=(crc&0x8000)?(uint16_t)((crc<<1)^0x1021):(uint16_t)(crc<<1);}return crc;
}
inline void makeHeader(uint8_t* h,uint16_t sequence,const uint8_t* payload,uint8_t type=1){
  h[0]=0xC3;h[1]=0x3C;h[2]=1;h[3]=type;h[4]=sequence&255;h[5]=sequence>>8;h[6]=0;h[7]=4;
  uint16_t crc=crc16(h+2,6);crc=crc16(payload,SCREEN_BYTES,crc);h[8]=crc&255;h[9]=crc>>8;
}
class Decoder {
  uint8_t bytes[HEADER_BYTES+SCREEN_BYTES];size_t used=0;
public:
  uint32_t valid=0,invalid=0;uint16_t sequence=0;uint8_t type=1;
  void reset(){used=0;}
  bool feed(uint8_t c,uint8_t* screen){
    if(used==0){if(c==0xC3)bytes[used++]=c;return false;}
    if(used==1){if(c==0x3C)bytes[used++]=c;else used=c==0xC3?1:0;return false;}
    bytes[used++]=c;
    if(used==8&&(bytes[2]!=1||(bytes[3]!=1&&bytes[3]!=2)||bytes[6]!=0||bytes[7]!=4)){invalid++;used=c==0xC3?1:0;return false;}
    if(used!=sizeof(bytes))return false;
    uint16_t crc=crc16(bytes+2,6);crc=crc16(bytes+10,SCREEN_BYTES,crc);
    used=0;
    if(crc!=(uint16_t)(bytes[8]|bytes[9]<<8)){invalid++;return false;}
    memcpy(screen,bytes+10,SCREEN_BYTES);sequence=bytes[4]|bytes[5]<<8;type=bytes[3];valid++;return true;
  }
};
} // namespace linkproto

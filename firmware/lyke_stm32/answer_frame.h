#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
    uint8_t data[48][16];
    uint64_t rows;
    unsigned sequence,x,y,width,height,crc;
    bool valid,receiving;
} AnswerFrame;
static uint16_t frame_checksum(const uint8_t *p,unsigned count)
{
    unsigned a=0,b=0;while(count--){a=(a+*p++)%255;b=(b+a)%255;}return (uint16_t)((b<<8)|a);
}
static bool frame_number(const char **p,unsigned *out)
{
    if(**p<'0'||**p>'9')return false;
    *out=0;
    do{if(*out>6553||(*out==6553&&**p>'5'))return false;*out=*out*10+(*(*p)++-'0');}while(**p>='0'&&**p<='9');
    if(**p==' ')++*p;else if(**p)return false;
    return true;
}
static int frame_nibble(unsigned c)
{
    if(c>='0'&&c<='9')return c-'0';
    if(c>='a'&&c<='f')return c-'a'+10;
    return -1;
}
// 0=unrelated/partial, 1=complete, -1=corrupt frame; no heap or unbounded parse.
static int frame_message(AnswerFrame *f,const char *line)
{
    const char *p;unsigned sequence,row;
    if(!strncmp(line,"FRAME ",6)) {
        unsigned x,y,w,h,crc;p=line+6;
        if(!frame_number(&p,&sequence)||!frame_number(&p,&x)||!frame_number(&p,&y)||
           !frame_number(&p,&w)||!frame_number(&p,&h)||!frame_number(&p,&crc)||*p||
           sequence!=f->sequence||w<126||w>256||h<48||x>w-126||y>h-48)return -1;
        f->x=x;f->y=y;f->width=w;f->height=h;f->crc=crc;f->rows=0;f->valid=false;f->receiving=true;
        return 0;
    }
    if(!strncmp(line,"PIX ",4)) {
        p=line+4;if(!frame_number(&p,&sequence)||!frame_number(&p,&row)||sequence!=f->sequence||!f->receiving||row>=48||strlen(p)!=32)return -1;
        uint8_t data[16];
        for(unsigned i=0;i<16;++i){int a=frame_nibble(p[2*i]),b=frame_nibble(p[2*i+1]);if(a<0||b<0)return -1;data[i]=(uint8_t)((a<<4)|b);}
        if(data[15]&3)return -1;
        memcpy(f->data[row],data,16);f->rows|=(uint64_t)1<<row;return 0;
    }
    if(!strncmp(line,"FRAME_END ",10)) {
        p=line+10;if(!frame_number(&p,&sequence)||*p||sequence!=f->sequence||!f->receiving)return -1;
        f->receiving=false;
        f->valid=f->rows==(((uint64_t)1<<48)-1)&&frame_checksum(&f->data[0][0],768)==f->crc;
        return f->valid?1:-1;
    }
    return 0;
}

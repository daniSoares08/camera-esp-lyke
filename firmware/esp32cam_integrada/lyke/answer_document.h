#pragma once
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "font5x7.h"
#include "answer_pager.h"

namespace lyke {
enum DrawOp { D_LINE, D_RECT, D_CIRCLE, D_ARC, D_TEXT, D_RESISTOR, D_CAPACITOR, D_SOURCE, D_ARROW };
struct DrawCommand { int16_t p[5]; uint8_t op; const char *text; };
struct AnswerBlock { const char *text; uint16_t width,height,first,count; bool drawing; };

// Fixed-size display list; text pointers refer to the retained, validated JSON.
struct AnswerDocument {
  AnswerBlock blocks[32]{}; DrawCommand commands[384]{};
  unsigned count=0,commandCount=0,height=48; uint16_t origin[32]{};
  template<class F> static unsigned lines(const char *text,F emit) {
    unsigned n=0;const char *p=text?text:"",*limit=p+answer_content_length(p,strlen(p));
    while(p<limit) {
      while(p<limit&&(*p=='\n'||*p=='\r'||*p==' '||*p=='\t'))++p;
      if(p==limit)break;
      unsigned used=0,space=0;while(p+used<limit&&p[used]!='\n'&&p[used]!='\r'&&used<21){if(p[used]==' ')space=used;++used;}
      if(used==21&&p+used<limit&&p[used]!='\n'&&p[used]!='\r'&&p[used]!=' '&&space)used=space;
      char row[22]{};memcpy(row,p,used);emit(n++,row);p+=used;
    }
    return n;
  }
  bool layout() {
    unsigned y=0;
    if(!count||count>32||commandCount>384)return false;
    for(unsigned i=0;i<count;++i) {
      origin[i]=y;
      y+=blocks[i].drawing?blocks[i].height:lines(blocks[i].text,[](unsigned,const char*){})*8;
      if(y>65535)return false;
    }
    height=y<48?48:y;return true;
  }
  void plain(const char *text) {
    count=1;commandCount=0;blocks[0]={text,126,0,0,0,false};layout();
  }
  unsigned widthAt(unsigned y) const {
    unsigned width=126;
    for(unsigned i=0;i<count;++i)if(blocks[i].drawing&&origin[i]<y+48&&origin[i]+blocks[i].height>y)
      if(blocks[i].width>width)width=blocks[i].width;
    return width;
  }
};

struct AnswerRaster {
  uint8_t pixels[48][16]{};
  int x=0,y=0,clipWidth=126,clipTop=0,clipBottom=65535;
  void dot(int xx,int yy) {
    if(xx<0||xx>=clipWidth||yy<clipTop||yy>=clipBottom)return;
    xx-=x;yy-=y;if(xx>=0&&xx<126&&yy>=0&&yy<48)pixels[yy][xx/8]|=uint8_t(0x80u>>(xx%8));
  }
  void line(int a,int b,int c,int d) {
    int dx=abs(c-a),sx=a<c?1:-1,dy=-abs(d-b),sy=b<d?1:-1,e=dx+dy;
    for(;;){dot(a,b);if(a==c&&b==d)break;int ee=2*e;if(ee>=dy){e+=dy;a+=sx;}if(ee<=dx){e+=dx;b+=sy;}}
  }
  void text(int xx,int yy,const char *p) {
    for(;p&&*p;++p,xx+=6){unsigned c=(uint8_t)*p;if(c<32||c>126)c='?';
      for(unsigned col=0;col<5;++col)for(unsigned row=0;row<7;++row)if(font5x7[c-32][col]&(1u<<row))dot(xx+col,yy+row);}
  }
  void circle(int xx,int yy,int r) {
    int a=r,b=0,e=1-r;
    while(a>=b){dot(xx+a,yy+b);dot(xx+b,yy+a);dot(xx-b,yy+a);dot(xx-a,yy+b);
      dot(xx-a,yy-b);dot(xx-b,yy-a);dot(xx+b,yy-a);dot(xx+a,yy-b);
      ++b;if(e<0)e+=2*b+1;else{--a;e+=2*(b-a)+1;}}
  }
  void component(const DrawCommand& c,int top) {
    float ax=c.p[0],ay=c.p[1]+top,dx=c.p[2]-ax,dy=c.p[3]+top-ay;
    float length=sqrtf(dx*dx+dy*dy);if(length<1)return;
    float nx=-dy/length,ny=dx/length;
    auto segment=[&](float t,float offset,float u,float offset2){
      line(lroundf(ax+t*dx+offset*nx),lroundf(ay+t*dy+offset*ny),lroundf(ax+u*dx+offset2*nx),lroundf(ay+u*dy+offset2*ny));};
    if(c.op==D_RESISTOR) {
      segment(0,0,.2f,0);float t=.2f,off=0;
      for(unsigned i=0;i<6;++i){float next=.25f+.1f*i,nextOff=i&1?-4:4;segment(t,off,next,nextOff);t=next;off=nextOff;}
      segment(t,off,.8f,0);segment(.8f,0,1,0);
    } else if(c.op==D_CAPACITOR) {
      segment(0,0,.45f,0);segment(.55f,0,1,0);segment(.45f,-7,.45f,7);segment(.55f,-7,.55f,7);
    } else if(c.op==D_SOURCE) {
      float radius=length<30?length/4:10,dt=radius/length;
      segment(0,0,.5f-dt,0);segment(.5f+dt,0,1,0);
      int cx=lroundf(ax+dx*.5f),cy=lroundf(ay+dy*.5f);circle(cx,cy,lroundf(radius));
      int px=lroundf(cx-dx/length*radius*.4f),py=lroundf(cy-dy/length*radius*.4f);
      int mx=lroundf(cx+dx/length*radius*.4f),my=lroundf(cy+dy/length*radius*.4f);
      line(px-2,py,px+2,py);line(px,py-2,px,py+2);line(mx-2,my,mx+2,my);
    } else {
      segment(0,0,1,0);float t=length>8?1-8/length:0;segment(t,4,1,0);segment(t,-4,1,0);
    }
  }
  void command(const DrawCommand& c,int top) {
    int a=c.p[0],b=c.p[1]+top;
    switch(c.op) {
    case D_LINE:line(a,b,c.p[2],c.p[3]+top);break;
    case D_RECT:line(a,b,a+c.p[2],b);line(a,b,a,b+c.p[3]);line(a+c.p[2],b,a+c.p[2],b+c.p[3]);line(a,b+c.p[3],a+c.p[2],b+c.p[3]);break;
    case D_CIRCLE:circle(a,b,c.p[2]);break;
    case D_TEXT:text(a,b,c.text);break;
    case D_ARC:{
      int from=c.p[3],to=c.p[4],direction=from<=to?1:-1;
      float radians=from*.01745329252f;int xx=a+lroundf(c.p[2]*cosf(radians)),yy=b-lroundf(c.p[2]*sinf(radians));
      while(from!=to){int step=abs(to-from)<3?abs(to-from):3;from+=direction*step;radians=from*.01745329252f;
        int nextX=a+lroundf(c.p[2]*cosf(radians)),nextY=b-lroundf(c.p[2]*sinf(radians));line(xx,yy,nextX,nextY);xx=nextX;yy=nextY;}
      break;}
    default:component(c,top);break;
    }
  }
  void render(const AnswerDocument& doc,unsigned left,unsigned top) {
    memset(pixels,0,sizeof(pixels));x=left;y=top;
    for(unsigned i=0;i<doc.count;++i) {
      const auto& block=doc.blocks[i];unsigned at=doc.origin[i];
      clipWidth=block.drawing?block.width:126;clipTop=at;
      clipBottom=block.drawing?at+block.height:65535;
      if(block.drawing) {
        if(at>=top+48||at+block.height<=top)continue;
        for(unsigned j=0;j<block.count;++j)command(doc.commands[block.first+j],at);
      } else AnswerDocument::lines(block.text,[&](unsigned row,const char *s){if(at+row*8+7>=top&&at+row*8<top+48)text(0,at+row*8,s);});
    }
  }
};
}

#include <cassert>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "../../esp32cam_integrada/lyke/answer_json.h"
#include "../answer_frame.h"
#include "../../esp32cam_integrada/lyke/power_policy.h"

static bool bit(const lyke::AnswerRaster& r,unsigned x,unsigned y){return r.pixels[y][x/8]&(0x80u>>(x%8));}
static void save(const char *folder,const char *name,const lyke::AnswerRaster& r){
    std::ofstream f(std::string(folder)+"/"+name+".pbm");assert(f);f<<"P1\n126 48\n";
    for(unsigned y=0;y<48;++y){for(unsigned x=0;x<126;++x)f<<bit(r,x,y)<<' ';f<<'\n';}
}
int main(int argc,char **argv){
    assert(argc==3);std::ifstream input(argv[1]);std::stringstream bytes;bytes<<input.rdbuf();
    JsonDocument json;assert(!deserializeJson(json,bytes.str()));
    lyke::AnswerDocument doc;assert(loadAnswerDocument(json.as<JsonVariantConst>(),doc));
    assert(doc.count==6&&doc.commandCount==24&&doc.height==384);
    lyke::AnswerRaster raster,shifted;
    std::vector<uint8_t> full(192*doc.height);
    for(unsigned top=0;top<doc.height;top+=48)for(unsigned left=0;left<192;left+=126){
      raster.render(doc,left,top);
      for(unsigned y=0;y<48&&top+y<doc.height;++y)for(unsigned x=0;x<126&&left+x<192;++x)
        full[(top+y)*192+left+x]=bit(raster,x,y);}
    {std::ofstream f(std::string(argv[2])+"/resposta_completa.pbm");assert(f);f<<"P1\n192 "<<doc.height<<"\n";
      for(unsigned y=0;y<doc.height;++y){for(unsigned x=0;x<192;++x)f<<unsigned(full[y*192+x])<<' ';f<<'\n';}}
    raster.render(doc,0,24);assert(bit(raster,16,16)&&bit(raster,32,16));save(argv[2],"circuito_esquerda",raster);
    shifted.render(doc,66,24);assert(bit(shifted,94,16));save(argv[2],"circuito_direita",shifted);
    for(unsigned y=0;y<48;++y)for(unsigned x=66;x<126;++x)assert(bit(raster,x,y)==bit(shifted,x-66,y));
    raster.render(doc,0,144);save(argv[2],"grafico_topo",raster);
    raster.render(doc,66,224);save(argv[2],"grafico_base",raster);
    raster.render(doc,0,272);save(argv[2],"angulo_topo",raster);
    raster.render(doc,0,320);save(argv[2],"angulo_base",raster);
    for(unsigned row=0;row<48;++row)assert(!(raster.pixels[row][15]&3));
    // Exercise the exact wire receiver against a renderer-produced viewport.
    AnswerFrame frame{};frame.sequence=9;char line[96];
    snprintf(line,sizeof(line),"FRAME 9 0 320 126 384 %u",frame_checksum(&raster.pixels[0][0],768));assert(!frame_message(&frame,line));
    for(unsigned row=0;row<48;++row){unsigned at=snprintf(line,sizeof(line),"PIX 9 %u ",row);
      for(unsigned x=0;x<16;++x)snprintf(line+at+2*x,3,"%02x",raster.pixels[row][x]);assert(!frame_message(&frame,line));}
    assert(frame_message(&frame,"FRAME_END 9")==1&&!memcmp(frame.data,raster.pixels,768));
    json["blocks"][1]["commands"][0]["op"]="eval";assert(!loadAnswerDocument(json.as<JsonVariantConst>(),doc));
    assert(!deserializeJson(json,bytes.str()));json["blocks"][4]["commands"][2]["p"][4]=10000;assert(!loadAnswerDocument(json.as<JsonVariantConst>(),doc));
    doc.plain("2*x+3=11\nx=4");raster.render(doc,0,0);save(argv[2],"texto_legado",raster);
    assert(doc.height==48&&doc.widthAt(0)==126);
    assert(lyke::AnswerDocument::lines("x=4\n\nTokens entrada: 9179\nTokens saida: 2616",[](unsigned,const char *s){assert(!strcmp(s,"x=4"));})==1);
    assert(lyke::AnswerDocument::lines("Tokens entrada: dados\nEsta linha e conteudo",[](unsigned,const char*){})==2);
    std::vector<std::string> rows;
    lyke::AnswerDocument::lines("phi=36.87 deg\npi=3.14159\nsqrt(36^2+27^2)=45 ohm\npilha principal\n\n2b) Corrente",[&](unsigned,const char* s){rows.emplace_back(s);});
    assert(uint8_t(rows[0][0])==lyke::AC_PHI&&uint8_t(rows[1][0])==lyke::AC_PI);
    assert(rows.size()==6&&rows[4].empty()&&rows[3]=="pilha principal");
    assert(uint8_t(rows[2][0])==lyke::AC_ROOT&&rows[2].find("45")!=std::string::npos);
    rows.clear();lyke::AnswerDocument::lines("|S1|=3.53553*0.0113659",[&](unsigned,const char*s){rows.emplace_back(s);});
    assert(rows.size()==2&&rows[0]=="|S1|=3.53553"&&rows[1]=="*0.0113659");
    assert(memcmp(lyke::answerGlyph(lyke::AC_PI),lyke::answerGlyph(lyke::AC_PHI),5));
    lyke::AnswerDocument::lines("phi_T=29.51 deg",[](unsigned,const char*s){assert(uint8_t(s[0])==lyke::AC_PHI&&!strncmp(s+1,"_T=29.51",8));});
    doc.plain("phi=36.87 deg\npi=3.14159\nC=1/(2*pi*f*Xc)\nsqrt(36^2+27^2)\nZ=45 angle 36.87 deg\nZ=36+j27 ohm");
    raster.render(doc,0,0);save(argv[2],"simbolos_resposta",raster);
    doc.plain("2a) Impedancia\nZ=12+j9 ohm\n|Z|=15 ohm\n\n2b) Corrente\nI=220/15=14.67 A");
    raster.render(doc,0,0);save(argv[2],"separacao_resposta",raster);
    PortalLease lease;lease.touch(100);
    assert(!lease.expired(120099,false)&&lease.expired(120100,false));
    assert(!lease.expired(999999,true));lease.close();assert(!lease.expired(999999,false));
    lease.touch(UINT32_MAX-100);assert(!lease.expired(50,false)&&lease.expired(120000,false));
    raster={};raster.command(lyke::DrawCommand{{0,20,60,20,0},lyke::D_SOURCE,""},0);
    assert(bit(raster,26,18)&&!bit(raster,34,18));
    puts("Actual rich-document renderer: circuit/plot/angle, inline text, pan, legacy fallback, bounds and STM wire roundtrip OK");
}

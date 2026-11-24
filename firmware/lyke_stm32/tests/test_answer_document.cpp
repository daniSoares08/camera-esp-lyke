#include <cassert>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "../../esp32cam_integrada/lyke/answer_json.h"
#include "../answer_frame.h"

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
    raster={};raster.command(lyke::DrawCommand{{0,20,60,20,0},lyke::D_SOURCE,""},0);
    assert(bit(raster,26,18)&&!bit(raster,34,18));
    puts("Actual rich-document renderer: circuit/plot/angle, inline text, pan, legacy fallback, bounds and STM wire roundtrip OK");
}

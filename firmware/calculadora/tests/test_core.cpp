#include "../common/calculator_app.h"
#include "../common/link_protocol.h"
#include "../common/api_contract.h"
#include "../common/press_filter.h"
#include "../common/camera_session.h"
#include "../common/vision_body.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <filesystem>

using namespace calc;
static bool close(double a,double b){return std::fabs(a-b)<=1e-10*std::max(1.0,std::fabs(b));}
static void expect(const std::string& expression,double value,bool degrees=true){std::array<double,6> m{};auto r=Parser(expression,degrees,7,m).run();if(!r.ok||!close(r.value,value)){std::cerr<<expression<<" = "<<r.value<<" "<<r.error<<" expected "<<value<<"\n";std::abort();}}
static void bad(const std::string& s){std::array<double,6> m{};assert(!Parser(s,true,0,m).run().ok);}
static void pbm(const std::string& path,const Screen& s){std::ofstream out(path,std::ios::binary);out<<"P4\n128 64\n";for(int y=0;y<64;y++)for(int b=0;b<16;b++){uint8_t value=0;for(int bit=0;bit<8;bit++)if(s.pixels[(y/8)*128+b*8+bit]&(1<<(y%8)))value|=1<<(7-bit);out.put((char)value);}}
int main(int argc,char** argv){
  assert(filterSelfTest());
  assert(ascii("\xCF\x80 \xC3\x97 2 \xE2\x89\xA5 6")=="pi * 2 >= 6");
  struct FakeCamera {bool active=true;int stops=0;void shutdown(){active=false;stops++;}};
  FakeCamera camera;{CameraSession<FakeCamera> session(camera);session.close();assert(!camera.active&&camera.stops==1);}assert(camera.stops==1);
  FakeCamera failedCamera;{CameraSession<FakeCamera> failedCapture(failedCamera);}assert(!failedCamera.active&&failedCamera.stops==1);
  expect("2+3*4",14);expect("(2+3)*4",20);expect("2^3^2",512);expect("-2^2",-4);expect("(-2)^2",4);expect("2^-3",0.125);
  expect("sin(30)",0.5);expect("cos(60)",0.5);expect("asin(0.5)",30);expect("tan(45)",1);expect("sin(pi/2)",1,false);
  expect("sqrt(144)+log(100)+ln(e)",15);expect("5!",120);expect("20%",0.2);expect("1.25E-3*800",1);expect("ans*2",14);
  bad("1/0");bad("sqrt(-1)");bad("log(0)");bad("asin(2)");bad("tan(90)");bad("171!");bad("3.5!");bad("1E309");bad("2+");bad("(2+3");bad("foo(2)");bad("2 3");
  Engine e;e.key("2");e.key("ADD");e.key("3");e.key("MUL");e.key("4");e.key("Y");assert(close(e.answer,14));e.key("ADD");e.key("1");e.key("Y");assert(close(e.answer,15));
  e.reset();e.key("6");e.key("0");e.key("T");assert(close(e.answer,0.5));
  e.reset();e.key("T");e.key("6");e.key("0");e.key("B");e.key("Y");assert(close(e.answer,0.5));
  e.reset();e.key("1");e.key("EEX");e.key("3");e.key("Z");e.key("Y");assert(close(e.answer,0.001));e.key("Z");e.key("Y");assert(close(e.answer,-0.001));
  e.reset();e.rpn=true;e.key("2");e.key("Y");e.key("3");e.key("ADD");assert(close(e.answer,5));e.key("Y");e.key("MUL");assert(close(e.answer,25));assert(e.stack.size()==1);
  assert(e.store(5));e.reset();e.recall(5);assert(close(e.answer,25));
  Engine tokens;tokens.function("tan");tokens.key("BACKSPACE");assert(tokens.input.empty());
  tokens.insert("12+sqrt(");tokens.key("BACKSPACE");assert(tokens.input=="12+");tokens.insert("pow10(");tokens.cursor-=2;tokens.key("BACKSPACE");assert(tokens.input=="12+");
  tokens.clear();tokens.key("C");assert(tokens.input=="pi");tokens.key("Y");assert(close(tokens.answer,kPi));tokens.key("BACKSPACE");
  App a;a.engine.input="2+3";a.engine.cursor=3;a.press("Y",100);assert(close(a.engine.answer,5));a.tick(5000);assert(a.view==View::CALCULATOR);a.release("Y",5100);a.press("E",5200);assert(a.view==View::MAIN);
  a.handle("ON");a.press("SPC",5300);assert(a.photoCapturing&&a.effects.back().kind==EffectKind::CAPTURE_SLOT);auto count=a.effects.size();a.press("SPC",5400);assert(a.effects.size()==count);a.photoCapturing=false;a.photoCount=3;a.handle("SPC");assert(a.effects.size()==count);a.handle("E");a.selected=1;a.handle("Y");assert(a.effects.back().kind==EffectKind::SEND_PHOTOS);
  App headerApp;headerApp.render(0);assert(headerApp.screen.pixels[3]==0);headerApp.online=true;headerApp.photoCount=1;headerApp.photoCapturing=true;headerApp.render(0);auto bright=headerApp.screen.pixels[112];headerApp.render(250);assert(bright!=headerApp.screen.pixels[112]);assert(headerApp.screen.pixels[104]&4);
  Screen piGlyph;piGlyph.text(0,0,"π");assert(piGlyph.pixels[0]==4&&piGlyph.pixels[1]==0x7c);
  a.setResponse("TRANSCRICAO\n2 + 3\n\nCALCULO\n2 + 3 = 5\n\nRESULTADO\n5\nUma explicacao suficientemente longa para rolar por varias linhas.");a.handle("Q");assert(a.scroll==1);a.handle("R");assert(a.scroll>1);a.handle("P");assert(a.scroll>=0);
  std::string previousAnswer=a.response;a.setError("Rede indisponivel");assert(a.response==previousAnswer&&a.view==View::BUSY&&!a.busy);
  App program;program.handle("H");program.handle("Y");assert(program.recording);for(const char* key:{"2","ADD","3","Y"})program.handle(key);assert(close(program.engine.answer,5));program.handle("H");program.handle("Y");assert(!program.recording);program.engine.reset();program.handle("H");program.handle("Q");program.handle("Y");assert(close(program.engine.answer,5));
  App w;w.profiles={"Casa","Telefone"};w.handle("E");w.handle("Y");assert(w.view==View::WIFI);w.handle("Q");w.handle("Q");w.handle("Y");assert(w.view==View::TEXT);w.handle("Y");w.handle("F");w.handle("1");w.handle("2");w.handle("F");assert(w.effects.back().kind==EffectKind::ADD_WIFI&&w.effects.back().first=="a"&&w.effects.back().second=="12");
  uint8_t payload[1024],output[1024],header[10];for(int i=0;i<1024;i++)payload[i]=(uint8_t)(i*73);linkproto::makeHeader(header,42,payload);linkproto::Decoder d;for(uint8_t c:{'b','o','o','t','\n'})d.feed(c,output);for(auto c:header)assert(!d.feed(c,output));for(int i=0;i<1024;i++){bool ok=d.feed(payload[i],output);assert(ok==(i==1023));}assert(d.sequence==42&&!memcmp(payload,output,1024));
  header[9]^=1;for(auto c:header)d.feed(c,output);for(auto c:payload)assert(!d.feed(c,output));assert(d.invalid==1);linkproto::makeHeader(header,43,payload);for(auto c:header)d.feed(c,output);for(auto c:payload)d.feed(c,output);assert(d.valid==2&&d.sequence==43);
  linkproto::makeHeader(header,44,payload,2);for(auto c:header)d.feed(c,output);for(auto c:payload)d.feed(c,output);assert(d.type==2&&d.sequence==44&&!memcmp(payload,output,1024));
  App otaMenu;otaMenu.view=View::MAIN;otaMenu.selected=6;otaMenu.handle("Y");assert(otaMenu.view==View::UPDATE);otaMenu.handle("Y");assert(otaMenu.effects.back().kind==EffectKind::UPDATE_S3);otaMenu.view=View::UPDATE;otaMenu.selected=1;otaMenu.handle("Y");assert(otaMenu.effects.back().kind==EffectKind::UPDATE_DISPLAY);
  JsonDocument doc;deserializeJson(doc,R"({"status":"completed","output":[{"type":"reasoning"},{"type":"message","content":[{"type":"output_text","text":"RESULTADO\n5"}]}],"usage":{"input_tokens":100,"output_tokens":20}})");auto ar=parseApiResponse(doc,200);assert(ar.ok&&ar.text=="RESULTADO\n5"&&ar.inputTokens==100);
  doc["status"]="incomplete";assert(!parseApiResponse(doc,200).ok);doc.clear();doc["error"]["code"]="credit_balance_exhausted";assert(parseApiResponse(doc,429).error.find("credito")!=std::string::npos);makeApiRequest(doc,false);assert(std::string(doc["model"]|"")=="gpt-6.1-sol"&&!doc["store"].as<bool>());
  doc.clear();doc["error"]["code"]="project_spend_limit_exceeded";assert(parseApiResponse(doc,429).error.find("gastos")!=std::string::npos);
  doc.clear();deserializeJson(doc,R"({"status":"completed","output":[{"type":"message","content":[{"type":"refusal","refusal":"Nao posso analisar"}]}]})");assert(!parseApiResponse(doc,200).ok);
  {const uint8_t bytes[]={0,1,2,3,4,5,6,7,8,9,10,11};VisionBody stream;stream.image(bytes,sizeof(bytes));stream.text("|");stream.image(bytes,1);std::string text;while(stream.remaining())text+=(char)stream.read();assert(text=="AAECAwQFBgcICQoL|AA==");}
  for(unsigned n=1;n<=3;n++){
    JsonDocument request;makeApiRequest(request,false,n);std::string json;serializeJson(request,json);
    assert(request["input"][0]["content"].size()==n+1);
    const uint8_t a[]={0},b[]={0,1},c[]={0,1,2};const uint8_t* photos[]={a,b,c};size_t sizes[]={1,2,3};const char* expected[]={"AA==","AAE=","AAEC"};
    VisionBody body;size_t start=0;
    for(unsigned i=0;i<n;i++){std::string marker="\"__PHOTO_"+std::to_string(i)+"__\"";size_t at=json.find(marker,start);assert(at!=std::string::npos);body.text(json.substr(start,at-start)+"\"data:image/jpeg;base64,");body.image(photos[i],sizes[i]);body.text("\"");start=at+marker.size();}body.text(json.substr(start));
    std::string wire;while(body.remaining())wire+=(char)body.read();assert(body.read()==-1&&wire.size()==body.length());JsonDocument parsed;assert(!deserializeJson(parsed,wire));
    for(unsigned i=0;i<n;i++)assert(std::string(parsed["input"][0]["content"][i+1]["image_url"]|"")==std::string("data:image/jpeg;base64,")+expected[i]);
  }
  if(argc>1){std::string dir=argv[1];std::filesystem::create_directories(dir);App preview;preview.engine.input="(125+37)*2";preview.engine.cursor=10;preview.engine.enter();preview.render(0);pbm(dir+"/01_calculadora.pbm",preview.screen);preview.handle("E");preview.render(3200);pbm(dir+"/02_menu.pbm",preview.screen);preview.profiles={"DANIEL-HUAWEI","iPhone 17 PRO MAX de Daniel"};preview.handle("Y");preview.render(3300);pbm(dir+"/03_wifi.pbm",preview.screen);preview.view=View::READER;preview.setResponse("TRANSCRICAO\n2*x + 6 = 14\n\nCALCULO\n2*x = 14 - 6\n2*x = 8\nx = 8/2\n\nRESULTADO\nx = 4");preview.render(3400);pbm(dir+"/04_resposta.pbm",preview.screen);preview.handle("R");preview.render(3500);pbm(dir+"/05_rolagem.pbm",preview.screen);}
  if(argc>1){App ui;ui.engine.input="tan(pi/4)";ui.engine.cursor=9;ui.render(0);pbm(std::string(argv[1])+"/06_header_empty.pbm",ui.screen);ui.online=true;ui.photoCount=1;ui.photoCapturing=true;ui.render(0);pbm(std::string(argv[1])+"/07_header_capture.pbm",ui.screen);ui.photoCount=3;ui.photoCapturing=false;ui.render(0);pbm(std::string(argv[1])+"/08_header_full.pbm",ui.screen);}
  std::cout<<"PASS: arithmetic, domains, E/SPC, token deletion, pi glyph, photo slots/blink, 1-3 image streaming, RPN, memory, editor, CRC and Responses\n";
}

#pragma once
#include "calculator_core.h"
#include "screen.h"
#include <stdint.h>

namespace calc {
enum class View { CALCULATOR, MAIN, WIFI, SCANNED, TEXT, FUNCTIONS, CONSTANTS, VARIABLES, PROGRAMS, SETTINGS, READER, BUSY, HELP, UPDATE };
enum class EffectKind { CONNECT, ADD_WIFI, SCAN_WIFI, CAPTURE, PORTAL, SAVE_SETTINGS, UPDATE_S3, UPDATE_DISPLAY, VIEW_PHOTO, CAPTURE_SLOT, SEND_PHOTOS, CLEAR_PHOTOS, REMOVE_PHOTO };
struct Effect { EffectKind kind; int index=0; std::string first,second; };
class App {
  bool enterHeld=false,longHandled=false,storeMode=false;
  uint32_t enterSince=0;
  int textStage=0,charGroup=0,charIndex=0;
  std::string textValue,textSSID;
  bool secret=false;
  const std::array<std::string,4> chars={"abcdefghijklmnopqrstuvwxyz","ABCDEFGHIJKLMNOPQRSTUVWXYZ","0123456789"," !@#$%&*()-_=+[]{};:'\",.<>/?\\|`~:^"};
  void effect(EffectKind k,int n=0,std::string a={},std::string b={}){effects.push_back({k,n,a,b});}
  void show(View next){view=next;selected=0;dirty=true;}
  void textEntry(int stage,const std::string& value={}){textStage=stage;textValue=value;secret=stage==2;charIndex=0;charGroup=0;show(View::TEXT);}
  void doneText(){
    if(textStage==1){if(textValue.empty()){notice="SSID vazio";return;}textSSID=textValue;textEntry(2);}
    else {effect(EffectKind::ADD_WIFI,0,textSSID,textValue);textValue.clear();textSSID.clear();show(View::WIFI);notice="Rede salva; selecione-a";}
  }
  void menuSelect(){
    if(view==View::MAIN){
      if(selected==0)show(View::WIFI);
      else if(selected==1){if(busy){notice="Consulta em andamento";show(View::BUSY);}else{busy=true;status="Preparando camera...";show(View::BUSY);effect(EffectKind::SEND_PHOTOS);}}
      else if(selected==2){show(View::READER);scroll=0;}
      else if(selected==3)show(View::CALCULATOR);
      else if(selected==4)show(View::SETTINGS);
      else if(selected==5){effect(EffectKind::PORTAL);status="Configuracao Wi-Fi";show(View::BUSY);}
      else if(selected==6)show(View::UPDATE);
      else if(selected==7){effect(EffectKind::VIEW_PHOTO);status="Abrindo fotos no Wi-Fi";show(View::BUSY);}
      else if(selected==8)effect(EffectKind::REMOVE_PHOTO);
      else if(selected==9)effect(EffectKind::CLEAR_PHOTOS);
      else show(View::HELP);
    }else if(view==View::UPDATE){effect(selected==0?EffectKind::UPDATE_S3:EffectKind::UPDATE_DISPLAY);status="Abrindo atualizacao...";show(View::BUSY);
    }else if(view==View::WIFI){
      if(selected<(int)profiles.size()){effect(EffectKind::CONNECT,selected);status="Conectando a rede...";show(View::BUSY);}
      else if(selected==(int)profiles.size())textEntry(1);
      else{effect(EffectKind::SCAN_WIFI);status="Buscando redes...";show(View::BUSY);}
    }else if(view==View::SCANNED){if(selected<(int)scanned.size())textEntry(1,scanned[selected]);}
    else if(view==View::FUNCTIONS){engine.function(functionNames()[selected]);show(View::CALCULATOR);}
    else if(view==View::CONSTANTS){const double values[]={kPi,std::exp(1.0),299792458,9.80665,6.02214076e23,1.380649e-23};if(selected==0)engine.pi();else engine.constant(values[selected]);show(View::CALCULATOR);}
    else if(view==View::VARIABLES){if(storeMode){engine.store(selected);effect(EffectKind::SAVE_SETTINGS);}else engine.recall(selected);show(View::CALCULATOR);}
    else if(view==View::PROGRAMS){
      if(selected==0){if(recording){recording=false;effect(EffectKind::SAVE_SETTINGS);notice="Programa salvo";}else{program.clear();recording=true;engine.reset();notice="Gravando; PRG para parar";}show(View::CALCULATOR);}
      else if(selected==1){recording=false;engine.reset();for(const auto& key:program){engine.key(key);if(!engine.error.empty())break;}show(View::CALCULATOR);}
      else{program.clear();recording=false;effect(EffectKind::SAVE_SETTINGS);notice="Programa apagado";show(View::CALCULATOR);}
    }else if(view==View::SETTINGS){
      if(selected==0){engine.rpn=!engine.rpn;engine.clear();}
      else if(selected==1)engine.degrees=!engine.degrees;
      else if(selected==2)economy=!economy;
      else if(selected==3)mirror=!mirror;
      else if(selected==4)flip=!flip;
      else {engine.reset();notice="Calculadora limpa";}
      effect(EffectKind::SAVE_SETTINGS);dirty=true;
    }
  }
public:
  Engine engine;Screen screen;
  unsigned photoCount=0;bool photoCapturing=false;
  View view=View::CALCULATOR;
  int selected=0,scroll=0;
  bool dirty=true,online=false,busy=false,recording=false,economy=false,mirror=false,flip=false,hasKey=false;
  std::string status="Pronto",notice,response="Nenhuma resposta salva.",network;
  std::vector<std::string> profiles,scanned,program;
  std::vector<Effect> effects;
  std::vector<std::string> responseLines={"Nenhuma resposta."};
  const std::vector<std::string>& functionNames(){static const std::vector<std::string> names={"sin","asin","cos","acos","tan","atan","sqrt","sq","inv","ln","log","exp","pow10","abs","fact","percent","sinh","cosh","tanh"};return names;}
  void press(const std::string& key,uint32_t){handle(key);}
  void release(const std::string&,uint32_t){}
  void tick(uint32_t){}
  bool holdingEnter()const{return false;}
  unsigned holdSeconds(uint32_t now)const{return enterHeld?(unsigned)((uint32_t)(now-enterSince)/1000):0;}
  void disconnectedKey(){enterHeld=false;longHandled=false;dirty=true;}
  void setResponse(const std::string& text){response=text.substr(0,12000);responseLines=wrap(response);scroll=0;busy=false;status="Resposta recebida";if(view==View::BUSY)show(View::READER);else{notice="Resposta disponivel";dirty=true;}}
  void setStatus(const std::string& text){status=ascii(text).substr(0,300);dirty=true;}
  void setError(const std::string& text){busy=false;photoCapturing=false;status="ERRO\n"+text;view=View::BUSY;dirty=true;}
  std::vector<std::string> menuItems(){
    if(view==View::MAIN)return {"Conectar no Wi-Fi","Enviar fotos para IA","Ler ultima resposta","Voltar a calculadora","Configuracoes","Configurar pelo celular","Atualizar firmware","Ver foto no Wi-Fi","Remover ultima foto","Limpar fotos","Ajuda e teclas"};
    if(view==View::UPDATE)return {"Atualizar ESP32-S3","Atualizar tela/DevKit"};
    if(view==View::WIFI){auto items=profiles;items.push_back("+ Cadastrar nova rede");items.push_back("Procurar redes proximas");return items;}
    if(view==View::SCANNED)return scanned.empty()?std::vector<std::string>{"Nenhuma rede encontrada"}:scanned;
    if(view==View::FUNCTIONS)return functionNames();
    if(view==View::CONSTANTS)return {"pi = 3.14159265359","e = 2.71828182846","c = 299792458 m/s","g = 9.80665 m/s2","NA = 6.02214076E23","kB = 1.380649E-23"};
    if(view==View::VARIABLES){std::vector<std::string> v;for(int i=0;i<6;i++)v.push_back(std::string(1,'A'+i)+" = "+format(engine.memory[i]));return v;}
    if(view==View::PROGRAMS)return {recording?"Parar e salvar gravacao":"Gravar novo programa","Executar programa","Apagar programa"};
    if(view==View::SETTINGS)return {std::string("Modo: ")+(engine.rpn?"RPN":"COMUM"),std::string("Angulo: ")+(engine.degrees?"GRAUS":"RADIANOS"),std::string("IA: ")+(economy?"ECONOMICA":"PRECISA"),std::string("Espelhar foto: ")+(mirror?"SIM":"NAO"),std::string("Girar foto: ")+(flip?"SIM":"NAO"),"Limpar calculadora"};
    return {};
  }
  void handle(const std::string& key){
    dirty=true;notice.clear();
    if(key=="E"){show(View::MAIN);return;}
    if(key=="SPC"){if(busy||photoCapturing){notice="Aguarde a captura";return;}if(photoCount>=3){notice="3 fotos: envie ou limpe";return;}photoCapturing=true;effect(EffectKind::CAPTURE_SLOT);return;}
    if(key=="ON"){if(view==View::CALCULATOR)engine.reset();else show(View::CALCULATOR);return;}
    if(view==View::CALCULATOR){
      auto command=engine.key(key);
      if(command==Command::FUNCTIONS)show(View::FUNCTIONS);
      else if(command==Command::PROGRAMS)show(View::PROGRAMS);
      else if(command==Command::CONSTANTS)show(View::CONSTANTS);
      else if(command==Command::VARIABLES||command==Command::STORE){storeMode=command==Command::STORE;show(View::VARIABLES);}
      else if(command==Command::SETTINGS)show(View::SETTINGS);
      else if(recording&&key!="LSHIFT"&&key!="RSHIFT"){if(program.size()<96)program.push_back(key);else{recording=false;notice="Programa cheio; salvo";effect(EffectKind::SAVE_SETTINGS);}}
      return;
    }
    if(view==View::TEXT){
      if(key=="F"){doneText();return;}
      if(key=="DEL"||key=="BACKSPACE"||key=="E"){if(!textValue.empty())textValue.pop_back();return;}
      if(key=="P")charIndex=(charIndex+(int)chars[charGroup].size()-1)%chars[charGroup].size();
      else if(key=="R")charIndex=(charIndex+1)%chars[charGroup].size();
      else if(key=="K"||key=="Q"){charGroup=(charGroup+(key=="K"?3:1))%4;charIndex=0;}
      else if(key=="A"||key=="B"||key=="C"||key=="D"){charGroup=key[0]-'A';charIndex=0;}
      else if(key=="Y"||key=="O"){size_t maxLen=secret?63:32;if(textValue.size()<maxLen)textValue+=chars[charGroup][charIndex];}
      else if(key=="SPC"){if(textValue.size()<(secret?63u:32u))textValue+=' ';}
      else if(key.size()==1&&key[0]>='0'&&key[0]<='9'){if(textValue.size()<(secret?63u:32u))textValue+=key;}
      return;
    }
    if(view==View::READER||view==View::HELP){
      int count=view==View::HELP?(int)helpLines().size():(int)responseLines.size();
      if(key=="K"||key=="A")scroll=std::max(0,scroll-1);
      else if(key=="Q"||key=="B")scroll=std::min(std::max(0,count-5),scroll+1);
      else if(key=="P"||key=="C")scroll=std::max(0,scroll-5);
      else if(key=="R"||key=="D")scroll=std::min(std::max(0,count-5),scroll+5);
      else if(key=="F")show(View::MAIN);
      return;
    }
    if(view==View::BUSY){if(key=="F")show(View::CALCULATOR);return;}
    if(key=="F"||key=="BACKSPACE"){show(View::CALCULATOR);return;}
    int count=menuItems().size();if(!count)return;
    if(key=="K"||key=="A")selected=(selected+count-1)%count;
    else if(key=="Q"||key=="B")selected=(selected+1)%count;
    else if(key=="P"||key=="C")selected=std::max(0,selected-5);
    else if(key=="R"||key=="D")selected=std::min(count-1,selected+5);
    else if(key=="Y"||key=="O"||key=="E")menuSelect();
  }
  std::vector<std::string> helpLines(){return wrap("CALCULADORA CIENTIFICA\nModo comum: 2 + 3 ENTER\nRPN: 2 ENTER 3 +\nE abre menu. SPC fotografa. Ate 3 fotos; menu envia.\nON volta / limpa conta.\nMTH: funcoes, inclusive SIN.\nPRG: gravar e executar contas.\nCST: constantes.\nVAR: recuperar A..F.\nSTO: guardar em A..F.\nNXT troca teclas A..F.\nSHIFT+COS: acos/cosh.\nSHIFT+TAN: atan/tanh.\nSHIFT+sqrt: quadrado.\nSHIFT+1/x: ln/exp.\nDEL limpa entrada.\nSHIFT+DEL limpa tudo.\nSetas: historico/cursor.\nNa resposta: cima/baixo rolam; esquerda/direita paginam.\nCadastro Wi-Fi: A..D escolhem caracteres, setas escolhem letra, ENTER insere, F salva.\nAs teclas M, SIN e ALPHA fisicas estao ausentes. Suas funcoes ficam nos menus.\nA IA pode errar. Confira a transcricao e as contas.\nAPI usa configuracao propria. Fotos so sao enviadas ao selecionar a consulta.\nAplicacao cientifica; nao emula a ROM de uma HP48.");}
  static size_t visibleColumns(std::string s){size_t n=s.size(),p=0;while((p=s.find("pi",p))!=std::string::npos){n--;p+=2;}return n;}
  static std::string displayExpression(std::string text){size_t p=0;while((p=text.find("pi",p))!=std::string::npos){text.replace(p,2,"π");p+=2;}return text;}
  void render(uint32_t now){
    if(view==View::CALCULATOR){
      screen.clear();
      if(engine.rpn){int first=std::max(0,(int)engine.stack.size()-3);for(int i=first;i<(int)engine.stack.size();i++)screen.text(1,11+(i-first)*9,std::to_string((int)engine.stack.size()-i)+": "+format(engine.stack[i]));screen.text(1,40,">"+engine.input.substr(engine.input.size()>19?engine.input.size()-19:0));}
      else{std::string expression=engine.justEvaluated||engine.input.empty()?engine.lastExpression:engine.input;size_t position=engine.justEvaluated?expression.size():engine.cursor;size_t begin=position>20?position-20:0;screen.text(1,13,displayExpression(expression.substr(begin,21)));if(!engine.justEvaluated&&engine.cursor>=begin)screen.rect(1+visibleColumns(expression.substr(begin,engine.cursor-begin))*6,22,5,1);std::string value=format(engine.answer);screen.text(value.size()<=10?std::max(1,127-(int)value.size()*12):1,30,value,true,value.size()<=10?2:1);}
      if(!engine.error.empty()){screen.rect(0,45,128,9);screen.text(1,46,engine.error.substr(0,21),false);}
      else if(!notice.empty())screen.text(1,46,notice.substr(0,21));
      if(holdingEnter()){screen.rect(0,44,128,10);screen.text(1,46,"Menu em "+std::to_string(3-std::min(3u,holdSeconds(now)))+"s",false);}
      if(engine.panel==0)screen.footer({"(",")","π","ANS","MNU","MTH"});else if(engine.panel==1)screen.footer({"SIN","LOG","LN","EXP","MNU","DEG"});else screen.footer({"MEM","STO","RCL","PRG","MNU","CFG"});
    }else if(view==View::TEXT){
      screen.clear();screen.header(secret?"Senha Wi-Fi":"Nome da rede (SSID)");std::string shown=secret?std::string(textValue.size(),'*'):textValue;screen.text(1,12,shown.substr(shown.size()>21?shown.size()-21:0));
      screen.text(1,25,"Caractere: ");screen.rect(70,23,16,12);screen.text(75,25,std::string(1,chars[charGroup][charIndex]),false);screen.text(1,39,"ENTER insere; setas");screen.text(1,47,"F salva; ON cancela");screen.footer({"abc","ABC","123","SYM","DEL","OK"});
    }else if(view==View::READER||view==View::HELP){
      screen.clear();auto lines=view==View::HELP?helpLines():responseLines;scroll=std::max(0,std::min(scroll,std::max(0,(int)lines.size()-5)));screen.header((view==View::HELP?"AJUDA ":"RESPOSTA ")+std::to_string(scroll+1)+"/"+std::to_string(lines.size()));for(int row=0;row<5&&scroll+row<(int)lines.size();row++)screen.text(1,11+row*8,lines[scroll+row]);screen.footer({"UP","DN","PG-","PG+","","VOL"});
    }else if(view==View::BUSY){
      screen.clear();screen.header(busy?"CONSULTA POR FOTO":"AGUARDE / CONFIGURE");auto lines=wrap(status);for(int row=0;row<5&&row<(int)lines.size();row++)screen.text(1,11+row*8,lines[row]);screen.footer({"","","","","","CAL"});
    }else{
      const char* title=view==View::MAIN?"MENU":view==View::UPDATE?"ATUALIZAR FIRMWARE":view==View::WIFI?"REDES SALVAS":view==View::SCANNED?"REDES PROXIMAS":view==View::FUNCTIONS?"FUNCOES":view==View::CONSTANTS?"CONSTANTES":view==View::VARIABLES?(storeMode?"GUARDAR EM":"RECUPERAR"):view==View::PROGRAMS?"PROGRAMA":"CONFIGURACOES";
      screen.menu(title,menuItems(),selected);
    }
    screen.rect(0,0,128,9,false);
    if(online){screen.pixel(3,7);screen.pixel(2,5);screen.pixel(4,5);screen.pixel(0,3);screen.pixel(1,2);screen.pixel(2,1);screen.pixel(3,1);screen.pixel(4,1);screen.pixel(5,2);screen.pixel(6,3);}
    if(view!=View::CALCULATOR)screen.text(10,1,view==View::MAIN?"MENU":view==View::WIFI?"WI-FI":view==View::BUSY?"AGUARDE":"OPCOES");
    for(unsigned i=0;i<3;i++){int x=103+i*8;screen.rect(x,1,6,7);screen.rect(x+1,2,4,5,false);if(i<photoCount||(photoCapturing&&i==photoCount&&(now/250)%2==0))screen.rect(x+1,2,4,5);}
    dirty=false;
  }
};
} // namespace calc

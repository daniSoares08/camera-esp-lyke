#pragma once
#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace calc {
constexpr double kPi=3.1415926535897932384626433832795;
struct Result { bool ok; double value; std::string error; };
inline Result failure(const char* message) { return {false,0,message}; }
inline Result number(double value) { return std::isfinite(value)?Result{true,value,{}}:failure("Resultado fora da faixa"); }
inline std::string format(double value) {
  if(value==0) value=0;
  char b[48]; std::snprintf(b,sizeof(b),"%.12g",value); return b;
}
inline Result unary(const std::string& f,double x,bool degrees) {
  double a=degrees?x*kPi/180:x, y=0;
  if(f=="sin") y=std::sin(a);
  else if(f=="cos") y=std::cos(a);
  else if(f=="tan") { if(std::fabs(std::cos(a))<1e-12) return failure("Tangente indefinida"); y=std::tan(a); }
  else if(f=="asin"||f=="acos") { if(std::fabs(x)>1) return failure("Dominio: -1 a 1"); y=f=="asin"?std::asin(x):std::acos(x); if(degrees)y*=180/kPi; }
  else if(f=="atan") { y=std::atan(x); if(degrees)y*=180/kPi; }
  else if(f=="sinh") y=std::sinh(x);
  else if(f=="cosh") y=std::cosh(x);
  else if(f=="tanh") y=std::tanh(x);
  else if(f=="sqrt") { if(x<0)return failure("Raiz de negativo"); y=std::sqrt(x); }
  else if(f=="sq") y=x*x;
  else if(f=="inv") { if(x==0)return failure("Divisao por zero"); y=1/x; }
  else if(f=="ln"||f=="log") { if(x<=0)return failure("Log exige x > 0"); y=f=="ln"?std::log(x):std::log10(x); }
  else if(f=="exp") y=std::exp(x);
  else if(f=="pow10") y=std::pow(10,x);
  else if(f=="abs") y=std::fabs(x);
  else if(f=="neg") y=-x;
  else if(f=="percent") y=x/100;
  else if(f=="fact") {
    if(x<0||x>170||std::floor(x)!=x)return failure("Fatorial: inteiro 0..170");
    y=1; for(unsigned i=2;i<=(unsigned)x;i++)y*=i;
  } else return failure("Funcao desconhecida");
  return number(y);
}
inline Result binary(char op,double a,double b) {
  if(op=='+')return number(a+b);
  if(op=='-')return number(a-b);
  if(op=='*')return number(a*b);
  if(op=='/')return b==0?failure("Divisao por zero"):number(a/b);
  if(op=='^')return number(std::pow(a,b));
  return failure("Operacao desconhecida");
}

// Recursive descent: power is right associative and binds before unary minus.
// Bounded nesting and input length prevent an expression from exhausting RAM.
class Parser {
  const std::string& s; size_t p=0; unsigned depth=0; bool deg;
  double ans; const std::array<double,6>& mem; std::string error;
  void ws(){while(p<s.size()&&(s[p]==' '||s[p]=='\t'))p++;}
  bool take(char c){ws();if(p<s.size()&&s[p]==c){p++;return true;}return false;}
  double fail(const char* m){if(error.empty())error=m;return 0;}
  double expr(){double v=term();for(;;){if(take('+'))v=apply('+',v,term());else if(take('-'))v=apply('-',v,term());else break;}return v;}
  double term(){double v=sign();for(;;){if(take('*'))v=apply('*',v,sign());else if(take('/'))v=apply('/',v,sign());else break;}return v;}
  double sign(){if(take('+'))return sign();if(take('-'))return -sign();return power();}
  double power(){double v=postfix();if(take('^'))v=apply('^',v,sign());return v;}
  double postfix(){double v=atom();for(;;){if(take('!'))v=fun("fact",v);else if(take('%'))v=fun("percent",v);else break;}return v;}
  double apply(char op,double a,double b){auto r=binary(op,a,b);if(!r.ok)fail(r.error.c_str());return r.value;}
  double fun(const std::string& f,double x){auto r=unary(f,x,deg);if(!r.ok)fail(r.error.c_str());return r.value;}
  double atom(){
    ws(); if(++depth>32){depth--;return fail("Expressao muito profunda");}
    double v=0;
    if(take('(')){v=expr();if(!take(')'))fail("Falta fechar parenteses");}
    else if(p<s.size()&&((s[p]>='0'&&s[p]<='9')||s[p]=='.')){
      const char* begin=s.c_str()+p; char* end=nullptr; errno=0;v=std::strtod(begin,&end);
      if(end==begin||errno==ERANGE||!std::isfinite(v))fail("Numero fora da faixa");
      p+=(size_t)(end-begin);
    } else if(p<s.size()&&((s[p]>='a'&&s[p]<='z')||(s[p]>='A'&&s[p]<='Z'))){
      size_t start=p;while(p<s.size()&&((s[p]>='a'&&s[p]<='z')||(s[p]>='A'&&s[p]<='Z')||(s[p]>='0'&&s[p]<='9')))p++;
      std::string f=s.substr(start,p-start);
      if(f=="pi")v=kPi;else if(f=="e")v=std::exp(1.0);else if(f=="ans")v=ans;
      else if(f.size()==1&&f[0]>='A'&&f[0]<='F')v=mem[f[0]-'A'];
      else if(take('(')){v=expr();if(!take(')'))fail("Falta fechar parenteses");v=fun(f,v);}
      else fail("Nome ou funcao invalida");
    } else fail("Numero esperado");
    depth--;return v;
  }
public:
  Parser(const std::string& text,bool degrees,double answer,const std::array<double,6>& memory):s(text),deg(degrees),ans(answer),mem(memory){}
  Result run(){if(s.empty())return failure("Digite uma conta");if(s.size()>240)return failure("Expressao muito longa");double v=expr();ws();if(p!=s.size())fail("Verifique a expressao");return error.empty()?number(v):Result{false,0,error};}
};

enum class Command { NONE, FUNCTIONS, PROGRAMS, CONSTANTS, VARIABLES, STORE, SETTINGS };
class Engine {
public:
  bool rpn=false,degrees=true,justEvaluated=false;
  int shift=0,panel=0,historyIndex=-1,activeMemory=0;
  double answer=0;
  std::string input,error,lastExpression;
  size_t cursor=0;
  std::array<double,6> memory{};
  std::vector<double> stack;
  std::vector<std::string> history;
  void clear(){input.clear();cursor=0;error.clear();justEvaluated=false;shift=0;}
  void reset(){clear();stack.clear();answer=0;lastExpression.clear();historyIndex=-1;}
  void insert(const std::string& s){
    if(justEvaluated){input.clear();cursor=0;justEvaluated=false;}
    if(input.size()+s.size()>240){error="Expressao muito longa";return;}
    input.insert(cursor,s);cursor+=s.size();error.clear();historyIndex=-1;
  }
  Result evaluateInput(){return Parser(input,degrees,answer,memory).run();}
  bool enter(){
    error.clear();
    if(rpn){
      if(input.empty()){if(stack.empty()){error="Pilha vazia";return false;}if(stack.size()>=32){error="Pilha cheia";return false;}stack.push_back(stack.back());answer=stack.back();return true;}
      auto r=evaluateInput();if(!r.ok){error=r.error;return false;}if(stack.size()>=32){error="Pilha cheia";return false;}
      stack.push_back(r.value);answer=r.value;remember();clear();return true;
    }
    if(input.empty())return true;
    auto r=evaluateInput();if(!r.ok){error=r.error;return false;}
    remember();answer=r.value;input=format(answer);cursor=input.size();justEvaluated=true;return true;
  }
  void remember(){lastExpression=input;history.push_back(input);if(history.size()>20)history.erase(history.begin());historyIndex=-1;}
  void operation(char op){
    error.clear();shift=0;
    if(rpn){
      if(!input.empty()&&!enter())return;
      if(stack.size()<2){error="Faltam 2 valores";return;}
      double b=stack.back(),a=stack[stack.size()-2];auto r=binary(op,a,b);if(!r.ok){error=r.error;return;}
      stack.pop_back();stack.back()=r.value;answer=r.value;return;
    }
    if(input.empty()&&op!='-'&&op!='+') {input=format(answer);cursor=input.size();}
    justEvaluated=false;insert(std::string(1,op));
  }
  void function(const std::string& f){
    shift=0;error.clear();
    if(rpn){
      if(!input.empty()&&!enter())return;
      if(stack.empty()){error="Pilha vazia";return;}
      auto r=unary(f,stack.back(),degrees);if(!r.ok){error=r.error;return;}stack.back()=answer=r.value;return;
    }
    auto entered=input.empty()?failure("vazio"):evaluateInput();
    if(justEvaluated||entered.ok){
      double x=justEvaluated?answer:entered.value;
      auto r=unary(f,x,degrees);if(!r.ok){error=r.error;return;}
      lastExpression=f+"("+(justEvaluated?format(x):input)+")";answer=r.value;input=format(answer);cursor=input.size();justEvaluated=true;
    } else insert(f+"(");
  }
  void constant(double value){if(rpn){if(stack.size()<32){stack.push_back(value);answer=value;}else error="Pilha cheia";}else insert(format(value));}
  bool store(int slot){
    double v=answer;
    if(!input.empty()&&!justEvaluated){auto r=evaluateInput();if(!r.ok){error=r.error;return false;}v=r.value;}
    else if(rpn&&!stack.empty())v=stack.back();
    memory[slot]=v;activeMemory=slot;return true;
  }
  void pi(){if(rpn)constant(kPi);else insert("pi");}
  void backspace(){
    justEvaluated=false;
    for(size_t a=0;a<input.size();){
      if(input[a]>='a'&&input[a]<='z'){size_t b=a+1;while(b<input.size()&&((input[b]>='a'&&input[b]<='z')||(input[b]>='0'&&input[b]<='9')))b++;if(b<input.size()&&input[b]=='(')b++;
        if(cursor>a&&cursor<=b){input.erase(a,b-a);cursor=a;error.clear();return;}a=b;
      }else a++;
    }
    if(cursor){input.erase(--cursor,1);error.clear();}
    else if(rpn&&input.empty()&&!stack.empty()){stack.pop_back();answer=stack.empty()?0:stack.back();}
  }
  void recall(int slot){activeMemory=slot;constant(memory[slot]);}
  void exponent(){
    if(justEvaluated){input.clear();cursor=0;justEvaluated=false;}
    if(input.empty())insert("1");
    if(cursor&&((input[cursor-1]>='0'&&input[cursor-1]<='9')||input[cursor-1]=='.'))insert("E");
  }
  void negate(){
    if(rpn&&input.empty()){function("neg");return;}
    if(input.empty()){insert("-");return;}
    size_t start=cursor;
    while(start>0&&((input[start-1]>='0'&&input[start-1]<='9')||input[start-1]=='.'))start--;
    if(start>1&&(input[start-1]=='-'||input[start-1]=='+')&&(input[start-2]=='E'||input[start-2]=='e')){input[start-1]=input[start-1]=='-'?'+':'-';}
    else if(start>0&&(input[start-1]=='E'||input[start-1]=='e')){
      if(start<input.size()&&(input[start]=='-'||input[start]=='+'))input[start]=input[start]=='-'?'+':'-';else{input.insert(start,"-");cursor++;}
    } else if(start>0&&input[start-1]=='-'&&(start==1||input[start-2]=='('||input[start-2]=='*'||input[start-2]=='/'||input[start-2]=='^')){input.erase(start-1,1);cursor--;}
    else{input.insert(start,"-");cursor++;}
    justEvaluated=false;
  }
  void historyMove(int direction){
    if(history.empty())return;
    if(historyIndex<0)historyIndex=(int)history.size();
    historyIndex=std::max(0,std::min((int)history.size()-1,historyIndex+direction));
    input=history[historyIndex];cursor=input.size();justEvaluated=false;error.clear();
  }
  Command key(const std::string& k){
    if(k.size()==1&&k[0]>='0'&&k[0]<='9'){insert(k);shift=0;return Command::NONE;}
    if(k=="LSHIFT"){shift=shift==1?0:1;return Command::NONE;}
    if(k=="RSHIFT"){shift=shift==2?0:2;return Command::NONE;}
    if(k=="G")return Command::FUNCTIONS;
    if(k=="H")return Command::PROGRAMS;
    if(k=="I")return Command::CONSTANTS;
    if(k=="J")return Command::VARIABLES;
    if(k=="N")return Command::STORE;
    if(k=="L"){panel=(panel+1)%3;return Command::NONE;}
    if(k=="K"){historyMove(-1);return Command::NONE;}
    if(k=="Q"){historyMove(1);return Command::NONE;}
    if(k=="P"){if(cursor)cursor--;return Command::NONE;}
    if(k=="R"){if(cursor<input.size())cursor++;return Command::NONE;}
    if(k=="Y"||k=="O"||k=="SPC"){enter();return Command::NONE;}
    if(k=="ADD")operation('+');else if(k=="SUB")operation('-');else if(k=="MUL")operation('*');else if(k=="DIV")operation('/');else if(k=="W")operation('^');
    else if(k=="DOT")insert(".");else if(k=="EEX")exponent();else if(k=="Z")negate();
    else if(k=="T")function(shift==1?"acos":shift==2?"cosh":"cos");
    else if(k=="U")function(shift==1?"atan":shift==2?"tanh":"tan");
    else if(k=="S")function(shift==1?"asin":shift==2?"sinh":"sin");
    else if(k=="V")function(shift?"sq":"sqrt");else if(k=="X")function(shift==1?"ln":shift==2?"exp":"inv");
    else if(k=="ON")reset();
    else if(k=="DEL"){if(shift)reset();else clear();}
    else if(k=="BACKSPACE"){
      backspace();
    } else if(k.size()==1&&k[0]>='A'&&k[0]<='F'){
      unsigned i=k[0]-'A';
      if(panel==0){if(i==0)insert("(");else if(i==1)insert(")");else if(i==2)pi();else if(i==3)constant(answer);else if(i==4)function("percent");else return Command::FUNCTIONS;}
      if(panel==1){const char* fs[]={"sin","log","ln","exp","sq"};if(i<5)function(fs[i]);else degrees=!degrees;}
      if(panel==2){if(i==0||i==2)return Command::VARIABLES;if(i==1)return Command::STORE;if(i==3)return Command::PROGRAMS;if(i==4)return Command::CONSTANTS;if(i==5)return Command::SETTINGS;}
    }
    return Command::NONE;
  }
};
} // namespace calc

#pragma once
// Production keypad filter. Kept separate from the original mapping firmware.
struct PressFilter {
  static constexpr int W=8;
  enum Result { QUIET, DOWN, UP, MULTIPLE };
  int samples[W]={},used=0,cursor=0,stable=-1,eventCode=-1;bool latched=true;
  void reset(){used=cursor=0;stable=eventCode=-1;latched=true;}
  Result update(int raw){
    samples[cursor]=raw;cursor=(cursor+1)%W;if(used<W)used++;if(used<W)return QUIET;
    int empty=0,ambiguous=0,codes[W]={},counts[W]={},kinds=0;
    for(int i=0;i<W;i++){int code=samples[i];if(code==-1){empty++;continue;}if(code==-2){ambiguous++;continue;}int slot=0;while(slot<kinds&&codes[slot]!=code)slot++;if(slot==kinds)codes[kinds++]=code;counts[slot]++;}
    int best=-1,votes=0,runner=0;for(int i=0;i<kinds;i++){if(counts[i]>votes){runner=votes;votes=counts[i];best=codes[i];}else if(counts[i]>runner)runner=counts[i];}
    if(empty>=6&&votes<=1){eventCode=stable;bool released=stable>=0;stable=-1;latched=false;return released?UP:QUIET;}
    if(ambiguous>=3||(votes>=3&&runner>=3)){bool first=stable!=-2;stable=-2;latched=true;return first?MULTIPLE:QUIET;}
    if(!latched&&votes>=3&&runner<=1&&ambiguous<=1){stable=eventCode=best;latched=true;return DOWN;}
    return QUIET;
  }
};
inline bool filterSelfTest(){
  PressFilter f;int downs=0,ups=0,bad=0;auto feed=[&](int k){auto r=f.update(k);downs+=r==PressFilter::DOWN;ups+=r==PressFilter::UP;bad+=r==PressFilter::MULTIPLE;};
  for(int i=0;i<24;i++)feed(-1);
  for(int i=0;i<96;i++)feed(i%2?19:-1);
  if(downs!=1||ups||f.stable!=19)return false;
  for(int i=0;i<24;i++)feed(-1);
  for(int i=0;i<240;i++)feed(i%12==0?i:-1);
  if(downs!=1||ups!=1)return false;
  for(int i=0;i<24;i++)feed(-2);
  for(int i=0;i<24;i++)feed(-1);
  for(int i=0;i<240;i++)feed(25);
  return downs==2&&ups==1&&bad==1&&f.stable==25;
}

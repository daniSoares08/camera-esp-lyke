#pragma once
#include <stdint.h>

// A rolling vote tolerates intermittent contacts. A press requires repeated
// evidence for ONE contact; rearming requires a mostly empty release window.
// This avoids resetting the debounce timer on every single noisy scan.
struct PressFilter {
  static constexpr int W=24;
  enum Result { QUIET, DOWN, UP, MULTIPLE };
  int samples[W]={}, used=0, cursor=0, stable=-1, eventCode=-1;
  bool latched=true;

  void reset() { used=cursor=0; stable=eventCode=-1; latched=true; }

  Result update(int raw) {
    samples[cursor]=raw; cursor=(cursor+1)%W;
    if(used<W) used++;
    if(used<W) return QUIET;
    int empty=0, ambiguous=0, codes[W]={}, counts[W]={}, kinds=0;
    for(int i=0;i<W;i++) {
      int code=samples[i];
      if(code==-1) { empty++; continue; }
      if(code==-2) { ambiguous++; continue; }
      int slot=0;
      while(slot<kinds && codes[slot]!=code) slot++;
      if(slot==kinds) { codes[slot]=code; kinds++; }
      counts[slot]++;
    }
    int best=-1, votes=0, runner=0;
    for(int i=0;i<kinds;i++) {
      if(counts[i]>votes) { runner=votes; votes=counts[i]; best=codes[i]; }
      else if(counts[i]>runner) runner=counts[i];
    }
    if(empty>=20 && votes<=2) {
      eventCode=stable;
      bool released=stable>=0;
      stable=-1; latched=false;
      return released?UP:QUIET;
    }
    if(ambiguous>=8 || (votes>=5 && runner>=5)) {
      bool first=stable!=-2;
      stable=-2; latched=true;
      return first?MULTIPLE:QUIET;
    }
    if(!latched && votes>=6 && runner<=2 && ambiguous<=3) {
      stable=eventCode=best; latched=true;
      return DOWN;
    }
    return QUIET;
  }
};

inline bool filterSelfTest() {
  PressFilter f;
  int downs=0, ups=0, multiple=0;
  auto feed=[&](int code) {
    auto r=f.update(code);
    if(r==PressFilter::DOWN) downs++;
    if(r==PressFilter::UP) ups++;
    if(r==PressFilter::MULTIPLE) multiple++;
  };
  for(int i=0;i<24;i++) feed(-1);
  // Replay the measured C pattern: about half the scans show the contact.
  for(int i=0;i<96;i++) feed(i%2?96:-1);
  if(downs!=1 || ups!=0 || f.stable!=96) return false;
  for(int i=0;i<24;i++) feed(-1);
  // Isolated, unrelated spikes must not become keys.
  for(int i=0;i<240;i++) feed(i%12==0?i:-1);
  if(downs!=1 || ups!=1) return false;
  for(int i=0;i<24;i++) feed(-2);
  if(multiple!=1) return false;
  for(int i=0;i<24;i++) feed(-1);
  for(int i=0;i<240;i++) feed(95);
  return downs==2 && ups==1 && multiple==1 && f.stable==95;
}

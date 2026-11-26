#pragma once
#include <stdint.h>

struct PortalLease {
  bool wanted=false;
  uint32_t activity=0;
  static constexpr uint32_t idleMs=120000;
  void touch(uint32_t now) { wanted=true;activity=now; }
  void close() { wanted=false; }
  bool expired(uint32_t now,bool busy) const {
    return wanted&&!busy&&uint32_t(now-activity)>=idleMs;
  }
};

struct OfflineRadioWatchdog {
  static constexpr uint32_t limitMs=20u*60u*1000u;
  uint32_t since=0;
  bool watching=true,paused=false;
  void resume(uint32_t now){since=now;watching=true;paused=false;}
  bool tick(uint32_t now,bool connected,bool busy){
    if(paused)return false;
    if(connected){watching=false;return false;}
    if(!watching){since=now;watching=true;}
    if(busy||uint32_t(now-since)<limitMs)return false;
    paused=true;return true;
  }
};

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

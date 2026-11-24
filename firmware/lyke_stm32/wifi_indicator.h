#pragma once
#include <stdint.h>
#include <stdbool.h>
enum WifiPhase { WIFI_IDLE, WIFI_JOIN, WIFI_OK, WIFI_FAIL, WIFI_DISABLED };
enum WifiSymbol { WIFI_HIDDEN, WIFI_SIGNAL, WIFI_CROSS };
typedef struct { unsigned phase, painted; uint32_t at; } WifiIndicator;
static void wifi_mark(WifiIndicator *s, unsigned phase, uint32_t now)
{
    if (s->phase != phase) { s->phase=phase; s->at=now; }
}
static unsigned wifi_symbol(const WifiIndicator *s, uint32_t now)
{
    if (s->phase==WIFI_OK) {
        uint32_t step=(now-s->at)/300;
        return step<6 && !(step&1) ? WIFI_SIGNAL : WIFI_HIDDEN;
    }
    if (s->phase==WIFI_FAIL) return ((now-s->at)/500)&1 ? WIFI_CROSS : WIFI_SIGNAL;
    return WIFI_HIDDEN;
}
static bool wifi_animation(WifiIndicator *s, uint32_t now)
{
    unsigned symbol=wifi_symbol(s,now);
    if (symbol==s->painted) return false;
    s->painted=symbol;return true;
}

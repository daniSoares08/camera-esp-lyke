#pragma once
#include <stdint.h>
#include <stdbool.h>

enum { IDLE_DEFAULT_MIN=60, IDLE_MAX_MIN=1440 };
typedef struct { uint32_t key_at,attempt_at; } InactivityTimer;
static inline unsigned idle_minutes_from_packed(uint32_t packed)
{
    unsigned n=(packed>>9)&2047u;
    return n&&n<=IDLE_MAX_MIN?n:IDLE_DEFAULT_MIN;
}
static inline void idle_touch(InactivityTimer *t,uint32_t now) { t->key_at=now; }
static inline bool idle_due(const InactivityTimer *t,unsigned minutes,uint32_t now)
{
    return (uint32_t)(now-t->key_at)>=minutes*60000u && (uint32_t)(now-t->attempt_at)>=60000u;
}

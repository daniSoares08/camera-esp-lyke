#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

enum SleepPhase { SLEEP_RUN, SLEEP_QUERY, SLEEP_COMMIT, SLEEP_ARMED, SLEEP_FAILED };
typedef struct {
    enum SleepPhase phase;
    uint16_t sequence;
    uint32_t since;
    bool supported, committed, wake_requested;
} SleepControl;

static bool sleep_start(SleepControl *s, uint32_t now)
{
    if (!s->supported || s->phase != SLEEP_RUN) return false;
    if (!++s->sequence) ++s->sequence;
    s->phase=SLEEP_QUERY; s->since=now;
    s->committed=s->wake_requested=false;
    return true;
}

static void sleep_tick(SleepControl *s, uint32_t now)
{
    if ((s->phase==SLEEP_QUERY || s->phase==SLEEP_COMMIT) &&
        (uint32_t)(now-s->since)>=1500) s->phase=SLEEP_FAILED;
}

static bool sleep_number(const char *p, uint16_t *number)
{
    uint32_t n=0; unsigned digits=0;
    for (;*p;++p) {
        if (*p<'0'||*p>'9'||++digits>5) return false;
        n=n*10+(unsigned)(*p-'0');
    }
    if (!digits || !n || n>65535) return false;
    *number=(uint16_t)n;return true;
}

// 1 means send GO; 2 means the peer has flushed SLEEPING and is shutting down.
// A delayed READY after timeout can never commit an unsolicited shutdown.
static int sleep_message(SleepControl *s, const char *text, uint32_t now)
{
    uint16_t sequence;
    sleep_tick(s,now);
    if (!strncmp(text,"SLEEP READY ",12) && sleep_number(text+12,&sequence) &&
        sequence==s->sequence && s->phase==SLEEP_QUERY) {
        s->phase=SLEEP_COMMIT;s->committed=true;s->since=now;return 1;
    }
    if (!strncmp(text,"SLEEPING ",9) && sleep_number(text+9,&sequence) &&
        sequence==s->sequence && s->phase==SLEEP_COMMIT) {
        s->phase=SLEEP_ARMED;return 2;
    }
    if ((!strncmp(text,"SLEEP BUSY ",11)||!strncmp(text,"SLEEP FAIL ",11)) &&
        sleep_number(text+11,&sequence) && sequence==s->sequence &&
        (s->phase==SLEEP_QUERY||s->phase==SLEEP_COMMIT)) s->phase=SLEEP_FAILED;
    return 0;
}

typedef struct { uint16_t sequence; uint32_t since; } SleepPeer;
static inline void sleep_peer_arm(SleepPeer *p,uint16_t sequence,uint32_t now) { p->sequence=sequence;p->since=now; }
static inline bool sleep_peer_commit(SleepPeer *p,uint16_t sequence,uint32_t now)
{
    bool ok=sequence&&sequence==p->sequence&&(uint32_t)(now-p->since)<1500;
    p->sequence=0;return ok;
}

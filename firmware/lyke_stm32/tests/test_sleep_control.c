#include <assert.h>
#include <stdio.h>
#include "../sleep_control.h"

int main(void)
{
    SleepControl s={0};SleepPeer peer={0};uint16_t number;
    assert(!sleep_start(&s,10));s.supported=true;
    assert(sleep_start(&s,10)&&s.sequence==1);
    assert(!sleep_message(&s,"SLEEP READY 2",11)&&s.phase==SLEEP_QUERY);
    assert(!sleep_message(&s,"SLEEPING 1",12));
    sleep_peer_arm(&peer,1,20);
    assert(sleep_message(&s,"SLEEP READY 1",30)==1&&s.committed);
    assert(!sleep_message(&s,"SLEEP READY 1",31));
    assert(sleep_peer_commit(&peer,1,32));
    assert(!sleep_peer_commit(&peer,1,33));
    assert(sleep_message(&s,"SLEEPING 1",34)==2&&s.phase==SLEEP_ARMED);
    s=(SleepControl){.supported=true};assert(sleep_start(&s,UINT32_MAX-100));
    sleep_tick(&s,100);assert(s.phase==SLEEP_QUERY);
    sleep_tick(&s,1400);assert(s.phase==SLEEP_FAILED&&!s.committed);
    assert(!sleep_message(&s,"SLEEP READY 1",1401));
    s.phase=SLEEP_RUN;assert(sleep_start(&s,2000));
    assert(sleep_message(&s,"SLEEP READY 2",2001)==1);
    sleep_tick(&s,3501);assert(s.phase==SLEEP_FAILED&&s.committed);
    s.phase=SLEEP_RUN;assert(sleep_start(&s,4000));
    sleep_message(&s,"SLEEP BUSY 3",4001);assert(s.phase==SLEEP_FAILED&&!s.committed);
    s.phase=SLEEP_RUN;s.sequence=65535;assert(sleep_start(&s,5000)&&s.sequence==1);
    for(const char **p=(const char *[]){"","0","65536","12 extra","-1","999999999999",NULL};*p;++p)
        assert(!sleep_number(*p,&number));
    assert(sleep_number("65535",&number)&&number==65535);
    sleep_peer_arm(&peer,9,UINT32_MAX-10);assert(sleep_peer_commit(&peer,9,20));
    sleep_peer_arm(&peer,10,100);assert(!sleep_peer_commit(&peer,10,1600));
    puts("Sleep handshake: capability, nonce, two-stage commit, timeout, stale messages and rollover OK");
    return 0;
}

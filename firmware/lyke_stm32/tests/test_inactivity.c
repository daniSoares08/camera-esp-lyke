#include <assert.h>
#include <stdio.h>
#include "../inactivity.h"
int main(void){
    InactivityTimer t={0};idle_touch(&t,100);
    assert(!idle_due(&t,60,3600099)&&idle_due(&t,60,3600100));
    t.attempt_at=3600100;assert(!idle_due(&t,60,3660099)&&idle_due(&t,60,3660100));
    for(uint32_t now=4000000;now<8000000;now+=4){idle_touch(&t,now);assert(!idle_due(&t,1,now));}
    idle_touch(&t,UINT32_MAX-100);t.attempt_at=UINT32_MAX-100;
    assert(!idle_due(&t,1,59898)&&idle_due(&t,1,59899));
    assert(idle_minutes_from_packed(0)==60);
    for(unsigned n=1;n<=1440;++n)assert(idle_minutes_from_packed(0x1ffu|(n<<9))==n);
    assert(idle_minutes_from_packed(1441u<<9)==60);
    assert(!idle_due(&(InactivityTimer){0},1440,86399999));
    assert(idle_due(&(InactivityTimer){0},1440,86400000));
    puts("Physical inactivity: 60-minute default, holds, wraparound, bounded retry, limits and legacy preferences OK");
}

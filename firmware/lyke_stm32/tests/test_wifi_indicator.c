#include <assert.h>
#include "../wifi_indicator.h"
int main(void) {
    WifiIndicator s={0};assert(wifi_symbol(&s,0)==WIFI_HIDDEN);
    wifi_mark(&s,WIFI_JOIN,10);assert(wifi_symbol(&s,1000)==WIFI_HIDDEN);
    wifi_mark(&s,WIFI_OK,1000);
    for(unsigned i=0;i<6;++i)assert(wifi_symbol(&s,1000+i*300)==(i&1?WIFI_HIDDEN:WIFI_SIGNAL));
    wifi_mark(&s,WIFI_OK,1900);assert(s.at==1000&&wifi_symbol(&s,2800)==WIFI_HIDDEN);
    wifi_mark(&s,WIFI_FAIL,3000);assert(wifi_symbol(&s,3000)==WIFI_SIGNAL);
    assert(wifi_symbol(&s,3500)==WIFI_CROSS&&wifi_symbol(&s,4000)==WIFI_SIGNAL);
    wifi_mark(&s,WIFI_DISABLED,4100);assert(wifi_symbol(&s,4100)==WIFI_HIDDEN);
    wifi_mark(&s,WIFI_OK,UINT32_MAX-100);assert(wifi_symbol(&s,199)==WIFI_HIDDEN);
    assert(wifi_animation(&s,499));assert(!wifi_animation(&s,499));
    return 0;
}

#include <assert.h>
#include "../battery_math.h"
#include "../battery_gauge.h"

int main(void)
{
    BatteryReading r = battery_convert(2730, 1500, 1500);
    assert(r.status == BATTERY_OK && r.mv == 4000 && r.pin_mv == 2000 && r.vdda_mv == 3000);
    r = battery_convert(4095, 1500, 1665);
    assert(r.mv == 6660 && r.status == BATTERY_SATURATED);
    r = battery_convert(3500, 1500, 1665);
    assert(r.status == BATTERY_RANGE);
    assert(battery_convert(2000, 0, 1500).status == BATTERY_NO_READING);
    assert(battery_convert(2000, 1500, 0).status == BATTERY_NO_READING);
    assert(battery_convert(2000, 200, 1500).status == BATTERY_NO_READING);
    assert(battery_convert(0, 1500, 1500).mv == 0);
    r=(BatteryReading){.status=BATTERY_OK,.mv=3840};
    assert(battery_level(&r,BATTERY_LEVEL_UNKNOWN)==2);
    r.mv=4200;assert(battery_level(&r,2)==4 && !battery_low(&r));
    r.mv=3400;assert(battery_level(&r,4)==0 && battery_low(&r));
    r.mv=3850;assert(battery_level(&r,2)==2);
    r.mv=3890;assert(battery_level(&r,2)==3);
    r.mv=3820;assert(battery_level(&r,3)==3);
    r.mv=3809;assert(battery_level(&r,3)==2);
    r.status=BATTERY_SATURATED;assert(battery_level(&r,4)==BATTERY_LEVEL_UNKNOWN && !battery_low(&r));
    r.status=BATTERY_OK;r.mv=0;assert(battery_level(&r,4)==BATTERY_LEVEL_UNKNOWN);
    for(unsigned mv=2500,level=BATTERY_LEVEL_UNKNOWN;mv<=4500;++mv){
        r.mv=mv;unsigned next=battery_level(&r,level);assert(next<=4);
        if(level<=4)assert(next>=level);level=next;
    }
    return 0;
}

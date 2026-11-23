#pragma once
#include "battery_math.h"

enum { BATTERY_LEVEL_UNKNOWN = 5 };

// Voltage-only indication for one Li-ion cell; these bars are not a fuel gauge.
// 40 mV hysteresis prevents small load/ADC changes from toggling a segment.
static unsigned battery_level(const BatteryReading *reading, unsigned previous)
{
    if (reading->status != BATTERY_OK || reading->mv < 2500 || reading->mv > 4500)
        return BATTERY_LEVEL_UNKNOWN;
    static const unsigned threshold[] = {3450, 3650, 3850, 4050};
    unsigned level = previous;
    if (level > 4) {
        level = 0;
        while (level < 4 && reading->mv >= threshold[level]) ++level;
    } else {
        while (level < 4 && reading->mv >= threshold[level] + 40) ++level;
        while (level && reading->mv + 40 < threshold[level-1]) --level;
    }
    return level;
}

static bool battery_low(const BatteryReading *reading)
{
    return reading->status == BATTERY_OK && reading->mv >= 2500 && reading->mv < 3450;
}

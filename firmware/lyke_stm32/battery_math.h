#pragma once
#include <stdint.h>
#include <stdbool.h>

enum BatteryStatus { BATTERY_NO_READING, BATTERY_OK, BATTERY_SATURATED, BATTERY_RANGE };
typedef struct {
    unsigned status, raw, reference, pin_mv, vdda_mv, mv;
} BatteryReading;

static BatteryReading battery_convert(unsigned input, unsigned reference, unsigned calibration)
{
    BatteryReading result = { .raw = input, .reference = reference };
    if (!reference || input > 4095 || reference > 4095 || calibration < 1200 || calibration >= 2300)
        return result;
    // STM32G431 factory VREFINT calibration is measured at VDDA=3000 mV.
    result.vdda_mv = (3000u * calibration + reference/2) / reference;
    if (result.vdda_mv < 1700 || result.vdda_mv > 3650) return result;
    result.pin_mv = (unsigned)((3000ull * calibration * input + 4095ull*reference/2) /
                              (4095ull * reference));
    result.mv = (unsigned)((6000ull * calibration * input + 4095ull*reference/2) /
                          (4095ull * reference));
    // A full-scale ADC cannot measure this single-cell divider reliably.
    // Report the fault instead of presenting ~2*VDDA as the battery voltage.
    result.status = input >= 4090 ? BATTERY_SATURATED :
                    result.mv > 4500 ? BATTERY_RANGE : BATTERY_OK;
    return result;
}

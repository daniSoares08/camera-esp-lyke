#pragma once
#include "battery_math.h"

#ifndef LYKE_VREF_CALIBRATION
#define LYKE_VREF_CALIBRATION (*(const uint16_t *)0x1fff75aau)
#endif

static BatteryReading battery;
enum BatteryPhase { BAT_IDLE, BAT_SETTLE, BAT_REGULATOR, BAT_CALIBRATE, BAT_READY, BAT_GAP, BAT_SAMPLE };
static enum BatteryPhase battery_phase;
static uint32_t battery_at, battery_completed_at;
static unsigned battery_channel, battery_samples, battery_total, battery_reference;
static bool battery_sampled;

static void battery_start(void)
{
    if (battery_phase != BAT_IDLE) return;
    const Pin pa0 = {GPIOA, 0, "PA0"};
    pin_mode(pa0, 3);
    GPIOA->PUPDR &= ~3u;
    battery_phase = BAT_SETTLE;
    battery_at = ticks;
}

static bool battery_complete(BatteryReading reading)
{
    battery = reading;
    // ADC1 is the only ADC user. Reset also terminates a failed conversion.
    RCC->AHB2ENR |= RCC_AHB2ENR_ADC12EN;
    (void)RCC->AHB2ENR;
    RCC->AHB2RSTR |= RCC_AHB2RSTR_ADC12RST;
    RCC->AHB2RSTR &= ~RCC_AHB2RSTR_ADC12RST;
    ADC1->CR = ADC_CR_DEEPPWD;
    ADC12_COMMON->CCR &= ~ADC_CCR_VREFEN;
    RCC->AHB2ENR &= ~RCC_AHB2ENR_ADC12EN;
    battery_phase = BAT_IDLE;
    battery_completed_at = ticks;
    battery_sampled = true;
    return true;
}

static void battery_cancel(void)
{
    if (battery_phase != BAT_IDLE) (void)battery_complete(battery);
}

// One bounded step per main-loop turn. No delay/spin while scanning keys/UART.
static bool battery_step(void)
{
    uint32_t elapsed = ticks - battery_at;
    switch (battery_phase) {
    case BAT_IDLE: return false;
    case BAT_SETTLE:
        if (elapsed < 200) return false;
        RCC->AHB2ENR |= RCC_AHB2ENR_ADC12EN;
        (void)RCC->AHB2ENR;
        ADC12_COMMON->CCR = (3u << ADC_CCR_CKMODE_Pos) | ADC_CCR_VREFEN;
        ADC1->CR = ADC_CR_ADVREGEN;
        battery_phase = BAT_REGULATOR;
        break;
    case BAT_REGULATOR:
        if (elapsed < 1) return false;
        ADC1->CR |= ADC_CR_ADCAL;
        battery_phase = BAT_CALIBRATE;
        break;
    case BAT_CALIBRATE:
        if (ADC1->CR & ADC_CR_ADCAL) {
            if (elapsed >= 10) return battery_complete((BatteryReading){0});
            return false;
        }
        ADC1->CFGR = ADC1->CFGR2 = ADC1->DIFSEL = 0;
        ADC1->SMPR1 = 7u << ADC_SMPR1_SMP1_Pos;
        ADC1->SMPR2 = 7u << ADC_SMPR2_SMP18_Pos;
        ADC1->ISR = ADC_ISR_ADRDY;
        ADC1->CR |= ADC_CR_ADEN;
        battery_phase = BAT_READY;
        break;
    case BAT_READY:
        if (!(ADC1->ISR & ADC_ISR_ADRDY)) {
            if (elapsed >= 10) return battery_complete((BatteryReading){0});
            return false;
        }
        battery_channel = 18;
        battery_samples = battery_total = 0;
        battery_phase = BAT_GAP;
        break;
    case BAT_GAP:
        if (elapsed < 2) return false;
        ADC1->SQR1 = battery_channel << ADC_SQR1_SQ1_Pos;
        ADC1->ISR = ADC_ISR_EOC | ADC_ISR_EOS | ADC_ISR_OVR;
        ADC1->CR |= ADC_CR_ADSTART;
        battery_phase = BAT_SAMPLE;
        break;
    case BAT_SAMPLE:
        if (!(ADC1->ISR & ADC_ISR_EOC)) {
            if (elapsed >= 10) return battery_complete((BatteryReading){0});
            return false;
        }
        { unsigned sample = ADC1->DR;
          if (battery_samples++) battery_total += sample; }
        if (battery_samples == 17) {
            unsigned average = (battery_total + 8) / 16;
            if (battery_channel == 1)
                return battery_complete(battery_convert(average, battery_reference, LYKE_VREF_CALIBRATION));
            battery_reference = average;
            battery_channel = 1;
            battery_samples = battery_total = 0;
        }
        battery_phase = BAT_GAP;
        break;
    }
    battery_at = ticks;
    return false;
}

static bool battery_poll(void)
{
    if (battery_phase == BAT_IDLE && (!battery_sampled || ticks - battery_completed_at >= 30000))
        battery_start();
    return battery_step();
}

// The explicit menu measurement retains its synchronous user-facing behavior.
static unsigned battery_mv(void)
{
    battery_start();
    while (battery_phase != BAT_IDLE) {
        battery_step();
        if (battery_phase != BAT_IDLE) wait_ms(1);
    }
    return battery.status == BATTERY_OK ? battery.mv : 0;
}

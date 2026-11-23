#pragma once

static bool adc_wait(volatile uint32_t *reg, uint32_t mask, bool set)
{
    uint32_t start = DWT->CYCCNT;
    while (((*reg & mask) != 0) != set)
        if (DWT->CYCCNT - start > 16000u * 10u)
            return false;
    return true;
}

static unsigned adc_average(unsigned channel)
{
    ADC1->SQR1 = channel << ADC_SQR1_SQ1_Pos;
    unsigned total = 0;
    for (unsigned i = 0; i < 9; ++i) {
        ADC1->CR |= ADC_CR_ADSTART;
        if (!adc_wait(&ADC1->ISR, ADC_ISR_EOC, true))
            return 0;
        unsigned sample = ADC1->DR;
        if (i)
            total += sample;
    }
    return (total + 4) / 8;
}

static unsigned battery_mv(void)
{
    const Pin pa0 = { GPIOA, 0, "PA0" };
    pin_mode(pa0, 3);
    RCC->AHB2ENR |= RCC_AHB2ENR_ADC12EN;
    (void)RCC->AHB2ENR;
    ADC12_COMMON->CCR = (3u << ADC_CCR_CKMODE_Pos) | ADC_CCR_VREFEN;
    ADC1->CR = ADC_CR_ADVREGEN;
    wait_ms(1);
    ADC1->CR |= ADC_CR_ADCAL;
    if (!adc_wait(&ADC1->CR, ADC_CR_ADCAL, false))
        goto off;
    ADC1->SMPR1 = 7u << ADC_SMPR1_SMP1_Pos;
    ADC1->SMPR2 = 7u << ADC_SMPR2_SMP18_Pos;
    ADC1->ISR = ADC_ISR_ADRDY;
    ADC1->CR |= ADC_CR_ADEN;
    if (!adc_wait(&ADC1->ISR, ADC_ISR_ADRDY, true))
        goto off;
    unsigned reference = adc_average(18), input = adc_average(1);
    const unsigned calibration = *(const uint16_t *)0x1fff75aau;
    unsigned mv = reference && calibration >= 1200 && calibration < 2300
        ? (unsigned)((6000ull * calibration * input + (4095u * reference) / 2u) /
                     (4095u * reference)) : 0;
    if (ADC1->CR & ADC_CR_ADSTART) {
        ADC1->CR |= ADC_CR_ADSTP;
        adc_wait(&ADC1->CR, ADC_CR_ADSTP, false);
    }
    ADC1->CR |= ADC_CR_ADDIS;
    adc_wait(&ADC1->CR, ADC_CR_ADEN, false);
    ADC1->CR = ADC_CR_DEEPPWD;
    ADC12_COMMON->CCR &= ~ADC_CCR_VREFEN;
    RCC->AHB2ENR &= ~RCC_AHB2ENR_ADC12EN;
    return mv;
off:
    RCC->AHB2RSTR |= RCC_AHB2RSTR_ADC12RST;
    RCC->AHB2RSTR &= ~RCC_AHB2RSTR_ADC12RST;
    RCC->AHB2ENR &= ~RCC_AHB2ENR_ADC12EN;
    return 0;
}

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {uint32_t PUPDR;} FakeGPIO;
static FakeGPIO gpio;
#define GPIOA (&gpio)
typedef struct {FakeGPIO *port;unsigned pin;const char *name;} Pin;
static void pin_mode(Pin p,unsigned mode){assert(p.port==GPIOA&&p.pin==0&&mode==3);}
static struct {uint32_t AHB2ENR,AHB2RSTR;} rcc;
static struct {uint32_t CR,ISR,DR,CFGR,CFGR2,DIFSEL,SMPR1,SMPR2,SQR1;} adc;
static struct {uint32_t CCR;} common;
#define RCC (&rcc)
#define ADC1 (&adc)
#define ADC12_COMMON (&common)
enum {RCC_AHB2ENR_ADC12EN=1,RCC_AHB2RSTR_ADC12RST=1,ADC_CCR_CKMODE_Pos=1,
      ADC_CCR_VREFEN=8,ADC_CR_ADVREGEN=1,ADC_CR_ADCAL=2,ADC_CR_ADEN=4,
      ADC_CR_ADSTART=8,ADC_CR_DEEPPWD=16,ADC_ISR_ADRDY=1,ADC_ISR_EOC=2,
      ADC_ISR_EOS=4,ADC_ISR_OVR=8,ADC_SMPR1_SMP1_Pos=0,ADC_SMPR2_SMP18_Pos=0,
      ADC_SQR1_SQ1_Pos=0};
static uint32_t ticks;
static unsigned input=2730,fail,wait_calls,reads[2];
static void wait_ms(unsigned);
#define LYKE_VREF_CALIBRATION 1500
#include "../battery.h"

static void hardware(void)
{
    if(battery_phase==BAT_CALIBRATE&&fail!=1)adc.CR&=~ADC_CR_ADCAL;
    if(battery_phase==BAT_READY)adc.ISR=fail==2?0:ADC_ISR_ADRDY;
    if(battery_phase==BAT_SAMPLE){
        adc.ISR=fail==3?0:ADC_ISR_EOC;
        adc.CR&=~ADC_CR_ADSTART;
        adc.DR=!battery_samples?4095:battery_channel==18?1500:input;
        if(fail!=3)++reads[battery_channel==1];
    }
}
static void wait_ms(unsigned n){assert(++wait_calls<1000);ticks+=n;hardware();}
static void finish(void)
{
    unsigned turns=0;bool done=false;
    while(!done){assert(++turns<1000);++ticks;hardware();done=battery_poll();}
    assert(battery_phase==BAT_IDLE&&!(rcc.AHB2ENR&RCC_AHB2ENR_ADC12EN));
    assert(!(common.CCR&ADC_CCR_VREFEN)&&adc.CR==ADC_CR_DEEPPWD);
}
int main(void)
{
    assert(!battery_poll()&&battery_phase==BAT_SETTLE);finish();
    assert(battery.status==BATTERY_OK&&battery.mv==4000&&battery.raw==2730);
    assert(reads[0]==17&&reads[1]==17&&!wait_calls);
    ticks=battery_completed_at+29999;assert(!battery_poll()&&battery_phase==BAT_IDLE);
    ++ticks;assert(!battery_poll()&&battery_phase==BAT_SETTLE);input=4095;finish();
    assert(battery.status==BATTERY_SATURATED&&!wait_calls);
    for(fail=1;fail<=3;++fail){battery_start();finish();assert(battery.status==BATTERY_NO_READING);}
    fail=0;input=2730;ticks=UINT32_MAX-100;battery_start();finish();
    assert(battery.status==BATTERY_OK&&battery.mv==4000&&!wait_calls);
    assert(battery_mv()==4000&&wait_calls>0);
    battery_start();assert(battery_phase==BAT_SETTLE);battery_cancel();
    assert(battery_phase==BAT_IDLE&&battery.mv==4000&&!(rcc.AHB2ENR&RCC_AHB2ENR_ADC12EN));
    puts("Actual ADC monitor: cooperative sampling, discarded first samples, 30s period, timeouts, wraparound and manual measurement OK");
    return 0;
}

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../sleep_control.h"
enum {TRACK_COUNT=19,EXTI0_IRQn=6,EXTI9_5_IRQn=23,EXTI15_10_IRQn=40,USART2_IRQn=38};
typedef int IRQn_Type;
typedef struct {uint32_t MODER,PUPDR,OTYPER,IDR,ODR;} GPIO_TypeDef;
static GPIO_TypeDef ga,gb,gc;
#define GPIOA (&ga)
#define GPIOB (&gb)
#define GPIOC (&gc)
#include "pins.h"
typedef struct {uint8_t first,second;} Pair;
static struct {uint32_t CTRL,VAL;} systick;
static struct {uint32_t ISCR_unused,ICSR,SCR;} scb;
static struct {uint32_t CR;} debug_regs;
static struct {uint32_t CR1;} pwr;
static struct {uint32_t CR,APB2ENR;} rcc;
static struct {uint32_t ISR,CR1;} uart;
static struct {uint32_t CR1;} spi;
static struct {uint32_t EXTICR[4];} syscfg;
static struct {uint32_t IMR1,EMR1,IMR2,EMR2,RTSR1,FTSR1,PR1,PR2;} exti;
static struct {uint32_t ISER[8],ICER[8],ICPR[8];} nvic;
#define SysTick (&systick)
#define SCB (&scb)
#define DBGMCU (&debug_regs)
#define PWR (&pwr)
#define RCC (&rcc)
#define USART2 (&uart)
#define SPI1 (&spi)
#define SYSCFG (&syscfg)
#define EXTI (&exti)
#define NVIC (&nvic)
enum {USART_ISR_TC=64,USART_CR1_RXNEIE_RXFNEIE=32,SPI_CR1_SPE=64,
 RCC_APB2ENR_SYSCFGEN=1,SCB_ICSR_PENDSTCLR_Msk=1,SCB_ICSR_PENDSVCLR_Msk=2,
 SCB_SCR_SLEEPDEEP_Msk=4,DBGMCU_CR_DBG_SLEEP=1,DBGMCU_CR_DBG_STOP=2,DBGMCU_CR_DBG_STANDBY=4,
 PWR_CR1_LPMS=7,PWR_CR1_LPMS_STOP1=1,RCC_CR_HSION=1,RCC_CR_HSIRDY=2};
static SleepControl sleep_control;
static bool battery_sampled=true,cancelled,irq_off,bounce;
static unsigned wfi_count,mode_enables,wake_pulses,lcd_off,lcd_on;
static unsigned rx_read,rx_write,incoming_length;
static bool incoming_overflow,rx_overflow;
static void pin_mode(Pin p,unsigned mode){p.port->MODER=(p.port->MODER&~(3u<<(p.bit*2)))|(mode<<(p.bit*2));}
static void pin_set(Pin p,bool on){if(on)p.port->ODR|=1u<<p.bit;else p.port->ODR&=~(1u<<p.bit);}
static void NVIC_DisableIRQ(int irq){NVIC->ISER[irq/32]&=~(1u<<(irq%32));}
static void NVIC_ClearPendingIRQ(int irq){NVIC->ICPR[irq/32]|=1u<<(irq%32);}
static void NVIC_EnableIRQ(int irq){
 if(irq==EXTI15_10_IRQn){++mode_enables;for(unsigned i=0;i<8;++i){assert(NVIC->ICER[i]==UINT32_MAX);NVIC->ISER[i]=0;}}
 NVIC->ISER[irq/32]|=1u<<(irq%32);
}
static void battery_cancel(void){cancelled=true;}
static void serial_init(void){USART2->CR1=1;USART2->ISR=USART_ISR_TC;pin_mode((Pin){GPIOA,2,""},2);pin_mode((Pin){GPIOA,3,""},2);}
static int serial_get(void){return -1;}
static void serial_send(const char*p){assert(!*p);}
static void keyboard_init(void){for(unsigned i=0;i<TRACK_COUNT;++i){pin_mode(tracks[i],0);tracks[i].port->PUPDR|=1u<<(tracks[i].bit*2);}}
static void lcd_transfer(bool data,const uint8_t*p,unsigned n){assert(!data&&n==2&&p[0]==0xae&&p[1]==0x28);++lcd_off;}
static void lcd_init(void){++lcd_on;}
static void wait_ms(unsigned n){
 assert(!irq_off);
 if(n==35&&bounce&&wfi_count==1)GPIOB->IDR|=1u<<12;
 if(n==50){assert(lcd_on&&!(GPIOA->ODR&(1u<<2)));++wake_pulses;}
}
static void fake_wfi(void){
 assert(irq_off&&SysTick->CTRL==0&&SCB->SCR&SCB_SCR_SLEEPDEEP_Msk);
 assert(cancelled&&lcd_off==1&&SPI1->CR1==0&&USART2->CR1==0);
 assert((PWR->CR1&PWR_CR1_LPMS)==PWR_CR1_LPMS_STOP1);
 assert(NVIC->ISER[1]==(1u<<8));
 for(unsigned i=0;i<8;++i)if(i!=1)assert(!NVIC->ISER[i]);
 assert(EXTI->IMR1==(1u<<12)&&!EXTI->EMR1&&!EXTI->IMR2&&!EXTI->EMR2);
 assert(!EXTI->RTSR1&&EXTI->FTSR1==(1u<<12)&&SYSCFG->EXTICR[3]==1);
 for(unsigned i=0;i<TRACK_COUNT;++i){
  const Pin p=tracks[i];unsigned mode=(p.port->MODER>>(p.bit*2))&3,pull=(p.port->PUPDR>>(p.bit*2))&3;
  if(i==1)assert(mode==0&&pull==1);else if(i==15)assert(mode==1&&!pull);else assert(mode==3&&!pull);
 }
 assert(GPIOA->ODR&(1u<<2));assert(!wake_pulses);
 assert(++wfi_count<=2);GPIOB->IDR&=~(1u<<12);
}
#define __disable_irq() (irq_off=true)
#define __enable_irq() (irq_off=false)
#define __DSB() ((void)0)
#define __ISB() ((void)0)
#define __WFI() fake_wfi()
#include "../power_stop.h"
static void reset(void){
 memset(&ga,0,sizeof(ga));memset(&gb,0,sizeof(gb));memset(&gc,0,sizeof(gc));memset(&nvic,0,sizeof(nvic));memset(&exti,0,sizeof(exti));memset(&syscfg,0,sizeof(syscfg));
 GPIOA->IDR=GPIOB->IDR=GPIOC->IDR=UINT32_MAX;keyboard_init();
 SysTick->CTRL=7;DBGMCU->CR=7;PWR->CR1=0x100;RCC->CR=RCC_CR_HSIRDY;SPI1->CR1=SPI_CR1_SPE;USART2->ISR=USART_ISR_TC;
 NVIC->ISER[1]=1u<<6;EXTI->IMR2=32;
 sleep_control=(SleepControl){0};battery_sampled=true;cancelled=irq_off=false;
 wfi_count=mode_enables=wake_pulses=lcd_off=lcd_on=0;
}
int main(void){
 for(unsigned pass=0;pass<3;++pass){
  reset();bounce=pass==1;sleep_control.wake_requested=pass==2;
  stop_until_mode((Pair){1,15});
  assert(wfi_count==(pass==2?0:pass==1?2:1));
  assert(!irq_off&&SysTick->CTRL==7&&!(SCB->SCR&SCB_SCR_SLEEPDEEP_Msk));
  assert(PWR->CR1==0x100&&DBGMCU->CR==7&&EXTI->IMR2==32&&SYSCFG->EXTICR[3]==0);
  assert(NVIC->ISER[1]==1u<<6&&!battery_sampled);
  assert(lcd_off==1&&lcd_on==1&&wake_pulses==1);
 }
 puts("Actual STOP routine: MODE-only EXTI, no tick/UART wake, bounce, early MODE, pin/peripheral restore and one ESP wake pulse OK");
 return 0;
}

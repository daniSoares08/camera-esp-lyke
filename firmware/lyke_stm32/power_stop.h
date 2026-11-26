#pragma once

static volatile uint32_t mode_wake_mask;
void EXTI15_10_IRQHandler(void) {
    EXTI->PR1=mode_wake_mask;
#ifdef LYKE_SLEEP_SELFTEST
    sleep_control.wake_requested=true;
#endif
}
void EXTI9_5_IRQHandler(void) __attribute__((alias("EXTI15_10_IRQHandler")));
void EXTI0_IRQHandler(void) __attribute__((alias("EXTI15_10_IRQHandler")));
void EXTI1_IRQHandler(void) __attribute__((alias("EXTI15_10_IRQHandler")));
void EXTI2_IRQHandler(void) __attribute__((alias("EXTI15_10_IRQHandler")));
void EXTI3_IRQHandler(void) __attribute__((alias("EXTI15_10_IRQHandler")));
void EXTI4_IRQHandler(void) __attribute__((alias("EXTI15_10_IRQHandler")));

static void esp_wake_pulse(void)
{
    const Pin tx={GPIOA,2,"PA2"};
    while(!(USART2->ISR&USART_ISR_TC)){}
    NVIC_DisableIRQ(USART2_IRQn);USART2->CR1=0;
    pin_set(tx,true);GPIOA->OTYPER&=~(1u<<2);pin_mode(tx,1);
    // UART idle is HIGH. A deliberately long LOW wakes ESP RTC GPIO13.
    pin_set(tx,false);wait_ms(50);pin_set(tx,true);wait_ms(10);
    serial_init();USART2->CR1|=USART_CR1_RXNEIE_RXFNEIE;
    while(serial_get()>=0){}
    rx_read=rx_write=0;incoming_length=0;incoming_overflow=rx_overflow=false;
    NVIC_ClearPendingIRQ(USART2_IRQn);NVIC_EnableIRQ(USART2_IRQn);
    serial_send("");
}

static void stop_until_mode(Pair pair)
{
    const Pin drive=tracks[pair.second],tx={GPIOA,2,"PA2"},rx={GPIOA,3,"PA3"};
#ifdef LYKE_SLEEP_SELFTEST
    const Pin sense={GPIOA,3,"PA3"};(void)pair;
#else
    const Pin sense=tracks[pair.first];
#endif
    const IRQn_Type irq=sense.bit<5?(IRQn_Type)(EXTI0_IRQn+sense.bit):sense.bit<10?EXTI9_5_IRQn:EXTI15_10_IRQn;
    uint32_t enabled[8],tick=SysTick->CTRL,debug=DBGMCU->CR;
    uint32_t imr1=EXTI->IMR1,emr1=EXTI->EMR1,imr2=EXTI->IMR2,emr2=EXTI->EMR2;
    uint32_t rising=EXTI->RTSR1,falling=EXTI->FTSR1;
    unsigned slot=sense.bit/4,shift=(sense.bit%4)*4;
    RCC->APB2ENR|=RCC_APB2ENR_SYSCFGEN;(void)RCC->APB2ENR;
    uint32_t old_route=SYSCFG->EXTICR[slot];
    unsigned port=sense.port==GPIOA?0:sense.port==GPIOB?1:2;
    mode_wake_mask=1u<<sense.bit;
    battery_cancel();
    // Give the peer time to leave its UART and enter deep sleep before any
    // peripheral is reconfigured. No periodic work survives the STOP below.
    wait_ms(100);
    const uint8_t off[]={0xae,0x28};lcd_transfer(false,off,sizeof(off));
    SPI1->CR1&=~SPI_CR1_SPE;
    while(!(USART2->ISR&USART_ISR_TC)){}
    __disable_irq();
    SysTick->CTRL=0;SysTick->VAL=0;
    SCB->ICSR=SCB_ICSR_PENDSTCLR_Msk|SCB_ICSR_PENDSVCLR_Msk;
    for(unsigned i=0;i<8;++i){enabled[i]=NVIC->ISER[i];NVIC->ICER[i]=UINT32_MAX;NVIC->ICPR[i]=UINT32_MAX;}
    USART2->CR1=0;
    pin_set(tx,true);pin_mode(tx,1);pin_mode(rx,3);GPIOA->PUPDR&=~(3u<<6);
    for(unsigned i=0;i<TRACK_COUNT;++i){pin_mode(tracks[i],3);tracks[i].port->PUPDR&=~(3u<<(2*tracks[i].bit));}
    pin_set(drive,false);pin_mode(drive,1);pin_mode(sense,0);
    sense.port->PUPDR|=1u<<(2*sense.bit);
    RCC->APB2ENR|=RCC_APB2ENR_SYSCFGEN;(void)RCC->APB2ENR;
    SYSCFG->EXTICR[slot]=(old_route&~(15u<<shift))|(port<<shift);
    EXTI->IMR1=mode_wake_mask;EXTI->EMR1=EXTI->IMR2=EXTI->EMR2=0;
    EXTI->RTSR1=0;EXTI->FTSR1=mode_wake_mask;
    NVIC_EnableIRQ(irq);
    DBGMCU->CR&=~(DBGMCU_CR_DBG_SLEEP|DBGMCU_CR_DBG_STOP|DBGMCU_CR_DBG_STANDBY);
    uint32_t power=PWR->CR1;
    PWR->CR1=(power&~PWR_CR1_LPMS)|PWR_CR1_LPMS_STOP1;
    while(!sleep_control.wake_requested){
        SCB->ICSR=SCB_ICSR_PENDSTCLR_Msk|SCB_ICSR_PENDSVCLR_Msk;
        EXTI->PR1=UINT32_MAX;EXTI->PR2=UINT32_MAX;NVIC_ClearPendingIRQ(irq);
        // PRIMASK closes the check/WFI race; a pending MODE interrupt wakes WFI.
        if(sense.port->IDR&mode_wake_mask){SCB->SCR|=SCB_SCR_SLEEPDEEP_Msk;__DSB();__WFI();__ISB();}
        SCB->SCR&=~SCB_SCR_SLEEPDEEP_Msk;
        SysTick->VAL=0;SysTick->CTRL=tick;__enable_irq();
        wait_ms(35);
        bool held=!(sense.port->IDR&mode_wake_mask);
        __disable_irq();SysTick->CTRL=0;
        if(held)break; // A bounce returns to STOP; it does not wake the ESP.
    }
    SCB->SCR&=~SCB_SCR_SLEEPDEEP_Msk;PWR->CR1=power;DBGMCU->CR=debug;
    NVIC_DisableIRQ(irq);NVIC_ClearPendingIRQ(irq);
    SYSCFG->EXTICR[slot]=old_route;
    EXTI->IMR1=imr1;EXTI->EMR1=emr1;EXTI->IMR2=imr2;EXTI->EMR2=emr2;
    EXTI->RTSR1=rising;EXTI->FTSR1=falling;EXTI->PR1=mode_wake_mask;
    RCC->CR|=RCC_CR_HSION;while(!(RCC->CR&RCC_CR_HSIRDY)){}
    keyboard_init();serial_init();USART2->CR1|=USART_CR1_RXNEIE_RXFNEIE;
    for(unsigned i=0;i<8;++i){NVIC->ICPR[i]=UINT32_MAX;NVIC->ISER[i]=enabled[i];}
    SysTick->VAL=0;SysTick->CTRL=tick;__enable_irq();
    lcd_init();battery_sampled=false;
    esp_wake_pulse();
}

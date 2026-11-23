#pragma once

#include "stm32g431xx.h"
#include <stdbool.h>
#include <stdint.h>

#define LYKE_APP 0x08002000u
#define LYKE_STAGE 0x08010000u
#define LYKE_MAX_APP (56u * 1024u)
#define LYKE_META 0x0801e000u
#define LYKE_KEYMAP 0x0801e800u
#define LYKE_REQUEST (*(volatile uint32_t *)0x200057f0u)
#define LYKE_BOOT_REQUEST 0x4c594b45u
#define LYKE_PAGE 2048u

static inline void serial_init(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;
    (void)RCC->APB1ENR1;
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(0xffu << 8)) | (0x77u << 8);
    GPIOA->MODER = (GPIOA->MODER & ~(0xfu << 4)) | (0xau << 4);
    GPIOA->PUPDR = (GPIOA->PUPDR & ~(3u << 6)) | (1u << 6);
    USART2->BRR = 139;
    USART2->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

static inline int serial_get(void)
{
    if (USART2->ISR & (USART_ISR_ORE | USART_ISR_FE | USART_ISR_NE))
        USART2->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NECF;
    return (USART2->ISR & USART_ISR_RXNE_RXFNE) ? (int)USART2->RDR : -1;
}

static inline void serial_send(const char *line)
{
    while (*line) {
        while (!(USART2->ISR & USART_ISR_TXE_TXFNF)) {}
        USART2->TDR = (uint8_t)*line++;
    }
    while (!(USART2->ISR & USART_ISR_TXE_TXFNF)) {}
    USART2->TDR = '\n';
}

static inline void flash_wait(void)
{
    while (FLASH->SR & FLASH_SR_BSY) {}
}

#define FLASH_ERRORS (FLASH_SR_OPERR | FLASH_SR_PROGERR | FLASH_SR_WRPERR | \
                      FLASH_SR_PGAERR | FLASH_SR_SIZERR | FLASH_SR_PGSERR | \
                      FLASH_SR_MISERR | FLASH_SR_FASTERR | FLASH_SR_RDERR)

static inline void flash_begin(void)
{
    flash_wait();
    if (FLASH->CR & FLASH_CR_LOCK) {
        FLASH->KEYR = 0x45670123u;
        FLASH->KEYR = 0xcdef89abu;
    }
    FLASH->SR = FLASH_ERRORS | FLASH_SR_EOP;
}

static inline void flash_end(void)
{
    FLASH->CR = FLASH_CR_LOCK;
    uint32_t acr = FLASH->ACR;
    FLASH->ACR = acr & ~(FLASH_ACR_ICEN | FLASH_ACR_DCEN);
    FLASH->ACR |= FLASH_ACR_ICRST | FLASH_ACR_DCRST;
    FLASH->ACR = acr;
    __DSB();
    __ISB();
}

static inline bool flash_erase(uint32_t addr)
{
    FLASH->SR = FLASH_ERRORS | FLASH_SR_EOP;
    FLASH->CR = FLASH_CR_PER | (((addr - FLASH_BASE) / LYKE_PAGE) << FLASH_CR_PNB_Pos);
    FLASH->CR |= FLASH_CR_STRT;
    flash_wait();
    FLASH->CR = 0;
    return !(FLASH->SR & FLASH_ERRORS);
}

static inline bool flash_write(uint32_t addr, uint32_t low, uint32_t high)
{
    FLASH->SR = FLASH_ERRORS | FLASH_SR_EOP;
    FLASH->CR = FLASH_CR_PG;
    *(volatile uint32_t *)addr = low;
    __ISB();
    *(volatile uint32_t *)(addr + 4) = high;
    flash_wait();
    FLASH->CR = 0;
    return !(FLASH->SR & FLASH_ERRORS);
}

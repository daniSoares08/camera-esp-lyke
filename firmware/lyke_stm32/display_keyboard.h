#include "device.h"
#include "../stm32_teclado/scan.h"
#include "pins.h"
#include "../calculadora/common/font5x7.h"

static volatile uint32_t ticks;

static uint8_t pixels[8][128];
// N?vel 5 escolhido na calibra??o da R3: raz?o 6 e volume eletr?nico 0x14.
static const uint8_t lcd_contrast[] = { 0x26, 0x81, 0x14 };

void SystemInit(void)
{
    SCB->VTOR = LYKE_APP;
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY)) {}
    RCC->CFGR = RCC_CFGR_SW_HSI;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI) {}
}

void __libc_init_array(void) {}

void SysTick_Handler(void)
{
    ++ticks;
}

static void wait_ms(uint32_t duration)
{
    uint32_t start = ticks;
    while (ticks - start < duration)
        __WFI();
}

static void settle(void)
{
    const uint32_t start = DWT->CYCCNT;
    while (DWT->CYCCNT - start < 16 * 45) {}
}

static void pin_mode(Pin pin, uint32_t mode)
{
    const unsigned shift = 2 * pin.bit;
    pin.port->MODER = (pin.port->MODER & ~(3u << shift)) | (mode << shift);
}

static void pin_set(Pin pin, bool high)
{
    pin.port->BSRR = 1u << (pin.bit + (high ? 0 : 16));
}

static void pin_alternate(Pin pin, uint32_t function)
{
    unsigned index = pin.bit / 8;
    unsigned shift = 4 * (pin.bit % 8);
    pin.port->AFR[index] = (pin.port->AFR[index] & ~(15u << shift)) | (function << shift);
    pin_mode(pin, 2);
}

static void keyboard_init(void)
{
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    (void)RCC->APB1ENR1;
    PWR->CR3 |= PWR_CR3_UCPD_DBDIS;
    for (unsigned i = 0; i < TRACK_COUNT; ++i) {
        Pin pin = tracks[i];
        unsigned shift = 2 * pin.bit;
        pin_mode(pin, 0);
        pin.port->OTYPER |= 1u << pin.bit;
        pin.port->PUPDR = (pin.port->PUPDR & ~(3u << shift)) | (1u << shift);
        pin_set(pin, false);
    }
}

static uint32_t sample_low(void)
{
    const uint32_t a = GPIOA->IDR;
    const uint32_t b = GPIOB->IDR;
    const uint32_t c = GPIOC->IDR;
    uint32_t low = 0;
    for (unsigned i = 0; i < TRACK_COUNT; ++i) {
        Pin pin = tracks[i];
        uint32_t value = pin.port == GPIOA ? a : (pin.port == GPIOB ? b : c);
        if ((value & (1u << pin.bit)) == 0)
            low |= 1u << i;
    }
    return low;
}

static Contact keyboard_scan(void)
{
    uint32_t idle_low = sample_low();
    if (idle_low)
        return (Contact){ LOW_AT_REST, __builtin_ctz(idle_low), 0 };

    uint32_t hits[TRACK_COUNT];
    for (unsigned i = 0; i < TRACK_COUNT; ++i) {
        // A ?nica sa?da ativa ? dreno aberto em LOW. As outras 18 s?o entradas.
        pin_mode(tracks[i], 1);
        settle();
        hits[i] = sample_low() & ~(1u << i);
        pin_mode(tracks[i], 0);
        settle();
    }
    return decode_contacts(hits);
}

static void lcd_transfer(bool data, const uint8_t *buffer, unsigned size)
{
    pin_set(lcd_rs, data);
    pin_set(lcd_cs, false);
    while (size--) {
        while (!(SPI1->SR & SPI_SR_TXE)) {}
        *(volatile uint8_t *)&SPI1->DR = *buffer++;
    }
    while (!(SPI1->SR & SPI_SR_TXE)) {}
    while (SPI1->SR & SPI_SR_BSY) {}
    pin_set(lcd_cs, true);
}

static void lcd_init(void)
{
    pin_set(lcd_cs, true);
    pin_set(lcd_rse, false);
    pin_set(lcd_rs, false);
    pin_mode(lcd_cs, 1);
    pin_mode(lcd_rse, 1);
    pin_mode(lcd_rs, 1);
    pin_alternate(lcd_scl, 5);
    pin_alternate(lcd_si, 5);

    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    (void)RCC->APB2ENR;
    SPI1->CR2 = 7u << SPI_CR2_DS_Pos;
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_BR_2 | SPI_CR1_SSM | SPI_CR1_SSI |
                SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE;
    SPI1->CR1 |= SPI_CR1_SPE;

    wait_ms(15);
    pin_set(lcd_rse, true);
    wait_ms(25);
    const uint8_t init[] = { 0xe2, 0xae, 0x40, 0xa0, 0xc8, 0xa6, 0xa2, 0x2f, 0xa4 };
    lcd_transfer(false, init, sizeof(init));
    lcd_transfer(false, lcd_contrast, sizeof(lcd_contrast));
    const uint8_t on = 0xaf;
    lcd_transfer(false, &on, 1);
    wait_ms(40);
}

static void lcd_show(void)
{
    for (uint8_t page = 0; page < 8; ++page) {
        const uint8_t address[] = { (uint8_t)(0xb0 | page), 0x10, 0x00 };
        lcd_transfer(false, address, sizeof(address));
        lcd_transfer(true, pixels[page], sizeof(pixels[page]));
    }
}



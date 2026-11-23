#include "display_keyboard.h"
#include "keys.h"
#include <stdarg.h>
#include <string.h>

enum { ROWS = 6, COLS = 21, X_FIRST = 2, PAGE_FIRST = 2 };
_Static_assert(PAGE_FIRST + ROWS == 8 && X_FIRST + COLS * 6 == 128, "LCD viewport reaches bottom and right");
_Static_assert(KEY_COUNT == 50, "Lyke 101078 tiene 50 teclas");
#define KEY_TEXT(id, label) label,
static const char *const key_label[] = { LYKE_KEYS(KEY_TEXT) };
#undef KEY_TEXT

static Pair keymap[KEY_COUNT];
static unsigned mapped, flash_position;
static const char *map_status = "UMA TECLA POR VEZ";
static bool map_press_pending;
static bool repair_on_release;
static int duplicate_key = -1, repair_key = -1;
static Contact previous;
static char incoming[192];
static unsigned incoming_length;
static bool incoming_overflow;
static uint8_t rx_ring[512];
static volatile unsigned rx_read, rx_write;
static volatile bool rx_overflow;
static const Pair skipped_on = { TRACK_COUNT, TRACK_COUNT };

// The LCD needs only strings and unsigned decimal fields.
static void print_text(char *out, size_t capacity, const char *format, ...)
{
    if (!capacity) return;
    char *end = out + capacity - 1;
    va_list arguments;
    va_start(arguments, format);
    while (*format && out < end) {
        char current = *format++;
        if (current != '%') { *out++ = current; continue; }
        if (*format == 's') {
            const char *text = va_arg(arguments, const char *);
            while (*text && out < end) *out++ = *text++;
        } else {
            unsigned minimum = 1;
            if (format[0] == '0' && format[1] == '2') { minimum = 2; format += 2; }
            unsigned value = va_arg(arguments, unsigned);
            char reverse[10];
            unsigned digits = 0;
            do { reverse[digits++] = '0' + value % 10; value /= 10; } while (value || digits < minimum);
            while (digits && out < end) *out++ = reverse[--digits];
        }
        ++format;
    }
    *out = 0;
    va_end(arguments);
}

static bool has_on(void)
{
    return mapped > KEY_ON && keymap[KEY_ON].first < TRACK_COUNT;
}

static unsigned mapped_contacts(void)
{
    return mapped - (mapped > KEY_ON && !has_on());
}

static void report_map(void)
{
    char state[12];
    print_text(state, sizeof(state), "MAP %u %u", mapped_contacts(), (unsigned)(mapped == KEY_COUNT));
    serial_send(state);
}

void USART2_IRQHandler(void)
{
    for (int c; (c = serial_get()) >= 0;) {
        unsigned next = (rx_write + 1) % sizeof(rx_ring);
        if (next == rx_read) {
            rx_overflow = true;
            continue;
        }
        rx_ring[rx_write] = (uint8_t)c;
        rx_write = next;
    }
}

static void line(unsigned index, const char *label, bool inverse)
{
    if (index >= ROWS)
        return;
    uint8_t *row = pixels[7 - PAGE_FIRST - index];
    memset(row, inverse ? 0xff : 0, 128);
    for (unsigned i = 0; i < COLS && label[i]; ++i) {
        uint8_t code = (uint8_t)label[i] - 32;
        static const uint8_t pi[] = { 1, 0x7f, 1, 0x3f, 0x41 };
        const uint8_t *glyph = (uint8_t)label[i] == 0x7f
            ? pi : font5x7[code < 95 ? code : '?' - 32];
        for (unsigned c = 0; c < 5; ++c) {
            uint8_t column = (uint8_t)(__RBIT((uint32_t)glyph[c]) >> 24);
            row[127 - (X_FIRST + i * 6 + c)] = inverse ? (uint8_t)~column : column;
        }
    }
}

static int key_index(Pair pair)
{
    for (unsigned i = 0; i < mapped; ++i)
        if (keymap[i].first == pair.first && keymap[i].second == pair.second)
            return (int)i;
    return -1;
}

static uint32_t key_record(Pair pair)
{
    return 0x4b590000u | (mapped << 10) | ((uint32_t)pair.first << 5) | pair.second;
}

// A repair record identifies the earlier slot holding the physical equals
// contact and includes the replacement contact for that slot.
enum { REPAIR_MAGIC = 0x4b5a0000u };

static void accept_key(Pair pair, int replaced_key)
{
    if (replaced_key >= 0) {
        keymap[mapped++] = keymap[replaced_key];
        keymap[replaced_key] = pair;
    } else {
        keymap[mapped++] = pair;
    }
}

static void load_map(void)
{
    const uint32_t *records = (const uint32_t *)LYKE_KEYMAP;
    for (unsigned offset = 0; offset < LYKE_PAGE && mapped < KEY_COUNT; offset += 8) {
        uint32_t value = records[offset / 4], inverse = records[offset / 4 + 1];
        if (value == UINT32_MAX && inverse == UINT32_MAX)
            continue;
        flash_position = offset + 8;
        Pair pair = { (value >> 5) & 31u, value & 31u };
        bool skip = mapped == KEY_ON && pair.first == TRACK_COUNT && pair.second == TRACK_COUNT;
        bool contact = pair.first < pair.second && pair.second < TRACK_COUNT && key_index(pair) < 0;
        if (inverse != ~value)
            continue;
        int replaced = (value >> 10) & 63u;
        bool repair = mapped == KEY_EQ && replaced < KEY_EQ && replaced != KEY_ON &&
            contact && (value & 0xffff0000u) == REPAIR_MAGIC;
        if (repair || (value == key_record(pair) && (skip || contact)))
            accept_key(pair, repair ? replaced : -1);
    }
}

static bool save_key(Pair pair)
{
    if (flash_position >= LYKE_PAGE)
        return false;
    uint32_t value = repair_key < 0 ? key_record(pair) :
        (REPAIR_MAGIC | ((uint32_t)repair_key << 10) | ((uint32_t)pair.first << 5) | pair.second);
    flash_begin();
    bool ok = flash_write(LYKE_KEYMAP + flash_position, value, ~value);
    flash_end();
    flash_position += 8;
    if (ok)
        accept_key(pair, repair_key);
    return ok;
}

static void advance_over_on(void)
{
    if (mapped == KEY_ON && !save_key(skipped_on))
        map_status = "ERRO AO PULAR ON";
}

static void draw_mapper(void)
{
    char step[] = "TECLA 00 DE 00";
    unsigned number = mapped_contacts() + 1, total = KEY_COUNT - !has_on();
    step[6] = '0' + number / 10;
    step[7] = '0' + number % 10;
    step[12] = '0' + total / 10;
    step[13] = '0' + total % 10;
    line(0, repair_key >= 0 ? "CORRIGIR TECLA E =" : "MAPEAR TECLADO", true);
    line(1, step, false);
    line(2, "APERTE E SOLTE", false);
    unsigned requested = repair_key >= 0 ? (unsigned)repair_key : mapped;
    line(3, key_label[requested], true);
    line(4, map_status, false);
    char detail[COLS + 1];
    if (duplicate_key >= 0) {
        print_text(detail, sizeof(detail), "JA E: %s", key_label[duplicate_key]);
        line(5, detail, false);
    } else {
        line(5, "", false);
    }
}

static void handle_mapper(Contact contact)
{
    if (mapped == KEY_ON)
        return; // Only the skip record may occupy this slot.
    if (contact.kind == IDLE) {
        if (repair_on_release) {
            repair_key = duplicate_key;
            duplicate_key = -1;
            map_status = "IGUAL JA IDENTIFICADO";
        } else if (map_press_pending) {
            bool ok = save_key((Pair){ previous.first, previous.second });
            map_status = ok ? "UMA TECLA POR VEZ" : "ERRO AO GRAVAR";
            if (ok) {
                advance_over_on();
                report_map();
            }
        }
        map_press_pending = false;
        repair_on_release = false;
    } else if (contact.kind == CONTACT && previous.kind == IDLE) {
        int found = key_index((Pair){ contact.first, contact.second });
        map_press_pending = found < 0;
        repair_on_release = mapped == KEY_EQ && found >= 0 && repair_key < 0;
        duplicate_key = found;
        map_status = found < 0 ? "SOLTE A TECLA" : repair_on_release ? "SOLTE PARA CORRIGIR" : "TECLA JA MAPEADA";
    } else {
        duplicate_key = -1;
        map_press_pending = false;
        repair_on_release = false;
        map_status = contact.kind == LOW_AT_REST ? "VERIFIQUE OS FIOS" : "SOLTE TODAS";
    }
    previous = contact;
}

#include "interface.h"

static bool service_serial(void)
{
    bool redraw = false;
    if (rx_overflow) {
        incoming_overflow = true;
        rx_overflow = false;
    }
    for (unsigned budget = 0; budget < 192 && rx_read != rx_write; ++budget) {
        int c = rx_ring[rx_read];
        rx_read = (rx_read + 1) % sizeof(rx_ring);
        if (c == '\r')
            continue;
        if (c != '\n') {
            if (incoming_length < sizeof(incoming) - 1)
                incoming[incoming_length++] = (char)c;
            else
                incoming_overflow = true;
            continue;
        }
        incoming[incoming_length] = 0;
        if (!incoming_overflow && strcmp(incoming, "LYKE-BOOT-V1") == 0) {
            LYKE_REQUEST = LYKE_BOOT_REQUEST;
            NVIC_SystemReset();
        }
        if (!incoming_overflow && strcmp(incoming, "LYKE-HELLO") == 0) {
            serial_send("LYKE APP V1");
            report_map();
        }
        if (!incoming_overflow && mapped == KEY_COUNT)
            redraw |= interface_message(incoming);
        incoming_length = 0;
        incoming_overflow = false;
    }
    return redraw;
}

static void redraw_screen(void)
{
    if (mapped < KEY_COUNT)
        draw_mapper();
    else
        interface_draw();
    lcd_show();
}

int main(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN;
    (void)RCC->AHB2ENR;
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    SysTick_Config(16000);
    keyboard_init();
    serial_init();
    USART2->CR1 |= USART_CR1_RXNEIE_RXFNEIE;
    NVIC_EnableIRQ(USART2_IRQn);
    lcd_init();
    load_map();
    advance_over_on();
    load_settings();
    serial_send("LYKE APP V1");
    report_map();
    redraw_screen();
    Contact pending = { MULTIPLE, 0, 0 }, stable = pending;
    previous = pending;
    uint32_t changed_at = ticks, last_scan = ticks;
    for (;;) {
        bool redraw = service_serial();
        if (ticks - last_scan >= 4) {
            last_scan = ticks;
            Contact reading = keyboard_scan();
            if (!contact_equal(reading, pending)) {
                pending = reading;
                changed_at = ticks;
            }
            if (!contact_equal(pending, stable) && ticks - changed_at >= 35) {
                bool single = stable.kind == IDLE && pending.kind == CONTACT;
                stable = pending;
                if (mapped < KEY_COUNT) {
                    handle_mapper(stable);
                    if (mapped == KEY_COUNT)
                        interface_mapped();
                } else if (single) {
                    int idx = key_index((Pair){ stable.first, stable.second });
                    if (idx >= 0)
                        interface_key((enum Key)idx);
                }
                redraw = true;
            }
            redraw |= interface_tick();
        }
        if (redraw)
            redraw_screen();
        if (rx_read == rx_write)
            __WFI();
    }
}

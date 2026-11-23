#include "device.h"

enum { CHUNK = 256 };
#define META_MAGIC 0x31594c43u

typedef struct {
    uint32_t size, crc, magic, inverse;
} Image;

void SystemInit(void) {}
void __libc_init_array(void) {}

static uint32_t crc32(uint32_t adr, uint32_t size)
{
    const volatile uint8_t *source = (const volatile uint8_t *)adr;
    uint32_t crc = UINT32_MAX;
    while (size--) {
        crc ^= *source++;
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

static bool vector_valid(uint32_t address, uint32_t size)
{
    const volatile uint32_t *vector = (const volatile uint32_t *)address;
    return vector[0] == 0x200057f0u && (vector[1] & 1) &&
           vector[1] >= LYKE_APP + 9 && vector[1] - LYKE_APP < size;
}

static bool size_valid(uint32_t size)
{
    return size >= 512 && size <= LYKE_MAX_APP;
}

static bool install(const Image *image)
{
    if (image->magic != META_MAGIC || image->inverse != ~META_MAGIC ||
        !size_valid(image->size) || !vector_valid(LYKE_STAGE, image->size))
        return false;
    if (vector_valid(LYKE_APP, image->size) && crc32(LYKE_APP, image->size) == image->crc)
        return true;
    if (crc32(LYKE_STAGE, image->size) != image->crc)
        return false;

    flash_begin();
    bool ok = true;
    for (unsigned offset = 0; ok && offset < image->size; offset += LYKE_PAGE)
        ok = flash_erase(LYKE_APP + offset);
    for (unsigned offset = 8; ok && offset < image->size; offset += 8) {
        const uint32_t *p = (const uint32_t *)(LYKE_STAGE + offset);
        ok = flash_write(LYKE_APP + offset, p[0], p[1]);
    }
    // Publish the vectors last. Until then no reset can enter the incomplete app.
    const uint32_t *vectors = (const uint32_t *)LYKE_STAGE;
    if (ok)
        ok = flash_write(LYKE_APP, vectors[0], vectors[1]);
    flash_end();
    return ok && crc32(LYKE_APP, image->size) == image->crc;
}

static int receive(uint32_t period_ms)
{
    const uint32_t start = DWT->CYCCNT;
    do {
        int c = serial_get();
        if (c >= 0)
            return c;
    } while (DWT->CYCCNT - start < period_ms * 16000u);
    return -1;
}

static bool parse_hex(const char *p, uint32_t *value)
{
    *value = 0;
    for (unsigned i = 0; i < 8; ++i) {
        unsigned c = (uint8_t)p[i];
        if (c >= '0' && c <= '9') c -= '0';
        else if (c >= 'a' && c <= 'f') c -= 'a' - 10;
        else return false;
        *value = (*value << 4) | c;
    }
    return true;
}

static bool update(const char *line)
{
    Image image = { 0, 0, META_MAGIC, ~META_MAGIC };
    if (line[8] != ' ' || !parse_hex(line, &image.size) ||
        !parse_hex(line + 9, &image.crc) || !size_valid(image.size)) {
        serial_send("LYKE BAD_HEADER");
        return false;
    }

    flash_begin();
    bool ok = flash_erase(LYKE_META);
    for (unsigned offset = 0; ok && offset < image.size; offset += LYKE_PAGE)
        ok = flash_erase(LYKE_STAGE + offset);
    serial_send(ok ? "LYKE READY_DATA" : "LYKE ERASE_ERROR");
    for (unsigned offset = 0; ok && offset < image.size; offset += CHUNK) {
        uint32_t words[CHUNK / 4];
        unsigned n = image.size - offset;
        if (n > CHUNK) n = CHUNK;
        uint8_t *bytes = (uint8_t *)words;
        for (unsigned i = 0; i < ((n + 7u) & ~7u); ++i) {
            int c = i < n ? receive(2000) : 0xff;
            if (c < 0) { ok = false; break; }
            bytes[i] = (uint8_t)c;
        }
        for (unsigned i = 0; ok && i < n; i += 8)
            ok = flash_write(LYKE_STAGE + offset + i, words[i/4], words[i/4+1]);
        if (ok)
            serial_send("LYKE ACK");
    }
    flash_end();

    ok = ok && vector_valid(LYKE_STAGE, image.size) && crc32(LYKE_STAGE, image.size) == image.crc;
    if (ok) {
        flash_begin();
        ok = flash_write(LYKE_META, image.size, image.crc) &&
             flash_write(LYKE_META + 8, image.magic, image.inverse);
        flash_end();
    }
    if (ok)
        ok = install(&image);
    serial_send(ok ? "LYKE INSTALLED" : "LYKE VERIFY_ERROR");
    if (ok) {
        while (!(USART2->ISR & USART_ISR_TC)) {}
        NVIC_SystemReset();
    }
    return false;
}

__attribute__((noreturn, naked)) static void jump(uint32_t stack __attribute__((unused)),
                                                 uint32_t start __attribute__((unused)))
{
    __asm volatile ("msr msp, r0\n bx r1");
}

int main(void)
{
    bool requested = LYKE_REQUEST == LYKE_BOOT_REQUEST;
    LYKE_REQUEST = 0;
    const Image *image = (const Image *)LYKE_META;
    bool app_ok = image->magic == META_MAGIC && image->inverse == ~META_MAGIC
        ? install(image) : vector_valid(LYKE_APP, LYKE_MAX_APP);

    if (!requested && app_ok)
        jump(*(volatile uint32_t *)LYKE_APP, *(volatile uint32_t *)(LYKE_APP + 4));

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    serial_init();
    for (;;) {
        serial_send("LYKE BOOT");
        const uint32_t started = DWT->CYCCNT;
        char line[24];
        unsigned length = 0;
        bool overlong = false;
        while (DWT->CYCCNT - started < 16000u * 1500u) {
            int c = receive(25);
            if (c < 0 || c == '\r') continue;
            if (c != '\n') {
                if (length < sizeof(line)) line[length++] = (char)c;
                else overlong = true;
                continue;
            }
            if (!overlong && length == 23 && line[0] == 'F' && line[1] == 'L' &&
                line[2] == 'A' && line[3] == 'S' && line[4] == 'H' && line[5] == ' ') {
                update(line + 6);
                NVIC_SystemReset();
            }
            length = 0;
            overlong = false;
        }
        if (app_ok)
            NVIC_SystemReset();
    }
}

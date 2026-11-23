#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "vendor/puff.h"
#ifdef __cplusplus
}
#endif

#define LYKE_PACK_HEADER 24u
#define LYKE_PACK_BLOCK 4096u
#define LYKE_PACK_LIMIT (48u * 1024u)
#define LYKE_PACK_APP_LIMIT (64u * 1024u)

static uint32_t image_word(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}

static uint32_t image_crc_update(uint32_t crc, const uint8_t *p, size_t size)
{
    while (size--) {
        crc ^= *p++;
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
    return crc;
}

static bool image_header_valid(const uint8_t *p, size_t size)
{
    return size >= LYKE_PACK_HEADER + 4 && size <= LYKE_PACK_LIMIT &&
        !memcmp(p, "LYKEG432", 8) && image_word(p+8) >= 512 &&
        image_word(p+8) <= LYKE_PACK_APP_LIMIT && image_word(p+16) == size-LYKE_PACK_HEADER &&
        ~image_crc_update(UINT32_MAX, p+LYKE_PACK_HEADER, size-LYKE_PACK_HEADER) == image_word(p+20);
}

// Caller supplies exactly one scratch block; neither decoder nor format allocates.
static bool image_next_block(const uint8_t *p, size_t size, size_t *at,
                             uint8_t *scratch, unsigned *plain)
{
    if (*at + 4 > size) return false;
    unsigned encoded = p[*at] | (unsigned)p[*at+1]<<8;
    *plain = p[*at+2] | (unsigned)p[*at+3]<<8;
    *at += 4;
    if (!*plain || *plain > LYKE_PACK_BLOCK || *at + encoded > size) return false;
    unsigned long source = encoded, destination = *plain;
    if (puff(scratch, &destination, p+*at, &source) || destination != *plain || source != encoded)
        return false;
    *at += encoded;
    return true;
}

static bool image_validate(const uint8_t *p, size_t size, uint8_t *scratch)
{
    if (!image_header_valid(p, size)) return false;
    size_t at = LYKE_PACK_HEADER;
    unsigned total = 0, plain;
    uint32_t crc = UINT32_MAX;
    while (at < size) {
        if (!image_next_block(p, size, &at, scratch, &plain)) return false;
        if (!total) {
            if (plain < 8) return false;
            uint32_t pc = image_word(scratch+4);
            if (image_word(scratch) != 0x200057f0u || !(pc & 1) ||
                pc < 0x08002009u || pc - 0x08002000u >= image_word(p+8)) return false;
        }
        total += plain;
        if (total > image_word(p+8)) return false;
        // All non-final blocks must align flash doubleword writes.
        if (at < size && plain % 8) return false;
        crc = image_crc_update(crc, scratch, plain);
    }
    return total == image_word(p+8) && ~crc == image_word(p+12);
}

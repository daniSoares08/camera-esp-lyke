#pragma once
#include "image_format.h"

#define META_MAGIC 0x32594c43u
typedef struct { uint32_t size, crc, magic, inverse; } Image;
static uint8_t image_scratch[LYKE_PACK_BLOCK] __attribute__((aligned(8)));

static uint32_t crc32(uint32_t address, uint32_t size)
{
    return ~image_crc_update(UINT32_MAX, (const uint8_t *)(uintptr_t)address, size);
}

static bool size_valid(uint32_t size) { return size >= 28 && size <= LYKE_PACK_LIMIT; }

static bool vector_valid(uint32_t address, uint32_t size)
{
    const uint8_t *p = (const uint8_t *)(uintptr_t)address;
    uint32_t pc = image_word(p+4);
    return image_word(p) == 0x200057f0u && (pc & 1) &&
           pc >= LYKE_APP + 9 && pc - LYKE_APP < size;
}

static bool install(const Image *image)
{
    const uint8_t *p = (const uint8_t *)(uintptr_t)LYKE_STAGE;
    if (image->magic != META_MAGIC || image->inverse != (uint32_t)~META_MAGIC ||
        !size_valid(image->size) || crc32(LYKE_STAGE, image->size) != image->crc ||
        !image_validate(p, image->size, image_scratch)) return false;
    const uint32_t size = image_word(p+8), crc = image_word(p+12);
    if (vector_valid(LYKE_APP, size) && crc32(LYKE_APP, size) == crc) return true;
    flash_begin();
    bool ok = true;
    for (unsigned offset = 0; ok && offset < size; offset += LYKE_PAGE)
        ok = flash_erase(LYKE_APP + offset);
    size_t at = LYKE_PACK_HEADER;
    unsigned offset = 0, plain = 0;
    uint32_t vectors[2] = {0};
    while (ok && at < image->size) {
        ok = image_next_block(p, image->size, &at, image_scratch, &plain);
        if (!ok) break;
        if (!offset) memcpy(vectors, image_scratch, 8);
        for (unsigned i = offset ? 0 : 8; ok && i < plain; i += 8) {
            uint8_t words[8];
            memset(words, 0xff, 8);
            unsigned n = plain-i < 8 ? plain-i : 8;
            memcpy(words, image_scratch+i, n);
            ok = flash_write(LYKE_APP+offset+i, image_word(words), image_word(words+4));
        }
        offset += plain;
    }
    // Vectors become valid only after the whole application has been written.
    if (ok) ok = flash_write(LYKE_APP, vectors[0], vectors[1]);
    flash_end();
    return ok && crc32(LYKE_APP, size) == crc;
}

#pragma once
#include "glyphs.h"

static void line(unsigned index, const char *label, bool inverse)
{
    if (index >= ROWS)
        return;
    uint8_t *row = pixels[7 - PAGE_FIRST - index];
    memset(row, inverse ? 0xff : 0, 128);
    for (unsigned i = 0; i < COLS && label[i]; ++i) {
        const uint8_t *glyph = calc_glyph((uint8_t)label[i]);
        for (unsigned c = 0; c < 5; ++c) {
            uint8_t column = (uint8_t)(__RBIT((uint32_t)glyph[c]) >> 24);
            row[127 - (X_FIRST + i * 6 + c)] = inverse ? (uint8_t)~column : column;
        }
    }
}

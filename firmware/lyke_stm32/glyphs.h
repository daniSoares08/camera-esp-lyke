#pragma once
#include "../calculadora/common/font5x7.h"
enum { GLYPH_TIMES=27, GLYPH_DELTA=28, GLYPH_THETA=29, GLYPH_OMEGA=30, GLYPH_DIVIDE=31, GLYPH_PI=127 };
static const uint8_t *calc_glyph(unsigned code)
{
    static const uint8_t extra[][5] = {
        {0x22,0x14,0x08,0x14,0x22}, {0x70,0x4c,0x43,0x4c,0x70},
        {0x3e,0x49,0x49,0x49,0x3e}, {0x5e,0x61,0x01,0x61,0x5e},
        {0x08,0x08,0x2a,0x08,0x08}, {1,0x7f,1,0x3f,0x41}
    };
    if (code>=GLYPH_TIMES && code<=GLYPH_DIVIDE) return extra[code-GLYPH_TIMES];
    if (code==GLYPH_PI) return extra[5];
    return font5x7[code>=32 && code<=126 ? code-32 : '?' -32];
}

#pragma once
#include "glyphs.h"
static const char *token_text(unsigned token)
{
    static const char *const names[] = {
        "\x7f","e","Ans","^-1","^2","^3","!","%","/","DMS"," nCr "," nPr "," root ",
        "sqrt(","cbrt(","log(","ln(","10^(","exp(","sin(","cos(","tan(","asin(","acos(","atan(",
        "sinh(","cosh(","tanh(","asinh(","acosh(","atanh(","Pol(","Rec(",
        "A","B","C","D","E","F","X","Y","M","Rnd(","Ran#","\x1c","\x1d","\x1e"
    };
    return token>=PI && token<=GREEK_OMEGA ? names[token-PI] : "?";
}

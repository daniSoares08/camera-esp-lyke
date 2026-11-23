#pragma once

#define LYKE_KEYS(KEY) \
    KEY(SHIFT, "SHIFT") KEY(ALPHA, "ALPHA") KEY(UP, "REPLAY CIMA") \
    KEY(LEFT, "REPLAY ESQUERDA") KEY(RIGHT, "REPLAY DIREITA") KEY(DOWN, "REPLAY BAIXO") \
    KEY(MODE, "MODE") KEY(ON, "ON") \
    KEY(INV, "X^-1") KEY(NCR, "nCr") KEY(POL, "Pol(") KEY(CUBE, "X^3") \
    KEY(FRAC, "ab/c") KEY(SQRT, "RAIZ QUADRADA") KEY(SQUARE, "X^2") \
    KEY(POWER, "^ (POTENCIA)") KEY(LOG, "log") KEY(LN, "ln") \
    KEY(NEG, "(-) SINAL") KEY(DMS, "GRAUS \" '") KEY(HYP, "hyp") \
    KEY(SIN, "sin") KEY(COS, "cos") KEY(TAN, "tan") \
    KEY(RCL, "RCL") KEY(ENG, "ENG") KEY(LPAREN, "(") KEY(RPAREN, ")") \
    KEY(COMMA, ", (VIRGULA)") KEY(MPLUS, "M+") \
    KEY(D7, "7") KEY(D8, "8") KEY(D9, "9") KEY(DEL, "DEL") KEY(AC, "AC") \
    KEY(D4, "4") KEY(D5, "5") KEY(D6, "6") KEY(MUL, "X (MULTIPLICAR)") KEY(DIV, "/ (DIVIDIR)") \
    KEY(D1, "1") KEY(D2, "2") KEY(D3, "3") KEY(ADD, "+") KEY(SUB, "- (SUBTRAIR)") \
    KEY(D0, "0") KEY(DOT, ". (PONTO)") KEY(EXP, "EXP") KEY(ANS, "Ans") KEY(EQ, "=")

#define KEY_ENUM(id, label) KEY_##id,
enum Key { LYKE_KEYS(KEY_ENUM) KEY_COUNT };
#undef KEY_ENUM

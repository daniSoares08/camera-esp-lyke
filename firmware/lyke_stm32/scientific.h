#pragma once

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

enum Symbol {
    PI = 128, EULER, ANS, RECIP, SQUARE, CUBE, FACT, PCT, FRAC, DMS,
    NCR, NPR, ROOT, SQRT, CBRT, LOG10, LN, POW10, EXP,
    SIN, COS, TAN, ASIN, ACOS, ATAN, SINH, COSH, TANH, ASINH, ACOSH, ATANH,
    POL, REC, VAR_A, VAR_B, VAR_C, VAR_D, VAR_E, VAR_F, VAR_X, VAR_Y, VAR_M, RND, RANDOM
};

static const double pi = 3.14159265358979323846;

typedef struct {
    const uint8_t *at;
    const char *error;
    unsigned depth;
    double answer, angle, secondary;
    const double *variables;
    bool two_values;
} Expression;

static double bad(Expression *expr, const char *error)
{
    if (!expr->error)
        expr->error = error;
    return 0;
}

static bool consume(Expression *expr, unsigned token)
{
    if (*expr->at != token)
        return false;
    ++expr->at;
    return true;
}

static double function(Expression *expr, unsigned symbol, double x)
{
    const double radians = x * expr->angle;
    switch (symbol) {
    case SQRT: return sqrt(x);
    case CBRT: return cbrt(x);
    case LOG10: return log10(x);
    case LN: return log(x);
    case POW10: return pow(10.0, x);
    case EXP: return exp(x);
    case SIN: return sin(radians);
    case COS: return cos(radians);
    case TAN: return fabs(cos(radians)) < 1e-12 ? bad(expr, "TANGENTE INDEFIN.") : tan(radians);
    case ASIN: return atan2(x, sqrt((1 - x) * (1 + x))) / expr->angle;
    case ACOS: return atan2(sqrt((1 - x) * (1 + x)), x) / expr->angle;
    case ATAN: return atan(x) / expr->angle;
    case SINH: return sinh(x);
    case COSH: return cosh(x);
    case TANH: return tanh(x);
    case ASINH: return asinh(x);
    case ACOSH: return acosh(x);
    case ATANH: return atanh(x);
    case RND: {
        if (x == 0) return 0;
        double quantum = pow(10.0, floor(log10(fabs(x))) - 9);
        return quantum > 0 ? round(x / quantum) * quantum : x;
    }
    default: return bad(expr, "FUNCAO INVALIDA");
    }
}

static double compare_int(Expression *expr, double n, double r, bool permutation)
{
    if (r < 0 || r > n || n > 1e9 || floor(n) != n || floor(r) != r)
        return bad(expr, "N E R INTEIROS");
    if (!permutation && r > n - r)
        r = n - r;
    if (r > 2048)
        return bad(expr, "FORA DA FAIXA");
    double value = 1;
    for (unsigned i = 1; i <= (unsigned)r && isfinite(value); ++i)
        value = (value / (permutation ? 1 : i)) * (n - (r - i));
    return value;
}

static double factorial(Expression *expr, double value)
{
    if (value < 0 || value > 170 || !isfinite(value) || floor(value) != value)
        return bad(expr, "FATORIAL: 0 A 170");
    double result = 1;
    for (unsigned i = 2; i <= (unsigned)value; ++i)
        result *= i;
    return result;
}

static double expr_sum(Expression *expr);

static bool digit(unsigned c)
{
    return c >= '0' && c <= '9';
}

static double expr_number(Expression *expr)
{
    double value = 0;
    int scale = 0;
    bool point = false, any = false;
    for (;;) {
        unsigned c = *expr->at;
        if (c == '.' && !point) point = true;
        else if (digit(c)) {
            any = true;
            value = 10 * value + (c - '0');
            scale -= point;
        } else break;
        ++expr->at;
    }
    if (!any) return bad(expr, "NUMERO INVALIDO");
    if (consume(expr, 'E')) {
        bool negative = consume(expr, '-');
        if (!negative) consume(expr, '+');
        if (!digit(*expr->at)) return bad(expr, "EXPOENTE INVALIDO");
        int exponent = 0;
        while (digit(*expr->at)) {
            if (exponent < 1000) exponent = exponent * 10 + *expr->at - '0';
            ++expr->at;
        }
        scale += negative ? -exponent : exponent;
    }
    if (!value) return 0;
    if (scale > 308) return bad(expr, "FORA DA FAIXA");
    if (scale < -420) return 0;
    if (scale < -300) { value /= 1e300; scale += 300; }
    return scale < 0 ? value / pow(10.0, -scale) : value * pow(10.0, scale);
}

static double expr_atom(Expression *expr)
{
    const uint8_t c = *expr->at;
    if (digit(c) || c == '.') return expr_number(expr);
    if (!c)
        return bad(expr, "FALTA UM VALOR");
    ++expr->at;
    if (c >= VAR_A && c <= VAR_M)
        return expr->variables[c - VAR_A];
    if (c == PI) return pi;
    if (c == EULER) return 2.71828182845904523536;
    if (c == ANS) return expr->answer;
    if (c == RANDOM) return (double)(rand() % 1000) / 1000;
    if (c != '(' && !(c >= SQRT && c <= REC) && c != RND)
        return bad(expr, "FALTA UM VALOR");

    double x = expr_sum(expr);
    if (c == POL || c == REC) {
        if (!consume(expr, ','))
            return bad(expr, "USE 2 VALORES");
        double y = expr_sum(expr);
        expr->two_values = true;
        if (c == POL) {
            expr->secondary = atan2(y, x) / expr->angle;
            x = hypot(x, y);
        } else {
            expr->secondary = x * sin(y * expr->angle);
            x *= cos(y * expr->angle);
        }
    } else if (c != '(') {
        x = function(expr, c, x);
    }
    if (!consume(expr, ')') && *expr->at)
        return bad(expr, "FECHE PARENTESES");
    return x;
}

static double expr_postfix(Expression *expr)
{
    double value = expr_atom(expr);
    while (!expr->error) {
        if (consume(expr, SQUARE)) value *= value;
        else if (consume(expr, CUBE)) value = value * value * value;
        else if (consume(expr, RECIP)) value = 1 / value;
        else if (consume(expr, FACT)) value = factorial(expr, value);
        else if (consume(expr, PCT)) value /= 100;
        else if (consume(expr, DMS)) {
            if (*expr->at >= '0' && *expr->at <= '9') {
                const double minutes = expr_atom(expr);
                value += minutes / 60;
                if (consume(expr, DMS) && *expr->at >= '0' && *expr->at <= '9') {
                    value += expr_atom(expr) / 3600;
                    consume(expr, DMS);
                }
            }
        } else break;
    }
    return value;
}

static double expr_sign(Expression *expr)
{
    if (expr->depth >= 24)
        return bad(expr, "MUITOS NIVEIS");
    ++expr->depth;
    double value;
    if (consume(expr, '-')) value = -expr_sign(expr);
    else if (consume(expr, '+')) value = expr_sign(expr);
    else {
        value = expr_postfix(expr);
        if (consume(expr, '^')) value = pow(value, expr_sign(expr));
        else if (consume(expr, ROOT)) {
            const double base = expr_sign(expr);
            bool odd = floor(value) == value && fmod(value, 2) != 0;
            value = base < 0 && odd ? -pow(-base, 1 / value) : pow(base, 1 / value);
        }
    }
    --expr->depth;
    return value;
}

static double expr_term(Expression *expr)
{
    double value = expr_sign(expr);
    while (!expr->error) {
        if (consume(expr, '*')) value *= expr_sign(expr);
        else if (consume(expr, '/')) {
            double divisor = expr_sign(expr);
            if (divisor == 0) return bad(expr, "DIVISAO POR ZERO");
            value /= divisor;
        }
        else if (consume(expr, FRAC)) {
            double divisor = expr_sign(expr);
            if (consume(expr, FRAC)) {
                double denominator = expr_sign(expr);
                if (denominator == 0) return bad(expr, "DIVISAO POR ZERO");
                value += copysign(fabs(divisor / denominator), value);
            } else if (divisor == 0)
                return bad(expr, "DIVISAO POR ZERO");
            else
                value /= divisor;
        }
        else if (consume(expr, NCR)) value = compare_int(expr, value, expr_sign(expr), false);
        else if (consume(expr, NPR)) value = compare_int(expr, value, expr_sign(expr), true);
        else {
            unsigned c = *expr->at;
            if (c == '(' || c == PI || c == EULER || c == ANS || (c >= SQRT && c <= RANDOM))
                value *= expr_sign(expr);
            else
                break;
        }
    }
    return value;
}

static double expr_sum(Expression *expr)
{
    double value = expr_term(expr);
    while (!expr->error) {
        if (consume(expr, '+')) value += expr_term(expr);
        else if (consume(expr, '-')) value -= expr_term(expr);
        else break;
    }
    return value;
}

static double evaluate(Expression *expr)
{
    double value = expr_sum(expr);
    if (*expr->at)
        return bad(expr, "REVISE A EXPRESSAO");
    if (!isfinite(value) || (expr->two_values && !isfinite(expr->secondary)))
        return bad(expr, "DOMINIO/FAIXA");
    return value;
}

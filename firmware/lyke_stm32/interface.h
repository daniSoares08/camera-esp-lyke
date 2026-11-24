#pragma once
#include "scientific.h"
#ifndef LYKE_INTERFACE_HOST_TEST
#include "battery.h"
#include "settings.h"
#endif
#include "battery_gauge.h"
#include "wifi_indicator.h"
#include "answer_frame.h"
static WifiIndicator wifi_indicator;
static AnswerFrame answer_frame;
static bool graphics_capable;

enum View { CALCULATOR, MENU, SETTINGS, SCAN_LIST, SAVED_LIST, PORTAL_VIEW, ANSWER_VIEW, BATTERY_VIEW, STAT_VIEW, SEND_CONFIRM, HELP, CONSTANTS, PROGRAM };
enum Menu { SEARCH, JOIN, CONFIG, PHOTOS, SEND, ANSWER, GALLERY, REMOVE, CLEAR, UPDATE, WIFI_OFF, BATTERY, CALC_SETUP, STATS, CONSTANT_MENU, PROGRAM_MENU, HELP_MENU, MENU_COUNT };
static const char *const menu_items[MENU_COUNT] = {
    "PROCURAR WIFI", "WIFI SALVO", "CONFIG. CELULAR", "TIRAR FOTO", "ENVIAR FOTOS IA",
    "ULTIMA RESPOSTA", "VER FOTOS NO WIFI", "REMOVER ULT. FOTO", "LIMPAR FOTOS", "ATUALIZAR FIRMWARE",
    "DESLIGAR WIFI", "BATERIA", "MODO CIENTIFICO", "ESTATISTICA", "CONSTANTES", "PROGRAMA", "AJUDA E TECLAS"
};
static const char *const angle_names[] = { "DEG", "RAD", "GRA" };
static const char *const format_names[] = { "NORM", "FIX", "SCI", "ENG" };
static const char *const constant_labels[] = { "PI", "EULER", "C  LUZ", "G  GRAVIDADE", "NA AVOGADRO", "KB BOLTZMANN" };
static const char *const constant_text[] = { "", "", "299792458", "9.80665", "6.02214076E23", "1.380649E-23" };
static const char *const help_lines[] = { "MODE ABRE MENU", "SHIFT+DIR TIRA FOTO", "TIRE 1 A 3 FOTOS",
    "ENVIE PELO MENU", "AC VOLTA OU LIMPA", "SETAS: REPLAY", "SHIFT+AC: APAGAR", "SHIFT+EXP: PI", "DEL APAGA FUNCAO" };
static enum View view;
static unsigned selected, settings_item, angle_mode, display_mode, precision = 6, help_page, constant_index, program_index;
static unsigned net_count, net_index, net_id[8];
static char networks[8][COLS + 1], answer_rows[ROWS][COLS + 1];
static unsigned answer_page, photo_count, voltage, link_state_at;
static bool answer_more, wifi, photo_busy, api_busy, answer_wait;
static bool scan_busy, wifi_saved_busy, portal_ready, last_answer;
static bool portal_pending;
static uint32_t started, note_at;
static char note[121];

enum { EXPR_CAP = 96, HISTORY_CAP = 8 };
static uint8_t entry[EXPR_CAP], history[HISTORY_CAP][EXPR_CAP];
static unsigned cursor, length, history_count, history_index;
static double ans, memory[12], second_value, stack[16];
static unsigned stack_size, stats_n;
static double sum_x, sum_y, sum_xx, sum_yy, sum_xy;
static bool solved, show_second, shift, alpha, hyp, rpn, editing_overwrite, blanked;
static unsigned recall, stat_item, solution_style;

static void notice(const char *text)
{
    print_text(note, sizeof(note), "%s", text);
    note_at = ticks;
}

static void request(const char *command)
{
    serial_send(command);
    started = ticks;
}

static void store_history(void)
{
    if (history_count) {
        if (strcmp((const char *)history[0], (const char *)entry) == 0)
            return;
        memmove(history[1], history[0], sizeof(history) - sizeof(history[0]));
    }
    memcpy(history[0], entry, length + 1);
    if (history_count < HISTORY_CAP)
        ++history_count;
    history_index = 0;
}

static void edit_clear(void)
{
    cursor = length = 0;
    entry[0] = 0;
    solved = false;
    note[0] = 0;
}

static void interface_key(enum Key key);

static void load_settings(void)
{
    read_preferences();
    if (!preferences.revision) return;
    angle_mode = (preferences.packed & 3u) % 3;
    display_mode = (preferences.packed >> 2) & 3u;
    precision = ((preferences.packed >> 4) & 15u) % 10;
    rpn = (preferences.packed >> 8) & 1u;
}

static void save_settings(const uint8_t *program)
{
    uint32_t packed = angle_mode | (display_mode << 2) | (precision << 4) | ((uint32_t)rpn << 8);
    if (!persist_preferences(packed, program)) notice("ERRO AO GRAVAR");
}

static void program_action(void)
{
    view = CALCULATOR;
    if (program_index == 0) {
        if (recording) {
            recording = false;
            notice("PROGRAMA SALVO");
            save_settings(recording_keys);
        } else {
            memset(recording_keys, 0, sizeof(recording_keys));
            recorded_keys = 0;
            recording = true;
            edit_clear();
        }
    } else if (program_index == 1) {
        recording = false;
        edit_clear();
        playing = true;
        if (!preferences.program[0]) notice("SEM PROGRAMA");
        for (unsigned i = 0; i < PROGRAM_CAP && view == CALCULATOR; ++i) {
            unsigned code = preferences.program[i];
            if (!code || code > KEY_COUNT || code == KEY_MODE+1 || code == KEY_ON+1 ||
                (code == KEY_AC+1 && shift)) break;
            interface_key((enum Key)(code-1));
            if (note[0] && !solved && code == KEY_EQ+1) break;
        }
        playing = false;
    } else {
        recording = false;
        memset(recording_keys, 0, sizeof(recording_keys));
        notice("PROGRAMA APAGADO");
        save_settings(recording_keys);
    }
}

static void edit_insert(unsigned value, bool continuation)
{
    if (solved) {
        edit_clear();
        if (continuation) {
            entry[0] = ANS;
            entry[1] = 0;
            cursor = length = 1;
        }
    }
    if (editing_overwrite && cursor < length) {
        entry[cursor++] = value;
    } else if (length < sizeof(entry) - 1) {
        memmove(entry + cursor + 1, entry + cursor, length - cursor + 1);
        entry[cursor++] = value;
        ++length;
    } else {
        notice("EXPRESSAO CHEIA");
    }
}

static double angle_scale(unsigned mode)
{
    return mode == 1 ? 1.0 : pi / (mode == 0 ? 180.0 : 200.0);
}

static bool evaluate_entry(void)
{
    if (!length || solved)
        return true;
    Expression expr = { .at = entry, .answer = ans, .angle = angle_scale(angle_mode), .variables = memory };
    double result = evaluate(&expr);
    if (expr.error) {
        notice(expr.error);
        return false;
    }
    store_history();
    ans = result;
    solved = true;
    solution_style = 0;
    show_second = expr.two_values;
    second_value = expr.secondary;
    if (show_second) {
        memory[VAR_X - VAR_A] = ans;
        memory[VAR_Y - VAR_A] = second_value;
    }
    return true;
}

static bool push_rpn(void)
{
    if (stack_size == sizeof(stack)/sizeof(stack[0])) {
        notice("PILHA CHEIA");
        return false;
    }
    if (!evaluate_entry())
        return false;
    stack[stack_size++] = ans;
    edit_clear();
    return true;
}

static void rpn_operation(unsigned symbol)
{
    if (length && !solved && !push_rpn())
        return;
    bool binary = (symbol < 128 && strchr("+-*/^", symbol)) ||
                  symbol == NPR || symbol == NCR || symbol == ROOT || symbol == FRAC;
    if (stack_size < (binary ? 2u : 1u)) {
        notice("PILHA INSUFICIENTE");
        return;
    }
    double b = stack[stack_size - 1], a = binary ? stack[stack_size - 2] : b;
    Expression expr = { .angle = angle_scale(angle_mode) };
    double result;
    switch (symbol) {
    case '+': result = a + b; break;
    case '-': result = a - b; break;
    case '*': result = a * b; break;
    case '/': case FRAC: result = a / b; break;
    case '^': result = pow(a, b); break;
    case ROOT: result = pow(b, 1 / a); break;
    case NPR: case NCR: result = compare_int(&expr, a, b, symbol == NPR); break;
    case SQUARE: result = a * a; break;
    case CUBE: result = a * a * a; break;
    case RECIP: result = 1 / a; break;
    case PCT: result = a / 100; break;
    case FACT: result = factorial(&expr, a); break;
    default: result = function(&expr, symbol, a); break;
    }
    if (expr.error || !isfinite(result)) {
        notice(expr.error ? expr.error : "DOMINIO/FAIXA");
        return;
    }
    if (binary) --stack_size;
    stack[stack_size - 1] = ans = result;
}

static char *decimal_digits(char *out, uint64_t value, unsigned minimum)
{
    char reversed[20];
    unsigned used = 0;
    do { reversed[used++] = '0' + value % 10; value /= 10; } while (value || used < minimum);
    while (used) *out++ = reversed[--used];
    *out = 0;
    return out;
}

static char *fixed_number(char *out, double value, unsigned after, bool trim)
{
    if (value < 0) { *out++ = '-'; value = -value; }
    uint64_t scale = 1;
    for (unsigned i = 0; i < after; ++i) scale *= 10;
    uint64_t rounded = (uint64_t)floor(value * scale + 0.5);
    out = decimal_digits(out, rounded / scale, 1);
    if (after) {
        *out++ = '.';
        char *fraction = out;
        out = decimal_digits(out, rounded % scale, after);
        if (trim) {
            while (out > fraction && out[-1] == '0') --out;
            if (out == fraction) --out;
        }
    }
    *out = 0;
    return out;
}

static void show_number(char *text, size_t size, double value)
{
    char formatted[42];
    const double magnitude = fabs(value);
    int digits = magnitude ? (int)floor(log10(magnitude)) : 0;
    if (display_mode == 1 && magnitude < 1e9) {
        fixed_number(formatted, value, precision, false);
    } else if (display_mode == 0 && digits >= -5 && digits < 10) {
        fixed_number(formatted, value, 9 - digits, true);
    } else {
        const bool engineering = display_mode == 3;
        int exponent = engineering ? digits - ((digits % 3 + 3) % 3) : digits;
        double mantissa = value;
        if (exponent < -300) mantissa = (value * 1e300) * pow(10.0, -exponent-300);
        else if (exponent < 0) mantissa *= pow(10.0, -exponent);
        else if (exponent > 0) mantissa /= pow(10.0, exponent);
        unsigned after = display_mode == 2 ? precision : engineering ? (unsigned)(9 - (digits - exponent)) : 9u;
        double carry = engineering ? 1000 : 10;
        if (fabs(mantissa) + 0.5 / pow(10.0, after) >= carry) {
            mantissa /= carry; exponent += engineering ? 3 : 1;
        }
        char *out = fixed_number(formatted, mantissa, after, display_mode != 2);
        *out++ = 'E';
        *out++ = exponent < 0 ? '-' : '+';
        decimal_digits(out, exponent < 0 ? -exponent : exponent, 1);
    }
    print_text(text, size, "%s", formatted);
}

static void display_result(char *text, size_t size)
{
    double value = ans;
    if (solution_style == 1 && fabs(value) < 1e8) {
        double target = fabs(value);
        for (unsigned den = 1; den <= 9999; ++den) {
            double n = round(target * den);
            if (fabs(target - n/den) <= 2e-11 * fmax(1,target)) {
                char numerator[22];
                char *at = numerator;
                if (value < 0) *at++ = '-';
                decimal_digits(at, (uint64_t)n, 1);
                print_text(text, size, "%s/%u", numerator, den);
                return;
            }
        }
    } else if (solution_style == 2 && fabs(value) < 1e8) {
        unsigned seconds = (unsigned)llround(fmod(fabs(value), 1.0) * 3600);
        unsigned degree = (unsigned)fabs(value) + seconds / 3600;
        print_text(text, size, "%s%u:%02u:%02u", value < 0 ? "-" : "", degree, seconds / 60 % 60, seconds % 60);
        return;
    }
    show_number(text, size, value);
}

#include "math_symbols.h"
#include "math_layout.h"

static void draw_expression_linear(void)
{
    char text[EXPR_CAP * 6], visible[COLS + 1];
    unsigned used = 0, position = 0;
    const uint8_t *input = solved && history_count ? history[0] : entry;
    for (unsigned i = 0; input[i] && used + 7 < sizeof(text); ++i) {
        if (i == cursor) position = used;
        if (input[i] < PI) text[used++] = (char)input[i];
        else {
            const char *token = token_text(input[i]);
            while (*token) text[used++] = *token++;
        }
    }
    if (solved || cursor == length) position = used;
    text[used] = 0;
    unsigned start = position >= COLS ? position - COLS + 1 : 0;
    print_text(visible, sizeof(visible), "%s", text + start);
    line(1, visible, false);
    if (!solved) {
        unsigned x = X_FIRST + (position - start) * 6;
        pixels[7 - PAGE_FIRST - 1][127 - x] |= 0x01;
    }
}

static void calc_pixel(int x, int y)
{
    if (x>=0 && x<126 && y>=8 && y<48)
        pixels[5-y/8][125-x] |= (uint8_t)(1u << (7-y%8));
}

static void calc_text_at(const char *text, unsigned top)
{
    for (unsigned i=0;i<COLS && text[i];++i) {
        const uint8_t *glyph=calc_glyph((uint8_t)text[i]);
        for (unsigned x=0;x<5;++x) for(unsigned y=0;y<7;++y)
            if (glyph[x]&(1u<<y)) calc_pixel(i*6+x,top+y);
    }
}

static void draw_calculation(char *text, unsigned capacity)
{
    bool result=solved || (rpn && stack_size), second=(show_second && solved) || (rpn && stack_size>1);
    bool status=photo_busy || api_busy || note[0] || recording;
    unsigned available=status?32:40, rh=0;
    uint8_t value[48];
    if (result) {
        display_result(text,capacity);
        unsigned i=0;
        for (;text[i] && i<sizeof(value)-1;++i) value[i]=solution_style==1 && text[i]=='/'?FRAC:(uint8_t)text[i];
        value[i]=0;
        rh=math_draw(value,i,false,8,available-(second?21:11),NULL);
        if (!rh) rh=7;
    }
    const uint8_t *input=solved && history_count?history[0]:entry;
    unsigned space=available-(result?rh+3:0)-(second?10:0);
    unsigned eh=input[0]?math_draw(input,solved?(unsigned)strlen((const char *)input):cursor,!solved,8,space,calc_pixel):0;
    if (!eh) {draw_expression_linear();eh=8;}
    if (result) {
        unsigned top=8+eh+3;
        if (!math_draw(value,strlen((const char *)value),false,top,rh,calc_pixel)) calc_text_at(text,top);
        if (second) {
            if (show_second && solved) {
                char number[32];show_number(number,sizeof(number),second_value);print_text(text,capacity,"Y=%s",number);
            } else show_number(text,capacity,stack[stack_size-2]);
            calc_text_at(text,top+rh+3);
        }
    }
}

static void dot(unsigned x, unsigned y)
{
    pixels[7-PAGE_FIRST][127-X_FIRST-x] |= (uint8_t)(1u << (7-y));
}

static void header(void)
{
    char text[COLS + 1];
    memset(text, ' ', COLS);
    text[COLS] = 0;
    if (shift) text[3] = 'S';
    if (alpha) text[5] = 'A';
    if (hyp) text[7] = 'H';
    line(0, text, false);
    unsigned signal=wifi_symbol(&wifi_indicator,ticks);
    if (signal==WIFI_SIGNAL) {
        dot(0,1); dot(4,1); dot(1,0); dot(2,0); dot(3,0);
        dot(1,3); dot(3,3); dot(2,2); dot(2,5);
    } else if(signal==WIFI_CROSS) {
        for(unsigned i=0;i<5;++i){dot(i,i);dot(4-i,i);}
    }
    static unsigned level = BATTERY_LEVEL_UNKNOWN;
    level = battery_level(&battery, level);
    // x=94..105, y=0..7: entirely inside the existing header, before photos.
    for (unsigned x=94; x<=104; ++x)
        for (unsigned y=0; y<=7; ++y)
            if (x==94 || x==104 || !y || y==7) dot(x,y);
    for (unsigned y=2; y<=5; ++y) dot(105,y);
    if (level == BATTERY_LEVEL_UNKNOWN) {
        dot(98,1); dot(99,1); dot(100,2); dot(99,3); dot(99,5);
    } else if (battery_low(&battery)) {
        if ((ticks/500)%2) {dot(99,1);dot(99,2);dot(99,3);dot(99,5);}
    } else {
        for (unsigned bar=0; bar<level; ++bar)
            for (unsigned y=2; y<=5; ++y) dot(96+2*bar,y);
    }
    for (unsigned slot = 0; slot < 3; ++slot) {
        bool fill = slot < photo_count;
        if (slot == photo_count && photo_busy && (ticks / 240) % 2) continue;
        for (unsigned x = 0; x < 4; ++x)
            for (unsigned y = 0; y < 4; ++y)
                if (fill || !x || !y || x == 3 || y == 3)
                    dot(COLS * 6 - 16 + 6 * slot + x, 1 + y);
    }
}

static void interface_draw(void)
{
    char text[48];
    for (unsigned row = 0; row < ROWS; ++row) line(row, "", false);
    if (blanked) return;
    if (view != ANSWER_VIEW) header();
    switch (view) {
    case CALCULATOR:
        draw_calculation(text,sizeof(text));
        if (photo_busy) line(5, "CAPTURANDO FOTO", false);
        else if (api_busy) line(5, "AGUARDANDO IA", false);
        else if (note[0]) line(5, note, false);
        else if (recording) line(5, "GRAVANDO PROGRAMA", false);
        break;
    case MENU: {
        unsigned start = selected - selected % 3;
        line(1, "MENU", true);
        for (unsigned i = 0; i < 3 && start+i < MENU_COUNT; ++i)
            line(2+i, menu_items[start+i], start+i == selected);
        break;
    }
    case SETTINGS:
        line(1, "MODO CIENTIFICO", true);
        print_text(text, sizeof(text), "ANGULO  %s", angle_names[angle_mode]); line(2, text, settings_item == 0);
        print_text(text, sizeof(text), "FORMATO %s %u", format_names[display_mode], precision); line(3, text, settings_item == 1);
        line(4, rpn ? "ENTRADA RPN" : "ENTRADA ALGEBRICA", settings_item == 2);
        break;
    case SCAN_LIST: case SAVED_LIST: {
        line(1, view == SCAN_LIST ? "REDES ENCONTRADAS" : "REDES SALVAS", true);
        unsigned start = net_index - net_index % 3;
        if (!net_count) line(2, scan_busy || wifi_saved_busy ? "AGUARDE..." : "NENHUMA REDE", false);
        for (unsigned i = 0; i < 3 && start+i < net_count; ++i)
            line(2+i, networks[start+i], start+i == net_index);
        break;
    }
    case PORTAL_VIEW:
        line(1, menu_items[selected], true);
        line(2, portal_ready ? "Wi-Fi Calc-Lyke" :
             portal_pending ? "ABRINDO REDE..." : "ESP SEM CONEXAO", false);
        line(3, portal_ready ? "192.168.8.1" : "", false);
        line(4, note[0] ? note : "AC PARA VOLTAR", false);
        line(5, portal_ready ? "" : "= TENTAR NOVAMENTE", false);
        break;
    case ANSWER_VIEW:
        if(graphics_capable && answer_frame.valid) {
            for(unsigned y=0;y<48;++y)for(unsigned x=0;x<126;++x)
                if(answer_frame.data[y][x/8]&(0x80u>>(x%8)))
                    pixels[5-y/8][125-x]|=(uint8_t)(1u<<(7-y%8));
        } else for (unsigned i = 0; i < sizeof(answer_rows)/sizeof(answer_rows[0]); ++i)
                line(i, answer_rows[i], false);
        break;
    case SEND_CONFIRM:
        print_text(text, sizeof(text), "ENVIAR %u FOTO(S)?", photo_count);
        line(1, text, true); line(2, "= ENVIA A IA", false);
        line(3, "AC CANCELA", false);
        break;
    case BATTERY_VIEW:
        line(1, "BATERIA PA0", true);
        if (battery.status == BATTERY_SATURATED) strcpy(text, "ADC SATURADO");
        else if (battery.status == BATTERY_RANGE) strcpy(text, "VERIFIQUE DIVISOR");
        else if (battery.status != BATTERY_OK || voltage < 300) strcpy(text, "SEM LEITURA");
        else print_text(text, sizeof(text), "%u.%02u V APROX.", voltage / 1000, voltage % 1000 / 10);
        line(2, text, false);
        print_text(text, sizeof(text), "PA0 %u mV", battery.pin_mv); line(3, text, false);
        print_text(text, sizeof(text), "ADC %u REF %u", battery.raw, battery.reference); line(4, text, false);
        line(5, "= MEDIR NOVAMENTE", false);
        break;
    case STAT_VIEW: {
        static const char *const labels[] = { "MEDIA X", "MEDIA Y", "DESVIO X AMOSTRA", "DESVIO X POP.", "SOMA X",
            "SOMA X^2", "REGRESSAO A", "REGRESSAO B", "CORRELACAO R", "SOMA XY" };
        double n = stats_n, sx = sum_xx - sum_x * sum_x / n, sy = sum_yy - sum_y * sum_y / n;
        double cov = sum_xy - sum_x * sum_y / n, slope = cov / sx;
        double values[] = { sum_x/n, sum_y/n, sqrt(fmax(0,sx)/(n-1)), sqrt(fmax(0,sx)/n), sum_x,
            sum_xx, sum_y/n - slope*sum_x/n, slope, cov/sqrt(sx*sy), sum_xy };
        line(1, "ESTATISTICA", true);
        print_text(text, sizeof(text), "N=%u  %s", stats_n, note); line(2, text, false);
        line(3, labels[stat_item], false);
        if (stats_n && isfinite(values[stat_item])) {
            show_number(text, sizeof(text), values[stat_item]); line(4, text, false);
        } else line(4, "SEM DADOS", false);
        break;
    }
    case HELP:
        line(1, "AJUDA", true);
        for (unsigned i = 0; i < 3; ++i) line(2+i, help_lines[help_page*3+i], false);
        break;
    case CONSTANTS: {
        unsigned first = constant_index - constant_index % 3;
        line(1, "CONSTANTES", true);
        for (unsigned i = 0; i < 3; ++i) line(2+i, constant_labels[first+i], first+i == constant_index);
        break;
    }
    case PROGRAM:
        line(1, "PROGRAMA", true);
        line(2, recording ? "SALVAR GRAVACAO" : "GRAVAR NOVO", program_index == 0);
        line(3, "EXECUTAR", program_index == 1);
        line(4, "APAGAR", program_index == 2);
        break;
    }
}

static void open_page(void)
{
    char command[40];
    memset(answer_rows, 0, sizeof(answer_rows));
    answer_wait = true;
    answer_more = false;
    if(graphics_capable) {
        answer_frame.sequence=(answer_frame.sequence+1)&65535u;
        answer_frame.receiving=answer_frame.valid=false;
        print_text(command,sizeof(command),"READG %u %u %u",answer_frame.x,answer_frame.y,answer_frame.sequence);
    } else print_text(command, sizeof(command), "READ6 %u", answer_page);
    request(command);
}

static void start_action(const char *command)
{
    if(!strcmp(command,"OFF"))wifi_mark(&wifi_indicator,WIFI_DISABLED,ticks);
    view = CALCULATOR;
    if (!photo_busy && !api_busy) {
        if (strcmp(command, "CAPTURE") == 0) photo_busy = true;
        if (strcmp(command, "SEND") == 0) api_busy = true;
        request(command);
    } else notice("ESP OCUPADO");
}

static void menu_action(void)
{
    note[0] = 0;
    switch (selected) {
    case SEARCH: net_count = net_index = 0; view = SCAN_LIST; scan_busy = true; request("SCAN"); break;
    case JOIN: net_count = net_index = 0; view = SAVED_LIST; wifi_saved_busy = true; request("LIST"); break;
    case CONFIG: case GALLERY: case UPDATE:
        view = PORTAL_VIEW; portal_ready = false; portal_pending = true; request("PORTAL"); break;
    case PHOTOS: start_action("CAPTURE"); break;
    case SEND: if (photo_count) view = SEND_CONFIRM; else { view = CALCULATOR; notice("TIRE UMA FOTO"); } break;
    case ANSWER: if (last_answer) { view = ANSWER_VIEW; answer_page = 0; answer_frame.x=answer_frame.y=0; open_page(); }
                 else { view = CALCULATOR; notice("SEM RESPOSTA SALVA"); } break;
    case REMOVE: start_action("REMOVE"); break;
    case CLEAR: start_action("CLEAR"); break;
    case WIFI_OFF: start_action("OFF"); break;
    case BATTERY: view = BATTERY_VIEW; voltage = battery_mv(); break;
    case CALC_SETUP: view = SETTINGS; break;
    case STATS: view = STAT_VIEW; break;
    case CONSTANT_MENU: view = CONSTANTS; constant_index = 0; break;
    case PROGRAM_MENU: view = PROGRAM; program_index = 0; break;
    case HELP_MENU: view = HELP; help_page = 0; break;
    }
}

static bool move_next(enum Key key)
{
    return key == KEY_DOWN || key == KEY_RIGHT;
}

static bool move_previous(enum Key key)
{
    return key == KEY_UP || key == KEY_LEFT;
}

static unsigned change(unsigned current, unsigned count, enum Key key)
{
    if (move_next(key)) return (current + 1) % count;
    if (move_previous(key)) return (current + count - 1) % count;
    return current;
}

static void navigation(enum Key key)
{
    if (key == KEY_ON || key == KEY_AC || key == KEY_MODE) {
        if (view == SETTINGS) save_settings(preferences.program);
        view = CALCULATOR;
        return;
    }
    switch (view) {
    case MENU:
        selected = change(selected, MENU_COUNT, key);
        if (key == KEY_EQ) menu_action();
        break;
    case SETTINGS:
        if (key == KEY_UP || key == KEY_DOWN) settings_item = change(settings_item, 3, key);
        else if (key == KEY_EQ || key == KEY_RIGHT || key == KEY_LEFT) {
            if (settings_item == 0) angle_mode = change(angle_mode, 3, key == KEY_EQ ? KEY_RIGHT : key);
            else if (settings_item == 1) display_mode = change(display_mode, 4, key == KEY_EQ ? KEY_RIGHT : key);
            else rpn = !rpn;
        } else if (settings_item == 1 && key == KEY_D0) precision = 0;
        else if (settings_item == 1) {
            switch (key) {
            case KEY_D1: precision=1; break; case KEY_D2: precision=2; break; case KEY_D3: precision=3; break;
            case KEY_D4: precision=4; break; case KEY_D5: precision=5; break; case KEY_D6: precision=6; break;
            case KEY_D7: precision=7; break; case KEY_D8: precision=8; break; case KEY_D9: precision=9; break;
            default: break;
            }
        }
        break;
    case SCAN_LIST: case SAVED_LIST:
        if (net_count) net_index = change(net_index, net_count, key);
        if (net_count && key == KEY_EQ) {
            if (view == SCAN_LIST) {
                view = PORTAL_VIEW; selected = CONFIG; portal_ready = false; portal_pending = true; request("PORTAL");
            } else {
                char command[] = "JOIN 0";
                command[5] += net_id[net_index];
                request(command); view = CALCULATOR;
            }
        }
        break;
    case ANSWER_VIEW:
        if(graphics_capable) {
            if(answer_wait||!answer_frame.valid)break;
            unsigned x=answer_frame.x,y=answer_frame.y;
            if(key==KEY_DOWN)y+=16;
            if(key==KEY_UP)y=y>16?y-16:0;
            if(key==KEY_RIGHT){if(answer_frame.width>126)x+=24;else y+=48;}
            if(key==KEY_LEFT){if(answer_frame.width>126)x=x>24?x-24:0;else y=y>48?y-48:0;}
            if(x>answer_frame.width-126)x=answer_frame.width-126;
            if(y>answer_frame.height-48)y=answer_frame.height-48;
            if(x!=answer_frame.x||y!=answer_frame.y){answer_frame.x=x;answer_frame.y=y;open_page();}
            break;
        }
        if (!answer_wait && move_next(key) && answer_more) { ++answer_page; open_page(); }
        if (!answer_wait && move_previous(key) && answer_page) { --answer_page; open_page(); }
        break;
    case PORTAL_VIEW:
        if (key == KEY_EQ) {
            note[0] = 0;
            portal_pending = true;
            request("PORTAL");
        }
        break;
    case BATTERY_VIEW: if (key == KEY_EQ) voltage = battery_mv(); break;
    case SEND_CONFIRM: if (key == KEY_EQ) start_action("SEND"); break;
    case STAT_VIEW:
        stat_item = change(stat_item, 10, key);
        if (key == KEY_EQ) {
            double values[] = { sum_x/stats_n, sum_y/stats_n, sqrt(fmax(0,sum_xx-sum_x*sum_x/stats_n)/(stats_n-1.0)),
                sqrt(fmax(0,sum_xx-sum_x*sum_x/stats_n)/stats_n), sum_x, sum_xx };
            if (stats_n && stat_item < 6 && isfinite(values[stat_item])) { ans = values[stat_item]; solved = true; view = CALCULATOR; }
        }
        break;
    case HELP: help_page = change(help_page, 3, key); break;
    case CONSTANTS:
        constant_index = change(constant_index, 6, key);
        if (key == KEY_EQ) {
            const char *value = constant_text[constant_index];
            if (constant_index < 2) edit_insert(constant_index ? EULER : PI, false);
            else while (*value) edit_insert((uint8_t)*value++, false);
            view = CALCULATOR;
        }
        break;
    case PROGRAM:
        program_index = change(program_index, 3, key);
        if (key == KEY_EQ) program_action();
        break;
    default: break;
    }
}

static void history_move(enum Key key)
{
    if (!history_count) return;
    if (solved) history_index = 0;
    else if (key == KEY_UP && history_index + 1 < history_count) ++history_index;
    else if (key == KEY_DOWN && history_index) --history_index;
    strcpy((char *)entry, (const char *)history[history_index]);
    cursor = length = strlen((const char *)entry);
    solved = false;
}

static unsigned alpha_symbol(enum Key key)
{
    switch (key) {
    case KEY_NEG: return VAR_A; case KEY_DMS: return VAR_B; case KEY_HYP: return VAR_C;
    case KEY_SIN: return VAR_D; case KEY_COS: return VAR_E; case KEY_TAN: return VAR_F;
    case KEY_RPAREN: return VAR_X; case KEY_COMMA: return VAR_Y; case KEY_MPLUS: return VAR_M;
    case KEY_LN: return EULER;
    case KEY_POWER: return GREEK_DELTA; case KEY_LOG: return GREEK_THETA;
    case KEY_EXP: return GREEK_OMEGA; default: return 0;
    }
}

static void stats_add(bool remove)
{
    if (!length || (remove && !stats_n)) return;
    const uint8_t *comma = (const uint8_t *)strchr((const char *)entry, ',');
    if (comma) entry[comma-entry] = 0;
    Expression x = { .at=entry, .angle=angle_scale(angle_mode), .variables=memory, .answer=ans };
    Expression y = x;
    if (comma) { entry[comma-entry] = ','; y.at=comma+1; }
    if (comma) entry[comma-entry] = 0;
    double a = evaluate(&x), b = comma ? evaluate(&y) : 0;
    if (comma) entry[comma-entry] = ',';
    if (x.error || y.error) { notice(x.error ? x.error : y.error); return; }
    double sign = remove ? -1 : 1;
    stats_n = remove ? stats_n - 1 : stats_n + 1;
    sum_x += sign*a; sum_y += sign*b; sum_xx += sign*a*a; sum_yy += sign*b*b; sum_xy += sign*a*b;
    edit_clear();
    view = STAT_VIEW; notice(remove ? "REMOVIDO" : "REGISTRADO");
}

static void interface_key(enum Key key)
{
    if (blanked) {
        if (key == KEY_ON || (!has_on() && key == KEY_MODE)) blanked = false;
        return;
    }
    if (view != CALCULATOR) { navigation(key); return; }
    if (recording && key != KEY_MODE && key != KEY_ON &&
        !(shift && (key == KEY_AC || key == KEY_RIGHT))) {
        if (recorded_keys < PROGRAM_CAP) recording_keys[recorded_keys++] = key + 1;
        else {
            recording = false;
            notice("PROGRAMA SALVO");
            save_settings(recording_keys);
            return;
        }
    }
    if (key == KEY_SHIFT) { shift = !shift; alpha = false; return; }
    if (key == KEY_ALPHA) { alpha = !alpha; shift = false; return; }
    bool shifted = shift, alpha_mode = alpha;
    shift = alpha = false;
    note[0] = 0;
    if (key == KEY_ON) {
        if (shifted && !playing) start_action("CAPTURE");
        else { edit_clear(); hyp = false; recall = 0; }
        return;
    }
    if (key == KEY_MODE) {
        if (shifted) {
            memset(memory, 0, sizeof(memory));
            stack_size = stats_n = history_count = 0;
            sum_x = sum_y = sum_xx = sum_yy = sum_xy = ans = 0;
            edit_clear(); notice("MEMORIAS LIMPAS");
        } else view = MENU;
        return;
    }
    if (key == KEY_RCL && !alpha_mode) { recall = shifted ? 2 : 1; notice(shifted ? "STO: VARIAVEL" : "RCL: VARIAVEL"); return; }
    if (recall || alpha_mode) {
        unsigned sym = alpha_symbol(key), operation = recall;
        recall = 0;
        if (variable_index(sym)>=0 && operation == 2) {
            if (evaluate_entry()) memory[variable_index(sym)] = ans;
        } else if (sym) edit_insert(sym, false);
        return;
    }
    if (key == KEY_EQ && !shifted) {
        if (rpn) push_rpn(); else evaluate_entry();
        return;
    }
    if (key == KEY_AC) {
        edit_clear(); hyp = false;
        if (shifted) { blanked = true; wifi_mark(&wifi_indicator,WIFI_DISABLED,ticks);request("OFF"); }
        return;
    }
    if (key == KEY_DEL) {
        if (shifted) editing_overwrite = !editing_overwrite;
        else if (solved) { history_move(KEY_UP); }
        else if (cursor) {
            memmove(entry + cursor - 1, entry + cursor, length - cursor + 1);
            --length; --cursor;
        } else if (rpn && !length && stack_size) {
            --stack_size; ans = stack_size ? stack[stack_size-1] : 0;
        }
        return;
    }
    if (key == KEY_LEFT || key == KEY_RIGHT) {
        if (key == KEY_RIGHT && shifted && !playing) {
            start_action("CAPTURE");
            return;
        }
        if (solved) history_move(KEY_UP);
        if (key == KEY_LEFT && cursor) --cursor;
        if (key == KEY_RIGHT && cursor < length) ++cursor;
        return;
    }
    if (key == KEY_UP || key == KEY_DOWN) { history_move(key); return; }
    if (key == KEY_HYP) { hyp = !hyp; return; }
    if (key == KEY_ENG) { display_mode = display_mode == 3 ? 0 : 3; return; }
    if (key == KEY_FRAC && (solved || shifted)) { solution_style = solution_style == 1 ? 0 : 1; return; }
    if (key == KEY_DMS && (solved || shifted)) { solution_style = solution_style == 2 ? 0 : 2; return; }
    if (key == KEY_MPLUS && evaluate_entry()) { memory[VAR_M-VAR_A] += shifted ? -ans : ans; notice("MEMORIA M ATUALIZ."); return; }
    if (key == KEY_ANS && shifted) { angle_mode = (angle_mode + 1) % 3; notice(angle_names[angle_mode]); return; }
    if ((key == KEY_D1 || key == KEY_D2) && shifted) { stat_item = key == KEY_D1 ? 4 : 0; view = STAT_VIEW; return; }
    if (key == KEY_COMMA && shifted) { stats_add(false); return; }
    unsigned symbol = 0;
    bool continuation = false;
    switch (key) {
    case KEY_D0: symbol = shifted ? RND : '0'; break; case KEY_D1: symbol='1'; break;
    case KEY_D2: symbol='2'; break; case KEY_D3: symbol='3'; break; case KEY_D4: symbol='4'; break;
    case KEY_D5: symbol='5'; break; case KEY_D6: symbol='6'; break; case KEY_D7: symbol='7'; break;
    case KEY_D8: symbol='8'; break; case KEY_D9: symbol='9'; break; case KEY_DOT: symbol=shifted?RANDOM:'.'; break;
    case KEY_EXP: symbol=shifted?PI:'E'; break; case KEY_ANS: symbol=ANS; break;
    case KEY_ADD: symbol='+'; continuation=true; break; case KEY_SUB: symbol='-'; continuation=true; break;
    case KEY_MUL: symbol='*'; continuation=true; break; case KEY_DIV: symbol='/'; continuation=true; break;
    case KEY_POWER: symbol=shifted?ROOT:'^'; continuation=true; break;
    case KEY_INV: symbol=shifted?FACT:RECIP; continuation=true; break;
    case KEY_NCR: symbol=shifted?NPR:NCR; continuation=true; break;
    case KEY_CUBE: symbol=shifted?CBRT:CUBE; continuation=!shifted; break;
    case KEY_FRAC: symbol=FRAC; continuation=true; break; case KEY_SQRT: symbol=SQRT; break;
    case KEY_SQUARE: symbol=SQUARE; continuation=true; break; case KEY_EQ: symbol=PCT; continuation=true; break;
    case KEY_DMS: symbol=DMS; break; case KEY_NEG: symbol='-'; break;
    case KEY_LOG: symbol=shifted?POW10:LOG10; break; case KEY_LN: symbol=shifted?EXP:LN; break;
    case KEY_SIN: symbol=hyp?(shifted?ASINH:SINH):(shifted?ASIN:SIN); hyp=false; break;
    case KEY_COS: symbol=hyp?(shifted?ACOSH:COSH):(shifted?ACOS:COS); hyp=false; break;
    case KEY_TAN: symbol=hyp?(shifted?ATANH:TANH):(shifted?ATAN:TAN); hyp=false; break;
    case KEY_POL: symbol=shifted?REC:POL; break;
    case KEY_LPAREN: symbol='('; break; case KEY_RPAREN: symbol=')'; break; case KEY_COMMA: symbol=','; break;
    default: break;
    }
    if (!symbol) return;
    if (rpn && ((continuation && symbol != DMS) || (symbol >= SQRT && symbol <= ATANH))) {
        rpn_operation(symbol);
        return;
    }
    if (symbol == 'E' && (!cursor || solved)) edit_insert('1', false);
    edit_insert(symbol, continuation);
}

static void interface_mapped(void)
{
    notice(has_on() ? "50 TECLAS MAPEADAS" : "49 TECLAS MAPEADAS");
    request("STATUS");
}

static bool interface_message(const char *text)
{
    link_state_at = ticks;
    if(!strcmp(text,"CAPS G1")){graphics_capable=true;return false;}
    if(!strncmp(text,"FRAME ",6)||!strncmp(text,"PIX ",4)||!strncmp(text,"FRAME_END ",10)) {
        int result=frame_message(&answer_frame,text);
        if(result){answer_wait=false;if(result<0){answer_frame.receiving=answer_frame.valid=false;strcpy(answer_rows[0],"FALHA NO DESENHO");}return true;}
        return false;
    }
    if(!strncmp(text,"LINK ",5)) {
        unsigned phase=!strcmp(text+5,"OK")?WIFI_OK:!strcmp(text+5,"FAIL")?WIFI_FAIL:
            !strcmp(text+5,"JOIN")?WIFI_JOIN:!strcmp(text+5,"OFF")?WIFI_DISABLED:WIFI_IDLE;
        wifi_mark(&wifi_indicator,phase,ticks);return true;
    }
    if (strncmp(text, "STATE ", 6) == 0 && strlen(text) == 13) {
        bool connected=text[6]=='1';
        if(connected&&!wifi)wifi_mark(&wifi_indicator,WIFI_OK,ticks);
        else if(!connected&&wifi&&wifi_indicator.phase!=WIFI_DISABLED)wifi_mark(&wifi_indicator,WIFI_FAIL,ticks);
        wifi = text[6] == '1';
        photo_count = text[8] >= '0' && text[8] <= '3' ? (unsigned)(text[8]-'0') : 0;
        portal_ready = text[10] == '1';
        if (portal_ready) portal_pending = false;
        last_answer = text[12] == '1';
        photo_busy = false;
    } else if (strncmp(text, "NOTE ", 5) == 0) {
        if(!strcmp(text+5,"CONECTANDO")){wifi_mark(&wifi_indicator,WIFI_JOIN,ticks);return true;}
        if(!strcmp(text+5,"WIFI CONECTADO")){wifi_mark(&wifi_indicator,WIFI_OK,ticks);return true;}
        if(!strcmp(text+5,"FALHA NO WIFI")){wifi_mark(&wifi_indicator,WIFI_FAIL,ticks);return true;}
        portal_pending = false;
        photo_busy = api_busy = false;
        notice(text+5);
    } else if (strcmp(text, "WORK FOTO") == 0) {
        photo_busy = true;
    } else if (strcmp(text, "WORK IA") == 0) {
        api_busy = true;
    } else if (strcmp(text, "ANSWER OK") == 0) {
        api_busy = false;
        view = ANSWER_VIEW; answer_page=0; answer_frame.x=answer_frame.y=0;open_page();
    } else if (strcmp(text, "PORTAL OK") == 0) {
        portal_ready = true;
        portal_pending = false;
    } else if (strncmp(text,"SCAN ",5)==0) {
        scan_busy = false;
    } else if (strcmp(text,"LIST_END")==0) {
        wifi_saved_busy = false;
    } else if (((view == SCAN_LIST && strncmp(text, "NET ", 4) == 0) ||
                (view == SAVED_LIST && strncmp(text, "SAVED ", 6) == 0)) && net_count < 8) {
        unsigned prefix = view == SCAN_LIST ? 4 : 6;
        if (text[prefix] < '0' || text[prefix] > '7' || text[prefix+1] != ' ') return false;
        net_id[net_count] = text[prefix]-'0';
        print_text(networks[net_count], sizeof(networks[net_count]), "%s", text+prefix+2);
        ++net_count;
    } else if (strncmp(text, "TEXT ", 5) == 0 && text[5] >= '0' && text[5] < '0' + ROWS && text[6]==' ') {
        print_text(answer_rows[text[5]-'0'], sizeof(answer_rows[0]), "%s", text+7);
    } else if (strcmp(text,"TEXT_END")==0 || strcmp(text,"TEXT_MORE")==0) {
        answer_wait = false;
        answer_more = text[5] == 'M';
        bool empty = true;
        for (unsigned i = 0; i < ROWS; ++i) if (answer_rows[i][0]) empty = false;
        if (empty && !answer_more)
            strcpy(answer_rows[0], "FIM DA RESPOSTA");
    } else return false;
    return true;
}

static bool interface_tick(void)
{
    static uint32_t last_ping, last_blink, last_battery_blink;
    if (mapped < KEY_COUNT) return false;
    uint32_t now = ticks;
    if(wifi_animation(&wifi_indicator,now)&&!blanked&&view!=ANSWER_VIEW)return true;
    if (!blanked && view != ANSWER_VIEW && battery_low(&battery) && now-last_battery_blink>=500) {
        last_battery_blink=now;
        return true;
    }
    if (view == PORTAL_VIEW && portal_pending && now - started > 5000) {
        portal_pending = false;
        portal_ready = false;
        return true;
    }
    if (!photo_busy && !api_busy && now - last_ping > 4500) {
        serial_send("STATUS");
        last_ping = now;
    }
    if (!api_busy && wifi && now - link_state_at > 14000) { wifi = false; wifi_mark(&wifi_indicator,WIFI_FAIL,now);return true; }
    if (note[0] && now - note_at > 4200) { note[0]=0; return true; }
    if ((scan_busy || wifi_saved_busy || answer_wait || photo_busy) && now - started > 22000) {
        scan_busy = wifi_saved_busy = answer_wait = photo_busy = false;
        notice("ESP SEM RESPOSTA"); return true;
    }
    if (api_busy && now - started > 110000) { api_busy=false; notice("TEMPO DA IA ESGOTOU"); return true; }
    if (photo_busy && now - last_blink >= 240) { last_blink = now; return true; }
    return false;
}

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "../keys.h"
#include "../battery_math.h"
enum {ROWS=6,COLS=21,X_FIRST=2,PAGE_FIRST=2,PROGRAM_CAP=96};
static uint8_t pixels[8][128], recording_keys[96];
static uint32_t ticks;
static unsigned mapped=KEY_COUNT,recorded_keys;
static bool recording,playing;
static BatteryReading battery;
static struct {uint32_t revision,packed;uint8_t program[96];} preferences;
static char sent[64];
static bool has_on(void) {return false;}
static void read_preferences(void) {}
static bool persist_preferences(uint32_t v,const uint8_t *p) {preferences.packed=v;memcpy(preferences.program,p,96);return true;}
static unsigned battery_mv(void) {return 4000;}
static void serial_send(const char *s) {snprintf(sent,sizeof(sent),"%s",s);}
static void print_text(char *p,size_t n,const char *format,...) {va_list args;va_start(args,format);vsnprintf(p,n,format,args);va_end(args);}
static uint32_t __RBIT(uint32_t x) {uint32_t y=0;for(int i=0;i<32;++i) {y=(y<<1)|(x&1);x>>=1;}return y;}
#include "../text_display.h"
#define LYKE_INTERFACE_HOST_TEST
#include "../interface.h"
static void press(const enum Key *keys,size_t n) {for(size_t i=0;i<n;++i)interface_key(keys[i]);}
#define PRESS(...) do{enum Key k[]={__VA_ARGS__};press(k,sizeof(k)/sizeof(*k));}while(0)
static void save(const char *folder,const char *name)
{
    if(!folder)return;char path[512];snprintf(path,sizeof(path),"%s/%s.pbm",folder,name);FILE *f=fopen(path,"wb");assert(f);
    fprintf(f,"P1\n128 48\n");for(int y=0;y<48;++y){for(int x=0;x<128;++x)fprintf(f,"%u ",(pixels[5-y/8][127-x]>>(7-y%8))&1);fputc('\n',f);}fclose(f);
}
int main(int argc,char **argv)
{
    const char *folder=argc>1?argv[1]:NULL;
    ticks=10;assert(interface_message("STATE 1 0 1 0"));
    assert(wifi_symbol(&wifi_indicator,ticks)==WIFI_SIGNAL);
    ticks=1810;assert(interface_message("LINK OK"));
    assert(wifi_symbol(&wifi_indicator,ticks)==WIFI_HIDDEN);
    assert(interface_message("LINK FAIL"));assert(wifi_symbol(&wifi_indicator,ticks)==WIFI_SIGNAL);
    ticks+=500;assert(wifi_symbol(&wifi_indicator,ticks)==WIFI_CROSS);
    assert(interface_message("LINK OFF"));assert(wifi_symbol(&wifi_indicator,ticks)==WIFI_HIDDEN);
    // Icon stays inside the current header; photo boxes and hidden rows are intact.
    battery=(BatteryReading){.status=BATTERY_OK,.mv=3840};
    interface_draw();save(folder,"bateria_media");
    uint8_t photos[128];memcpy(photos,pixels[5],128);
    battery.mv=4200;interface_draw();save(folder,"bateria_cheia");
    for(unsigned x=110;x<126;++x)assert(pixels[5][125-x]==photos[125-x]);
    for(unsigned x=0;x<128;++x)assert(!pixels[6][x]&&!pixels[7][x]);
    battery.mv=3400;ticks=500;interface_draw();save(folder,"bateria_baixa");
    uint8_t low[128];memcpy(low,pixels[5],128);ticks=1000;interface_draw();
    assert(memcmp(low,pixels[5],128));
    battery.status=BATTERY_SATURATED;interface_draw();save(folder,"bateria_sem_leitura");
    battery=(BatteryReading){.status=BATTERY_OK,.mv=3840};
    PRESS(KEY_D1,KEY_FRAC,KEY_D2,KEY_ADD,KEY_D3,KEY_FRAC,KEY_D4,KEY_EQ);
    assert(solved && fabs(ans-1.25)<1e-12);interface_draw();save(folder,"tela_fracoes_decimal");
    PRESS(KEY_FRAC);assert(solution_style==1);interface_draw();save(folder,"tela_fracoes_resultado");
    PRESS(KEY_AC,KEY_SQRT,KEY_D9,KEY_ADD,KEY_D1,KEY_D6,KEY_RPAREN,KEY_EQ);
    assert(solved&&ans==5);interface_draw();save(folder,"tela_raiz");
    PRESS(KEY_AC,KEY_D7,KEY_SHIFT,KEY_RCL,KEY_POWER);assert(memory[9]==7);
    PRESS(KEY_AC,KEY_ALPHA,KEY_POWER,KEY_MUL,KEY_D2,KEY_EQ);assert(ans==14);interface_draw();save(folder,"tela_delta");
    PRESS(KEY_AC,KEY_D2,KEY_POWER,KEY_D1,KEY_D0,KEY_EQ);assert(ans==1024);interface_draw();save(folder,"tela_potencia");
    // Empty-frame response test: six text rows, with no status icons or title.
    wifi=true;photo_count=3;view=ANSWER_VIEW;battery.mv=3400;
    for(unsigned i=0;i<6;++i)snprintf(answer_rows[i],22,"LINHA %u",i+1);
    interface_draw();uint8_t actual[8][128];memcpy(actual,pixels,sizeof(pixels));
    memset(pixels,0,sizeof(pixels));for(unsigned i=0;i<6;++i)line(i,answer_rows[i],false);
    assert(!memcmp(actual,pixels,sizeof(pixels)));save(folder,"tela_resposta_6_linhas");
    view=CALCULATOR;photo_busy=true;interface_draw();
    // Status is in the final row and all graphics remain above it.
    uint8_t status[128];memcpy(status,pixels[0],128);line(5,"CAPTURANDO FOTO",false);assert(!memcmp(status,pixels[0],128));
    view=ANSWER_VIEW;open_page();assert(!strcmp(sent,"READ6 0"));
    assert(interface_message("TEXT 5 SEXTA LINHA"));assert(!strcmp(answer_rows[5],"SEXTA LINHA"));
    assert(!interface_message("CAPS G1"));answer_frame.sequence=42;
    uint8_t bitmap[768]={0};for(unsigned y=0;y<48;++y)bitmap[y*16+y/8]|=0x80u>>(y%8);
    char message[100];snprintf(message,sizeof(message),"FRAME 42 0 0 192 200 %u",frame_checksum(bitmap,768));
    assert(!interface_message(message));
    for(unsigned y=0;y<48;++y){unsigned at=snprintf(message,sizeof(message),"PIX 42 %u ",y);
        for(unsigned x=0;x<16;++x)snprintf(message+at+2*x,3,"%02x",bitmap[y*16+x]);assert(!interface_message(message));}
    assert(interface_message("FRAME_END 42"));interface_draw();save(folder,"tela_frame_bitmap");
    for(unsigned y=0;y<48;++y)for(unsigned x=0;x<126;++x)
        assert(((pixels[5-y/8][125-x]>>(7-y%8))&1)==(x==y));
    PRESS(KEY_RIGHT);assert(!strcmp(sent,"READG 24 0 43"));
    answer_frame.valid=true;answer_wait=false;PRESS(KEY_DOWN);assert(!strcmp(sent,"READG 24 16 44"));
    puts("Actual interface: keyboard, Greek memory, 2D result, six-row response and status boundaries OK");
    return 0;
}

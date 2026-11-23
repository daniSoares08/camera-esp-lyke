#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
using std::min;
class String:public std::string {
public:using std::string::string;using std::string::operator=;
    String(const std::string& s):std::string(s){}
};
static uint32_t clock_ms;
static uint32_t millis(){return clock_ms++;}
static void delay(unsigned n){clock_ms+=n;}
static constexpr int MALLOC_CAP_SPIRAM=1,MALLOC_CAP_8BIT=2;
static void *heap_caps_malloc(size_t n,int){return malloc(n);}
static struct {size_t available=4*1024*1024;size_t getFreePsram(){return available;}} ESP;
static uint32_t fake_crc(const uint8_t*,size_t);
static struct {
    std::string input;size_t at=0,total=0;uint32_t crc=0;unsigned boots=0,chunks=0,drop_ack=0;bool silent=false,legacy=false;
    std::vector<uint8_t> received;
    int available(){return at<input.size();}
    int read(){return (unsigned char)input[at++];}
    void print(const char*){++boots;if(!silent)input+=legacy?"LYKE BOOT\n":"LYKE BOOT V2\n";else input+="MAP 49 1\n";}
    void printf(const char *fmt,...){char text[80];va_list args;va_start(args,fmt);vsnprintf(text,80,fmt,args);va_end(args);
        unsigned long n,c;assert(sscanf(text,"FLASH %lx %lx",&n,&c)==2);total=n;crc=c;input+="LYKE READY_DATA\n";}
    size_t write(const uint8_t *p,size_t n){received.insert(received.end(),p,p+n);++chunks;
        if(chunks!=drop_ack)input+="LYKE ACK\n";
        if(received.size()==total){assert(fake_crc(received.data(),received.size())==crc);input+="LYKE INSTALLED\nLYKE APP V1\n";}return n;}
} stmSerial;
#include "../../esp32cam_integrada/lyke/stm_update.h"
static uint32_t fake_crc(const uint8_t *p,size_t n){return ~image_crc_update(UINT32_MAX,p,n);}
static void clear_serial(){stmSerial={};clock_ms=0;}
static void upload(const std::vector<uint8_t>& p){stmUpdate.start(true,"lyke-stm32-ota.bin");for(size_t i=0;i<p.size();i+=137)stmUpdate.write(p.data()+i,min(size_t(137),p.size()-i));}
int main(int argc,char **argv)
{
    assert(argc==2);std::ifstream f(argv[1],std::ios::binary);std::vector<uint8_t> p((std::istreambuf_iterator<char>(f)),{});assert(!p.empty());
    clear_serial();upload(p);assert(stmUpdate.finish());assert(stmSerial.received==p && stmSerial.boots==1);
    clear_serial();auto corrupt=p;corrupt.back()^=1;upload(corrupt);assert(!stmUpdate.finish() && !stmSerial.boots);
    clear_serial();corrupt.resize(26);upload(corrupt);assert(!stmUpdate.finish() && !stmSerial.boots);
    clear_serial();stmSerial.drop_ack=2;upload(p);assert(!stmUpdate.finish());assert(stmUpdate.error.find("LYKE ACK")!=std::string::npos);
    clear_serial();stmSerial.silent=true;upload(p);assert(!stmUpdate.finish());assert(stmUpdate.error.find("MAP 49 1")!=std::string::npos);
    clear_serial();stmSerial.legacy=true;upload(p);assert(!stmUpdate.finish() && !stmSerial.total && stmSerial.received.empty());
    assert(stmUpdate.error.find("base R10")!=std::string::npos);
    clear_serial();stmUpdate.start(false,"lyke-stm32-ota.bin");stmUpdate.write(p.data(),p.size());assert(!stmUpdate.finish() && !stmSerial.boots);
    clear_serial();ESP.available=100;stmUpdate.start(true,"lyke-stm32-ota.bin");assert(stmUpdate.error=="PSRAM insuficiente");ESP.available=4*1024*1024;
    puts("Actual ESP updater: valid transfer, early CRC rejection, truncation, lost ACK, legacy app, auth and RAM checks OK");
    return 0;
}

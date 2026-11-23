#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include <fstream>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif
#define FLASH_BASE 0x08000000u
#define LYKE_APP 0x08002000u
#define LYKE_STAGE 0x08012000u
#define LYKE_PAGE 2048u
static int budget=-1,operations;
struct PowerCut {};
static void flash_begin() {}
static void flash_end() {}
static void step() {if(budget==0)throw PowerCut();if(budget>0)--budget;++operations;}
static bool flash_erase(uint32_t address)
{
    assert(address>=LYKE_APP && address+LYKE_PAGE<=LYKE_STAGE);step();
    memset((void*)(uintptr_t)address,0xff,LYKE_PAGE);return true;
}
static bool flash_write(uint32_t address,uint32_t a,uint32_t b)
{
    assert(address>=LYKE_APP && address+8<=LYKE_STAGE && !(address&7));step();
    auto p=(uint32_t*)(uintptr_t)address;assert(p[0]==UINT32_MAX && p[1]==UINT32_MAX);p[0]=a;p[1]=b;return true;
}
#include "../boot_store.h"
int main(int argc,char **argv)
{
    assert(argc==3);
#ifdef _WIN32
    auto memory=VirtualAlloc((void*)FLASH_BASE,128*1024,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
#else
    auto memory=mmap((void*)FLASH_BASE,128*1024,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);
#endif
    assert(memory==(void*)FLASH_BASE);
    std::ifstream f(argv[1],std::ios::binary),a(argv[2],std::ios::binary);
    std::vector<uint8_t> package((std::istreambuf_iterator<char>(f)),{}),app((std::istreambuf_iterator<char>(a)),{});assert(!app.empty());
    memset(memory,0xa5,128*1024);memcpy((void*)LYKE_STAGE,package.data(),package.size());
    std::vector<uint8_t> baseline((uint8_t*)memory,(uint8_t*)memory+128*1024);
    Image image={(uint32_t)package.size(),crc32(LYKE_STAGE,package.size()),META_MAGIC,~META_MAGIC};
    budget=-1;operations=0;assert(install(&image));int count=operations;assert(!memcmp((void*)LYKE_APP,app.data(),app.size()));
    int tested=0;
    for(int cut=0;cut<count;++cut) {
        // Exhaustive for small fixtures; page/copy boundaries plus samples for full release.
        if(app.size()>8192 && cut>40 && cut<count-40 && cut%61)continue;
        memcpy(memory,baseline.data(),baseline.size());budget=cut;operations=0;
        try {install(&image);assert(false);}catch(PowerCut&){}
        budget=-1;assert(install(&image));assert(!memcmp((void*)LYKE_APP,app.data(),app.size()));
        assert(!memcmp(memory,baseline.data(),LYKE_APP-FLASH_BASE));
        assert(!memcmp((void*)0x0801e000,baseline.data()+0x1e000,8192));++tested;
    }
    // Corrupt/foreign packages must never erase any application page.
    memcpy(memory,baseline.data(),baseline.size());((uint8_t*)LYKE_STAGE)[25]^=0x80;
    budget=-1;operations=0;assert(!install(&image));assert(!operations);
    printf("Real loader logic: %d interrupted commits recovered; loader/keymap/preferences preserved; corruption rejected\n",tested);
    return 0;
}

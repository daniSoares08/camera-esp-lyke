#include <assert.h>
#include "../image_format.h"
#ifdef _WIN32
__declspec(dllexport)
#endif
int validate_image(const uint8_t *p,size_t n)
{
    uint8_t guarded[LYKE_PACK_BLOCK+16];memset(guarded,0xa5,sizeof(guarded));
    int result=image_validate(p,n,guarded);
    for(unsigned i=LYKE_PACK_BLOCK;i<sizeof(guarded);++i)assert(guarded[i]==0xa5);
    return result;
}

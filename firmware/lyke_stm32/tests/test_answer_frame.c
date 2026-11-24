#include <assert.h>
#include <stdio.h>
#include "../answer_frame.h"
int main(void) {
    struct {unsigned before;AnswerFrame f;unsigned after;} guard={.before=0x1234,.after=0x9876};
    AnswerFrame *f=&guard.f;f->sequence=7;
    uint8_t bytes[768]={0};for(unsigned row=0;row<48;++row)bytes[row*16+row/8]|=0x80u>>(row%8);
    char line[100];snprintf(line,sizeof(line),"FRAME 7 12 40 192 200 %u",frame_checksum(bytes,768));
    assert(!frame_message(f,line));
    for(unsigned row=0;row<48;++row){
        unsigned at=snprintf(line,sizeof(line),"PIX 7 %u ",row);
        for(unsigned x=0;x<16;++x)snprintf(line+at+2*x,3,"%02x",bytes[row*16+x]);
        assert(!frame_message(f,line));
    }
    assert(frame_message(f,"FRAME_END 7")==1&&f->valid&&!memcmp(f->data,bytes,768));
    f->rows&=~(uint64_t)1;f->receiving=true;assert(frame_message(f,"FRAME_END 7")==-1);
    assert(frame_message(f,"FRAME 7 100 0 126 48 0")==-1);
    assert(frame_message(f,"FRAME 8 0 0 126 48 0")==-1);
    assert(frame_message(f,"FRAME 999999 0 0 126 48 0")==-1);
    assert(frame_message(f,"FRAME 7 0 0 126 65536 0")==-1);
    f->receiving=true;assert(frame_message(f,"PIX 7 48 00000000000000000000000000000000")==-1);
    assert(frame_message(f,"PIX 7 0 00000000000000000000000000000003")==-1);
    assert(frame_message(f,"PIX 7 0 0000000000000000000000000000000z")==-1);
    assert(guard.before==0x1234&&guard.after==0x9876);
    return 0;
}

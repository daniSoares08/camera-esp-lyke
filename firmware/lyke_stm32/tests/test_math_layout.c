#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../scientific.h"
#include "../math_symbols.h"
#include "../math_layout.h"
static unsigned char image[48][126];
static void pixel(int x,int y) {assert(x>=0&&x<126&&y>=8&&y<48);image[y][x]=1;}
static void save(const char *folder,const char *name)
{
    if (!folder) return;
    char path[512];snprintf(path,sizeof(path),"%s/%s.pbm",folder,name);
    FILE *f=fopen(path,"wb");assert(f);fprintf(f,"P1\n128 48\n");
    for(int y=0;y<48;++y) {for(int x=0;x<128;++x) fprintf(f,"%u ",x>=2?image[y][x-2]:0);fputc('\n',f);}fclose(f);
}
static void check(const char *folder,const char *name,const uint8_t *input,double expected)
{
    double variables[12]={0};variables[9]=7;variables[10]=2;variables[11]=3;
    Expression e={.at=input,.variables=variables,.angle=pi/180};double result=evaluate(&e);
    assert(!e.error && fabs(result-expected)<1e-10*fmax(1,fabs(expected)));
    memset(image,0,sizeof(image));
    unsigned used=math_draw(input,strlen((const char*)input),true,9,36,pixel);
    assert(used && used<=36);
    for (unsigned y=0;y<8;++y)for(unsigned x=0;x<126;++x)assert(!image[y][x]);
    save(folder,name);
}
int main(int argc,char **argv)
{
    const char *folder=argc>1?argv[1]:NULL;
    check(folder,"fracoes",(uint8_t[]){'1',FRAC,'2','+','3',FRAC,'4',0},1.25);
    check(folder,"raiz",(uint8_t[]){SQRT,'9','+','1','6',')',0},5);
    check(folder,"potencia",(uint8_t[]){'2','^','1','0','+','3',SQUARE,0},1033);
    check(folder,"mista",(uint8_t[]){'1',FRAC,'1',FRAC,'2',0},1.5);
    check(folder,"gregas",(uint8_t[]){'2',PI,'+',GREEK_DELTA,'+',GREEK_THETA,'+',GREEK_OMEGA,0},2*pi+12);
    check(folder,"raiz_fracao",(uint8_t[]){SQRT,'1',FRAC,'4',')',0},.5);
    check(folder,"cubica",(uint8_t[]){CBRT,'2','7',')','+','3',ROOT,'8',0},5);
    check(folder,"funcao",(uint8_t[]){SIN,'3','0',')','+',POW10,'2',')',0},100.5);
    check(folder,"precedencia",(uint8_t[]){'-','2','^','2','+','(','-','2',')',SQUARE,0},0);
    assert(variable_index(GREEK_DELTA)==9 && variable_index(GREEK_THETA)==10 && variable_index(GREEK_OMEGA)==11);
    assert(!math_draw((uint8_t[]){'5',NCR,'2',0},3,true,8,32,pixel));
    uint8_t long_input[96];for(unsigned i=0;i<94;++i)long_input[i]=i%2?'+':'1';long_input[94]='1';long_input[95]=0;
    memset(image,0,sizeof(image));assert(math_draw(long_input,95,true,8,32,pixel));assert(math_layout.shift>0);
    for (unsigned cursor=0;cursor<8;++cursor)
        assert(math_draw((uint8_t[]){SQRT,'1','2','+',GREEK_DELTA,')',SQUARE,0},cursor,true,8,36,pixel));
    puts("Mathematical rendering: fractions, roots, superscripts, Greek, precedence, cursor and scrolling OK");
    return 0;
}

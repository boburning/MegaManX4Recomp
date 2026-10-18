#include "mmx4_coop_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do {if(!(x)) {fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);}} while(0)
#define ACTOR 0x80173D60u
#define PARENT 0x80143028u
static uint8_t ram[0x200000],second[0xE4];
static unsigned ready=1,projected,alive[2]={1,1},writes;
static int owner=-1;
int mmx4_coop_ready(void) {return (int)ready;}
int mmx4_coop_projected(void) {return (int)projected;}
int mmx4_coop_alive(unsigned seat) {CHECK(seat<2);return (int)alive[seat];}
int mmx4_coop_lifecycle_script_owner(void) {return owner;}
uint8_t *mmx4_coop_second_body(void) {return second;}
uint8_t psx_mod_read_byte(uint32_t a) {return ram[a&0x1FFFFFu];}
uint16_t psx_mod_read_half(uint32_t a) {return psx_mod_read_byte(a)|(uint16_t)psx_mod_read_byte(a+1)<<8;}
uint32_t psx_mod_read_word(uint32_t a) {return psx_mod_read_half(a)|(uint32_t)psx_mod_read_half(a+2)<<16;}
void psx_mod_write_byte(uint32_t a,uint8_t v) {ram[a&0x1FFFFFu]=v;++writes;}
void psx_mod_write_half(uint32_t a,uint16_t v) {psx_mod_write_byte(a,(uint8_t)v);psx_mod_write_byte(a+1,(uint8_t)(v>>8));}
void psx_mod_write_word(uint32_t a,uint32_t v) {psx_mod_write_half(a,(uint16_t)v);psx_mod_write_half(a+2,(uint16_t)(v>>16));}
static void put(uint8_t *p,uint32_t v) {for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(v>>(8*i));}
static uint32_t word(const uint8_t *p) {return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint32_t hash(const uint8_t *p,unsigned n) {uint32_t v=2166136261u;while(n--)v=(v^*p++)*16777619u;return v;}

int main(void) {
    for(unsigned test=0;test<22;++test) {
        memset(ram,0,sizeof ram);memset(second,0,sizeof second);
        ready=1;projected=0;owner=-1;alive[0]=alive[1]=1;
        uint8_t *a=ram+(ACTOR&0x1FFFFFu),*p=ram+(MMX4_PLAYER&0x1FFFFFu);
        for(unsigned i=0;i<0x60;++i)a[i]=(uint8_t)(i*7+3);
        a[0]=0x41;a[1]=34;a[4]=1;a[5]=2;a[0x14]=0;
        put(a+0x50,PARENT);ram[PARENT&0x1FFFFFu]=1;ram[(PARENT+1)&0x1FFFFFu]=38;
        put(p+8,452u<<16);put(p+12,155u<<16);
        put(second+8,(515u<<16)+0x8000u);put(second+12,202u<<16);
        put(a+8,(450u<<16)+0x4000u);put(a+12,(149u<<16)+0x1234u);
        put(a+0x18,449u<<16);put(a+0x1C,148u<<16);
        uint32_t actor=ACTOR;
        if(test==1) {
            put(p+8,0xFFFC0000u);put(p+12,0xFFFE8000u);
            put(second+8,0x00018000u);put(second+12,0xFFF90000u);
            put(a+8,0xFFFA4000u);put(a+12,0xFFF89234u);
        }
        if(test==2)second[0x14]=2;
        ram[MMX4_PLAY&0x1FFFFFu]=6;ram[(MMX4_PLAY+12)&0x1FFFFFu]=6;
        if(test==3)ram[(MMX4_PLAY+1)&0x1FFFFFu]=2; /* Paused world drawing stays supported. */
        if(test==4)ready=0;
        if(test==5)projected=1;
        if(test==6)owner=0;
        if(test==7)owner=1;
        if(test==8)alive[0]=0; /* Native survivor graphic already follows P2. */
        if(test==9)alive[1]=0;
        if(test==10)ram[MMX4_PLAY&0x1FFFFFu]=7;
        if(test==11)ram[(MMX4_PLAY+12)&0x1FFFFFu]=5;
        if(test==12)ram[(MMX4_PLAY+13)&0x1FFFFFu]=1;
        if(test==13)++actor;
        if(test==14)a[1]=35;
        if(test==15)a[4]=2;
        if(test==16)a[5]=3; /* Completion/flyaway is not another free-course target. */
        if(test==17)put(a+0x50,PARENT+1);
        if(test==18)ram[(PARENT+1)&0x1FFFFFu]=39;
        if(test==19)second[0x14]=0x80;
        if(test>=20)a[5]=(uint8_t)(test-20); /* Both opening scan phases. */
        uint8_t copy[0x60],original[0x60],old_second[0xE4];
        memset(copy,0xCC,sizeof copy);memcpy(original,a,sizeof original);memcpy(old_second,second,sizeof second);
        uint32_t before=hash(ram,sizeof ram);writes=0;
        int ok=mmx4_coop_assessment_copy(actor,copy);
        CHECK(ok==(test<4 || test>=20));
        CHECK(writes==0 && hash(ram,sizeof ram)==before);
        CHECK(!memcmp(second,old_second,sizeof second));
        if(ok) {
            uint8_t expected[0x60];memcpy(expected,original,sizeof expected);
            uint32_t dx=word(second+8)-word(p+8),dy=word(second+12)-word(p+12);
            put(expected+8,word(original+8)+dx);put(expected+12,word(original+12)+dy);
            put(expected+0x18,word(original+0x18)+dx);put(expected+0x1C,word(original+0x1C)+dy);
            expected[0x14]=second[0x14];
            /* Includes every native grading/countdown byte and parent link. */
            CHECK(!memcmp(expected,copy,sizeof copy));
            CHECK(word(copy+8)==word(second+8)-(2u<<16)+0x4000u);
            CHECK(word(copy+12)==word(second+12)-(6u<<16)+0x1234u);
        }else for(unsigned i=0;i<sizeof copy;++i)CHECK(copy[i]==0xCC);
    }
    CHECK(!mmx4_coop_assessment_copy(ACTOR,NULL));
    puts("PASS: analysis copy, native state preservation and scope guards");
    return 0;
}

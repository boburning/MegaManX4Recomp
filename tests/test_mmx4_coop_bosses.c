/* State/ownership checks complement the original-disc two-encounter run. */
#include "mmx4_coop_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t ram[0x200000];
static int enabled,projected,local,ready=1;
static PSXModFunctionFilterCallback transition;
static struct {uint32_t address;PSXModFunctionEntryCallback fn;} hooks[20];
static unsigned count;
uint8_t psx_mod_read_byte(uint32_t a){return ram[a&0x1fffff];}
void psx_mod_write_byte(uint32_t a,uint8_t v){ram[a&0x1fffff]=v;}
void psx_mod_write_word(uint32_t a,uint32_t v){for(unsigned i=0;i<4;++i)psx_mod_write_byte(a+i,(uint8_t)(v>>(8*i)));}
int psx_mod_local_view_scope(void){return local;}
int mmx4_coop_projected(void){return projected;}
int mmx4_coop_ready(void){return ready;}
uint32_t mmx4_coop_counterpart_story_text(void){return 0x9f046000u;}
int psx_mod_current_option_value(const char *key,char *value,uint32_t cap){
    CHECK(!strcmp(key,"both_story_bosses") && cap>=6);strcpy(value,enabled?"true":"false");return 1;
}
void psx_mod_counter_add(const char *name,uint32_t v){(void)name;CHECK(v==1);}
int psx_mod_register_function_filter_plugin(const char *id,uint32_t a,PSXModFunctionFilterCallback fn){
    (void)id;CHECK(a==0x800200D4u);transition=fn;return 1;
}
int psx_mod_register_instruction_plugin(const char *id,uint32_t a,uint32_t word,PSXModFunctionEntryCallback fn){
    (void)id;(void)word;CHECK(count<20);hooks[count].address=a;hooks[count++].fn=fn;return 1;
}
int mmx4_coop_finish(CPUState *cpu,uint32_t result){cpu->gpr[2]=result;return 1;}
static void hook(CPUState *cpu,uint32_t a){
    for(unsigned i=0;i<count;++i)if(hooks[i].address==a){hooks[i].fn(cpu,a);return;}
    CHECK(0);
}
static void capture(uint8_t bytes[7]){
    Mmx4SnapshotIO s={bytes,NULL,7,0,MMX4_SNAPSHOT_WRITE,1};
    CHECK(mmx4_coop_bosses_snapshot(&s) && s.pos==7);
}
int main(void){
    CPUState cpu={0};uint8_t first[7],second[7];
    psx_mod_write_byte(MMX4_PLAY+0xC,11);
    psx_mod_write_byte(MMX4_PLAY+0xF,1);
    mmx4_coop_bosses_activate();
    hook(&cpu,0x8008D324u);CHECK(!transition(&cpu,0x800200D4u));
    for(unsigned campaign=0;campaign<2;++campaign){
        enabled=1;mmx4_coop_bosses_activate();
        psx_mod_write_byte(MMX4_PLAY+0x43,(uint8_t)campaign);
        psx_mod_write_byte(MMX4_PLAY+0xF,1);
        capture(first);
        CHECK(!transition(&cpu,0x800200D4u)); /* Death/menu exit is not victory. */
        hook(&cpu,campaign?0x800810A8u:0x8008D324u);
        projected=1;CHECK(!transition(&cpu,0x800200D4u));projected=0;
        local=1;CHECK(!transition(&cpu,0x800200D4u));local=0;
        CHECK(transition(&cpu,0x800200D4u));
        CHECK(psx_mod_read_byte(MMX4_PLAY)==4);
        mmx4_coop_bosses_new_stage();
        psx_mod_write_byte(MMX4_PLAY+0x1D,0); /* Native loader clears it. */
        psx_mod_write_byte(MMX4_PLAY,5);
        mmx4_coop_bosses_stage_initialization();
        CHECK(psx_mod_read_byte(MMX4_PLAY)==5 && psx_mod_read_byte(MMX4_PLAY+0x1D)==1);
        CHECK(psx_mod_read_byte(MMX4_PLAY+0x43)==campaign);
        capture(second);
        for(unsigned i=0;i<4;++i){
            unsigned reg=hooks[i].address==0x8001302Cu?3u:2u;
            cpu.gpr[reg]=campaign;hooks[i].fn(&cpu,hooks[i].address);
            CHECK(cpu.gpr[reg]==(campaign^1u));
            CHECK(psx_mod_read_byte(MMX4_PLAY+0x43)==campaign);
        }
        hook(&cpu,0x800224D4u);CHECK(cpu.gpr[4]==(campaign^1u));
        hook(&cpu,0x80022554u);CHECK(cpu.gpr[4]==0x9f046000u);
        hook(&cpu,0x80022684u);CHECK(cpu.gpr[2]==(campaign^1u));
        hook(&cpu,0x800227B4u);CHECK(cpu.gpr[2]==(campaign^1u));
        hook(&cpu,0x8002284Cu);CHECK(cpu.gpr[2]==(campaign^1u));
        hook(&cpu,0x80022880u);CHECK(cpu.gpr[3]==(campaign^1u));
        hook(&cpu,0x800228B4u);CHECK(cpu.gpr[3]==(campaign^1u));
        hook(&cpu,0x800228F4u);CHECK(cpu.gpr[2]==(campaign^1u));
        hook(&cpu,0x80022928u);CHECK(cpu.gpr[4]==(campaign^1u));
        hook(&cpu,campaign?0x8008D324u:0x800810A8u);
        CHECK(cpu.gpr[2]==(campaign?0x40u:1u));
        CHECK(!transition(&cpu,0x800200D4u)); /* No third battle. */
        Mmx4SnapshotIO s={NULL,first,7,0,MMX4_SNAPSHOT_READ,1};
        CHECK(mmx4_coop_bosses_snapshot(&s));
        cpu.gpr[2]=campaign;hook(&cpu,0x80013148u);CHECK(cpu.gpr[2]==campaign);
        s=(Mmx4SnapshotIO){NULL,second,7,0,MMX4_SNAPSHOT_READ,1};
        CHECK(mmx4_coop_bosses_snapshot(&s));
        mmx4_coop_bosses_new_stage();
        cpu.gpr[2]=campaign;hook(&cpu,0x80013148u);CHECK(cpu.gpr[2]==campaign);
        enabled=0;mmx4_coop_bosses_activate();
        s=(Mmx4SnapshotIO){NULL,second,7,0,MMX4_SNAPSHOT_VALIDATE,1};
        CHECK(!mmx4_coop_bosses_snapshot(&s));
    }
    puts("Optional story encounter state and campaign ownership passed.");return 0;
}

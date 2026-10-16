#include "mmx4_coop_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);exit(1);}}while(0)
#define BOSS 0x8013BED0u
#define SAVE_BYTES (8+48*3+48*3*2)
static uint8_t ram[0x200000];
static int enabled,local,ready=1,per_player,projected;
static unsigned shot_type=255,hero;
static struct {uint32_t address;PSXModFunctionEntryCallback fn;} hooks[32];
static unsigned count;
uint8_t psx_mod_read_byte(uint32_t a){return ram[a&0x1fffff];}
uint32_t psx_mod_read_word(uint32_t a){return psx_mod_read_byte(a)|psx_mod_read_byte(a+1)<<8|
    psx_mod_read_byte(a+2)<<16|(uint32_t)psx_mod_read_byte(a+3)<<24;}
void psx_mod_write_byte(uint32_t a,uint8_t v){ram[a&0x1fffff]=v;}
static void word(uint32_t a,uint32_t v){for(unsigned i=0;i<4;++i)psx_mod_write_byte(a+i,(uint8_t)(v>>(8*i)));}
int psx_mod_local_view_scope(void){return local;}
int mmx4_coop_ready(void){return ready;}
int mmx4_coop_projected(void){return projected;}
int psx_mod_current_option_value(const char *key,char *value,uint32_t cap){
    CHECK(cap>=16);
    if(!strcmp(key,"boss_scaling"))strcpy(value,enabled?"true":"false");
    else {CHECK(!strcmp(key,"boss_cooldowns"));if(per_player==2)return 0;strcpy(value,per_player?"per_player":"shared");}
    return 1;
}
void psx_mod_counter_add(const char *name,uint32_t v){(void)name;CHECK(v<=255);}
int psx_mod_register_instruction_plugin(const char *id,uint32_t a,uint32_t expected,PSXModFunctionEntryCallback fn){
    (void)id;(void)expected;CHECK(count<32);hooks[count].address=a;hooks[count++].fn=fn;return 1;
}
static void hook(CPUState *cpu,uint32_t a){
    for(unsigned i=0;i<count;++i)if(hooks[i].address==a){hooks[i].fn(cpu,a);return;}
    CHECK(0);
}
static void actor(unsigned kind,unsigned hp){
    memset(ram,0,sizeof ram);mmx4_coop_balance_reset();
    psx_mod_write_byte(BOSS,0x41);psx_mod_write_byte(BOSS+1,(uint8_t)kind);
    psx_mod_write_byte(BOSS+4,1);psx_mod_write_byte(BOSS+0x5C,(uint8_t)hp);
}
static unsigned hit(unsigned amount,int rejected){
    unsigned hp=psx_mod_read_byte(BOSS+0x5C);
    CPUState cpu={0};cpu.gpr[4]=cpu.gpr[16]=BOSS;
    cpu.gpr[2]=amount;cpu.gpr[3]=hp-amount;
    mmx4_coop_balance_begin(BOSS);
    hook(&cpu,0x8002DF08u);
    int left=(int32_t)cpu.gpr[3];
    psx_mod_write_byte(BOSS+0x5C,(uint8_t)(left>0?left:0));
    if(rejected){hook(&cpu,0x8007D8CCu);psx_mod_write_byte(BOSS+0x5C,(uint8_t)hp);}
    mmx4_coop_balance_end();
    return psx_mod_read_byte(BOSS+0x5C);
}
static unsigned timed_hit(unsigned seat,unsigned amount,unsigned ticks) {
    projected=(int)seat;
    unsigned hp=psx_mod_read_byte(BOSS+0x5C);
    CPUState cpu={0};cpu.gpr[4]=cpu.gpr[16]=BOSS;cpu.gpr[2]=amount;cpu.gpr[3]=hp-amount;
    cpu.gpr[5]=0x801406F8u;psx_mod_write_byte(cpu.gpr[5]+1,(uint8_t)shot_type);
    psx_mod_write_byte(MMX4_PLAYER+2,(uint8_t)hero);
    mmx4_coop_balance_begin(BOSS);hook(&cpu,0x8002DF08u);
    psx_mod_write_byte(BOSS+0x5C,(uint8_t)cpu.gpr[3]);
    cpu.gpr[2]=ticks;hook(&cpu,0x8008CEC0u);
    mmx4_coop_balance_end();projected=0;return cpu.gpr[2];
}
int main(void){
    CPUState cpu={0};uint8_t saved[SAVE_BYTES];
    mmx4_coop_balance_activate();actor(73,48);CHECK(hit(1,0)==47);
    enabled=1;mmx4_coop_balance_activate();actor(73,48);
    /* Exact doubled survival, including odd one-point hits and death. */
    for(unsigned n=1;n<=96;++n)CHECK(hit(1,0)==48-n/2);
    CHECK(!psx_mod_read_byte(BOSS+0x5C));
    actor(73,48);CHECK(hit(3,0)==47);CHECK(hit(5,0)==44);CHECK(hit(127,0)==0);
    actor(64,48);CHECK(hit(1,0)==48);CHECK(hit(1,1)==48);CHECK(hit(1,0)==47);
    /* Ordinary enemies and Iris's armor never acquire extra durability. */
    actor(58,48);CHECK(hit(4,0)==44);
    actor(19,4);CHECK(hit(2,0)==2);
    actor(18,48);CHECK(hit(2,0)==47);
    actor(43,48);CHECK(hit(2,0)==47);
    cpu.gpr[16]=BOSS;cpu.gpr[2]=43;hook(&cpu,0x80064CD4u);CHECK(cpu.gpr[2]==45);
    cpu.gpr[2]=64;hook(&cpu,0x80063AB4u);CHECK(cpu.gpr[2]==32);
    /* Train miniboss body/three destructible sections contribute native HP
     * deltas to the shared bar; halve each section before that aggregation. */
    actor(46,32);CHECK(hit(4,0)==30);
    actor(47,16);for(unsigned n=1;n<=32;++n)CHECK(hit(1,0)==16-n/2);
    actor(33,48);CHECK(hit(4,0)==46);
    /* Preserve the native four-point reaction threshold despite halved HP. */
    actor(73,48);mmx4_coop_balance_begin(BOSS);
    cpu.gpr[4]=BOSS;cpu.gpr[2]=4;hook(&cpu,0x8002DF08u);CHECK(cpu.gpr[3]==46);
    cpu.gpr[17]=BOSS;cpu.gpr[16]=2;hook(&cpu,0x80088EF8u);CHECK(cpu.gpr[16]==4);
    mmx4_coop_balance_end();
    actor(66,48);CHECK(hit(4,0)==44);
    actor(66,48);psx_mod_write_byte(BOSS+2,1);CHECK(hit(4,0)==46);
    /* Native health-bar ownership also identifies unlisted minibosses. */
    actor(34,48);word(MMX4_PLAY+0x20,BOSS);psx_mod_write_byte(MMX4_PLAY+0x24,1);
    CHECK(hit(4,0)==46);
    actor(73,127);CHECK(hit(1,0)==126);
    /* Final Sigma's two pools keep distinct fractional damage. */
    actor(74,48);psx_mod_write_byte(BOSS+0x91,1);CHECK(hit(1,0)==48);
    psx_mod_write_byte(BOSS+0x91,2);CHECK(hit(1,0)==48);
    psx_mod_write_byte(BOSS+0x91,1);CHECK(hit(1,0)==47);
    psx_mod_write_byte(BOSS+0x5C,48);psx_mod_write_byte(BOSS+0x91,2);CHECK(hit(1,0)==47);
    actor(73,48);CHECK(hit(1,0)==48);
    Mmx4SnapshotIO io={saved,NULL,sizeof saved,0,MMX4_SNAPSHOT_WRITE,1};
    CHECK(mmx4_coop_balance_snapshot(&io) && io.pos==sizeof saved);
    uint32_t digest=mmx4_coop_balance_digest(1);
    CHECK(hit(1,0)==47 && digest!=mmx4_coop_balance_digest(1));
    io=(Mmx4SnapshotIO){NULL,saved,sizeof saved,0,MMX4_SNAPSHOT_READ,1};
    CHECK(mmx4_coop_balance_snapshot(&io) && digest==mmx4_coop_balance_digest(1));
    psx_mod_write_byte(BOSS+0x5C,48);CHECK(hit(1,0)==47);
    /* Slot reuse and initialization cannot inherit half a hit. */
    actor(73,48);CHECK(hit(1,0)==48);mmx4_coop_balance_forget(BOSS);CHECK(hit(1,0)==48);
    psx_mod_write_byte(BOSS+4,0);CHECK(hit(1,0)==48);psx_mod_write_byte(BOSS+4,1);
    cpu.gpr[16]=BOSS;cpu.gpr[2]=64;hook(&cpu,0x8008CEC0u);CHECK(cpu.gpr[2]==32);
    cpu.gpr[2]=1;hook(&cpu,0x8008CEC0u);CHECK(cpu.gpr[2]==1);
    cpu.gpr[2]=255;hook(&cpu,0x8008CEC0u);CHECK(cpu.gpr[2]==255);
    digest=mmx4_coop_balance_digest(1);local=1;
    cpu.gpr[4]=BOSS;cpu.gpr[2]=1;cpu.gpr[3]=47;hook(&cpu,0x8002DF08u);
    CHECK(cpu.gpr[3]==47 && digest==mmx4_coop_balance_digest(1));local=0;
    mmx4_coop_balance_begin(BOSS);CHECK(!mmx4_coop_balance_snapshot_ready());
    mmx4_coop_balance_end();CHECK(mmx4_coop_balance_snapshot_ready());
    enabled=0;mmx4_coop_balance_activate();
    io=(Mmx4SnapshotIO){NULL,saved,sizeof saved,0,MMX4_SNAPSHOT_VALIDATE,1};
    CHECK(!mmx4_coop_balance_snapshot(&io));
    /* A fresh/missing choice defaults to independent full native cooldowns.
     * The first hit leaves the partner eligible; then both timers coexist. */
    enabled=1;per_player=2;mmx4_coop_balance_activate();actor(73,48);
    CHECK(timed_hit(0,1,64)==1);
    CHECK(!mmx4_coop_balance_hit_allowed(BOSS,0) && mmx4_coop_balance_hit_allowed(BOSS,1));
    CHECK(timed_hit(1,1,64)==63);
    CHECK(!mmx4_coop_balance_hit_allowed(BOSS,0) && !mmx4_coop_balance_hit_allowed(BOSS,1));
    CHECK(psx_mod_read_byte(BOSS+0x5C)==47);
    for(unsigned i=0;i<62;++i){mmx4_coop_balance_begin(BOSS);mmx4_coop_balance_end();}
    CHECK(!mmx4_coop_balance_hit_allowed(BOSS,0) && !mmx4_coop_balance_hit_allowed(BOSS,1));
    mmx4_coop_balance_begin(BOSS);mmx4_coop_balance_end();
    CHECK(mmx4_coop_balance_hit_allowed(BOSS,0) && !mmx4_coop_balance_hit_allowed(BOSS,1));
    digest=mmx4_coop_balance_digest(1);
    io=(Mmx4SnapshotIO){saved,NULL,sizeof saved,0,MMX4_SNAPSHOT_WRITE,1};
    CHECK(mmx4_coop_balance_snapshot(&io));
    mmx4_coop_balance_begin(BOSS);mmx4_coop_balance_end();CHECK(mmx4_coop_balance_hit_allowed(BOSS,1));
    CHECK(digest!=mmx4_coop_balance_digest(1));
    io=(Mmx4SnapshotIO){NULL,saved,sizeof saved,0,MMX4_SNAPSHOT_READ,1};
    CHECK(mmx4_coop_balance_snapshot(&io) && digest==mmx4_coop_balance_digest(1));
    CHECK(!mmx4_coop_balance_hit_allowed(BOSS,1));
    mmx4_coop_balance_forget(BOSS);CHECK(mmx4_coop_balance_hit_allowed(BOSS,1));
    /* Independent timers retain Zero's short first/second saber chain,
     * followed by full immunity after the finisher. Both seats qualify. */
    for(unsigned seat=0;seat<2;++seat)for(unsigned kind=57;kind<=68;kind+=11) {
        actor(kind,48);hero=1;shot_type=24;
        CHECK(timed_hit(seat,2,64)==1);
        for(unsigned tick=0;tick<3;++tick) {
            mmx4_coop_balance_begin(BOSS);mmx4_coop_balance_end();
            CHECK(!mmx4_coop_balance_hit_allowed(BOSS,seat));
            CHECK(mmx4_coop_balance_hit_allowed(BOSS,seat^1u));
        }
        mmx4_coop_balance_begin(BOSS);mmx4_coop_balance_end();
        CHECK(mmx4_coop_balance_hit_allowed(BOSS,seat));
        shot_type=25;CHECK(timed_hit(seat,2,64)==1);
        for(unsigned tick=0;tick<4;++tick){mmx4_coop_balance_begin(BOSS);mmx4_coop_balance_end();}
        CHECK(mmx4_coop_balance_hit_allowed(BOSS,seat));
        shot_type=26;CHECK(timed_hit(seat,4,64)==1);
        for(unsigned tick=0;tick<4;++tick){mmx4_coop_balance_begin(BOSS);mmx4_coop_balance_end();}
        CHECK(!mmx4_coop_balance_hit_allowed(BOSS,seat));
        CHECK(psx_mod_read_byte(BOSS+0x5C)==44);
    }
    actor(57,48);shot_type=24;hero=0;CHECK(timed_hit(0,2,64)==1);
    for(unsigned tick=0;tick<4;++tick){mmx4_coop_balance_begin(BOSS);mmx4_coop_balance_end();}
    CHECK(!mmx4_coop_balance_hit_allowed(BOSS,0));shot_type=255;hero=0;
    per_player=0;mmx4_coop_balance_activate();
    io=(Mmx4SnapshotIO){NULL,saved,sizeof saved,0,MMX4_SNAPSHOT_VALIDATE,1};
    CHECK(!mmx4_coop_balance_snapshot(&io));
    puts("Boss scaling preserves weak hits, separate pools, rejected hits and snapshots.");return 0;
}

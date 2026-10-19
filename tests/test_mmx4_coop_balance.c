#include "mmx4_coop_internal.h"
uint64_t psx_cycle_count;
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);exit(1);}}while(0)
#define BOSS 0x8013BED0u
#define SAVE_BYTES (8+48*3+48*3*2+2*17*4+48*3*2*4+48*3*2*2+48*3*6)
static uint8_t ram[0x200000];
static int enabled,local,ready=1,per_player,projected;
static unsigned shot_type=255,hero;
static uint32_t shot_address=0x801406F8u;
static uint32_t damage_table;
static unsigned native_clangs;
static struct {uint32_t address;PSXModFunctionEntryCallback fn;} hooks[32];
static unsigned count;
uint8_t psx_mod_read_byte(uint32_t a){return ram[a&0x1fffff];}
uint32_t psx_mod_read_word(uint32_t a){return psx_mod_read_byte(a)|psx_mod_read_byte(a+1)<<8|
    psx_mod_read_byte(a+2)<<16|(uint32_t)psx_mod_read_byte(a+3)<<24;}
void psx_mod_write_byte(uint32_t a,uint8_t v){ram[a&0x1fffff]=v;}
static void word(uint32_t a,uint32_t v){for(unsigned i=0;i<4;++i)psx_mod_write_byte(a+i,(uint8_t)(v>>(8*i)));}
void psx_mod_write_word(uint32_t a,uint32_t v){word(a,v);}
uint16_t psx_mod_read_half(uint32_t a){return (uint16_t)(psx_mod_read_byte(a)|psx_mod_read_byte(a+1)<<8);}
void psx_mod_write_half(uint32_t a,uint16_t v){psx_mod_write_byte(a,(uint8_t)v);psx_mod_write_byte(a+1,(uint8_t)(v>>8));}
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
int psx_mod_register_function_entry_plugin(const char *id,uint32_t a,PSXModFunctionEntryCallback fn) {
    return psx_mod_register_instruction_plugin(id,a,0,fn);
}
static void hook(CPUState *cpu,uint32_t a){
    for(unsigned i=0;i<count;++i)if(hooks[i].address==a){hooks[i].fn(cpu,a);return;}
    CHECK(0);
}
/* The relevant original DE30 contract: a rejection table clangs and stamps
 * a fresh contact before the original boss caller can expire its countdown.
 * This stub does not choose/project a different table or shorten a timer. */
uint32_t mmx4_coop_call(CPUState *cpu,uint32_t address,uint32_t actor,uint32_t shot) {
    CHECK(address==0x8002DE30u);
    unsigned key=psx_mod_read_byte(shot+0x64),type=psx_mod_read_byte(shot+1);
    if(key && key==psx_mod_read_byte(actor+0x65))return 0x7Eu;
    psx_mod_write_byte(actor+0x65,(uint8_t)key);
    uint32_t table=psx_mod_read_word(actor+0x58);
    unsigned reaction=psx_mod_read_byte(table+type*2u);
    if(reaction==255){++native_clangs;return 0x7Fu;}
    unsigned amount=psx_mod_read_byte(table+type*2u+1u);
    cpu->gpr[4]=actor;cpu->gpr[5]=shot;cpu->gpr[7]=table;
    cpu->gpr[2]=amount;cpu->gpr[3]=psx_mod_read_byte(actor+0x5C)-amount;
    hook(cpu,0x8002DF08u);
    psx_mod_write_byte(actor+0x5C,(uint8_t)cpu->gpr[3]);
    return type+1u;
}
int mmx4_coop_finish(CPUState *cpu,uint32_t value){cpu->gpr[2]=value;return 1;}
static unsigned contact(CPUState *cpu,unsigned seat) {
    if(!mmx4_coop_balance_contact_allowed(cpu->gpr[4],cpu->gpr[5],seat))return 0x7Eu;
    if(mmx4_coop_balance_project_contact(cpu,0x8002DE30u))return cpu->gpr[2];
    return mmx4_coop_call(cpu,0x8002DE30u,cpu->gpr[4],cpu->gpr[5]);
}
static void actor(unsigned kind,unsigned hp){
    memset(ram,0,sizeof ram);mmx4_coop_balance_reset();
    psx_mod_write_byte(BOSS,0x41);psx_mod_write_byte(BOSS+1,(uint8_t)kind);
    psx_mod_write_byte(BOSS+4,1);psx_mod_write_byte(BOSS+0x5C,(uint8_t)hp);
}
static uint32_t memory_digest(void) {
    uint32_t value=2166136261u;
    for(unsigned i=0;i<sizeof ram;++i)value=(value^ram[i])*16777619u;
    return value;
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
    cpu.gpr[5]=shot_address;psx_mod_write_byte(cpu.gpr[5]+1,(uint8_t)shot_type);
    cpu.gpr[7]=damage_table;
    psx_mod_write_byte(MMX4_PLAYER+2,(uint8_t)hero);
    mmx4_coop_balance_begin(BOSS);hook(&cpu,0x8002DF08u);
    psx_mod_write_byte(BOSS+0x5C,(uint8_t)cpu.gpr[3]);
    word(BOSS+0x58,0x801060F0u);
    word(0x8008CEC0u,0xA202008Bu);
    cpu.gpr[2]=ticks;hook(&cpu,0x8008CEC0u);
    psx_mod_write_byte(BOSS+0x8B,(uint8_t)cpu.gpr[2]);
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
    /* Character, not controller seat, determines the additional 20%.
     * Carry small hits exactly in both cooldown modes, including rejected
     * contacts and mixed X/Zero attacks. No new contact veto is introduced. */
    for(unsigned mode=0;mode<2;++mode)for(unsigned seat=0;seat<2;++seat) {
        per_player=(int)mode;mmx4_coop_balance_activate();actor(57,48);
        projected=(int)seat;psx_mod_write_byte(MMX4_PLAYER+2,1);
        for(unsigned n=1;n<=20;++n)CHECK(hit(1,0)==48-(n*4)/10);
        CHECK(psx_mod_read_byte(BOSS+0x5C)==40); /* Formerly 38. */
        actor(57,48);psx_mod_write_byte(MMX4_PLAYER+2,1);
        CHECK(hit(2,0)==48);CHECK(hit(2,1)==48);CHECK(hit(3,0)==46);
        actor(57,48);unsigned units=0;
        for(unsigned n=0;n<20;++n) {
            unsigned character=n%2;psx_mod_write_byte(MMX4_PLAYER+2,(uint8_t)character);
            units+=character?4:5;CHECK(hit(1,0)==48-units/10);
        }
        actor(57,48);psx_mod_write_byte(MMX4_PLAYER+2,1);enabled=0;
        mmx4_coop_balance_activate();CHECK(hit(2,0)==46);enabled=1;
    }
    projected=0;per_player=0;mmx4_coop_balance_activate();
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
    /* A fresh/missing choice defaults to independent half-length cooldowns.
     * The first hit leaves the partner eligible; then both timers coexist. */
    enabled=1;per_player=2;mmx4_coop_balance_activate();actor(73,48);
    CHECK(timed_hit(0,1,64)==1);
    /* The native expiry tick must not make the visual recovery disappear
     * while one attacker remains immune. A render query changes no state. */
    CHECK(mmx4_coop_balance_flash_ticks(BOSS)==32);
    digest=mmx4_coop_balance_digest(1);local=1;
    CHECK(mmx4_coop_balance_flash_ticks(BOSS)==32);
    CHECK(mmx4_coop_balance_digest(1)==digest);local=0;
    CHECK(!mmx4_coop_balance_hit_allowed(BOSS,0) && mmx4_coop_balance_hit_allowed(BOSS,1));
    CHECK(timed_hit(1,1,64)==31);
    CHECK(mmx4_coop_balance_flash_ticks(BOSS)==32);
    CHECK(!mmx4_coop_balance_hit_allowed(BOSS,0) && !mmx4_coop_balance_hit_allowed(BOSS,1));
    CHECK(psx_mod_read_byte(BOSS+0x5C)==47);
    for(unsigned i=0;i<30;++i){mmx4_coop_balance_begin(BOSS);mmx4_coop_balance_end();}
    CHECK(!mmx4_coop_balance_hit_allowed(BOSS,0) && !mmx4_coop_balance_hit_allowed(BOSS,1));
    mmx4_coop_balance_begin(BOSS);mmx4_coop_balance_end();
    CHECK(mmx4_coop_balance_hit_allowed(BOSS,0) && !mmx4_coop_balance_hit_allowed(BOSS,1));
    CHECK(mmx4_coop_balance_flash_ticks(BOSS)==1);
    digest=mmx4_coop_balance_digest(1);
    io=(Mmx4SnapshotIO){saved,NULL,sizeof saved,0,MMX4_SNAPSHOT_WRITE,1};
    CHECK(mmx4_coop_balance_snapshot(&io));
    mmx4_coop_balance_begin(BOSS);mmx4_coop_balance_end();CHECK(mmx4_coop_balance_hit_allowed(BOSS,1));
    CHECK(!mmx4_coop_balance_flash_ticks(BOSS));
    CHECK(digest!=mmx4_coop_balance_digest(1));
    io=(Mmx4SnapshotIO){NULL,saved,sizeof saved,0,MMX4_SNAPSHOT_READ,1};
    CHECK(mmx4_coop_balance_snapshot(&io) && digest==mmx4_coop_balance_digest(1));
    CHECK(!mmx4_coop_balance_hit_allowed(BOSS,1));
    CHECK(mmx4_coop_balance_flash_ticks(BOSS)==1);
    mmx4_coop_balance_forget(BOSS);CHECK(mmx4_coop_balance_hit_allowed(BOSS,1));
    CHECK(!mmx4_coop_balance_flash_ticks(BOSS));
    /* Hidden Sigma health banks and recycled/dead actors must not borrow
     * another pool's recovery flash; only an active boss can display it. */
    actor(74,48);psx_mod_write_byte(BOSS+0x91,1);CHECK(timed_hit(0,2,64)==1);
    psx_mod_write_byte(BOSS+0x91,2);CHECK(!mmx4_coop_balance_flash_ticks(BOSS));
    CHECK(timed_hit(1,2,10)==1);CHECK(mmx4_coop_balance_flash_ticks(BOSS)==5);
    psx_mod_write_byte(BOSS+0x91,1);CHECK(mmx4_coop_balance_flash_ticks(BOSS)==31);
    ready=0;CHECK(!mmx4_coop_balance_flash_ticks(BOSS));ready=1;
    psx_mod_write_byte(BOSS+0x5C,0);CHECK(!mmx4_coop_balance_flash_ticks(BOSS));
    psx_mod_write_byte(BOSS+0x5C,48);psx_mod_write_byte(BOSS+4,2);
    CHECK(!mmx4_coop_balance_flash_ticks(BOSS));psx_mod_write_byte(BOSS+4,1);
    psx_mod_write_byte(BOSS+1,58);CHECK(!mmx4_coop_balance_flash_ticks(BOSS));
    psx_mod_write_byte(BOSS+1,74);psx_mod_write_byte(BOSS,0);
    CHECK(!mmx4_coop_balance_flash_ticks(BOSS));
    CHECK(!mmx4_coop_balance_flash_ticks(BOSS+1));
    /* Independent timers retain Zero's short first/second saber chain,
     * followed by half-length finisher/aerial immunity. Both seats qualify. */
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
        CHECK(psx_mod_read_byte(BOSS+0x5C)==45); /* Eight raw Zero damage * .4. */
        /* Finisher and upgraded aerial saber use32 of the native64 ticks,
         * with no extra tick and no immunity imposed on the other hero. */
        for(unsigned tick=4;tick<31;++tick){mmx4_coop_balance_begin(BOSS);mmx4_coop_balance_end();}
        CHECK(!mmx4_coop_balance_hit_allowed(BOSS,seat));
        mmx4_coop_balance_begin(BOSS);mmx4_coop_balance_end();
        CHECK(mmx4_coop_balance_hit_allowed(BOSS,seat));
        shot_type=34;CHECK(timed_hit(seat,2,64)==1);
        for(unsigned tick=0;tick<31;++tick){
            mmx4_coop_balance_begin(BOSS);mmx4_coop_balance_end();
            CHECK(!mmx4_coop_balance_hit_allowed(BOSS,seat));
            CHECK(mmx4_coop_balance_hit_allowed(BOSS,seat^1u));
        }
        mmx4_coop_balance_begin(BOSS);mmx4_coop_balance_end();
        CHECK(mmx4_coop_balance_hit_allowed(BOSS,seat));
    }
    actor(57,48);shot_type=24;hero=0;CHECK(timed_hit(0,2,64)==1);
    for(unsigned tick=0;tick<4;++tick){mmx4_coop_balance_begin(BOSS);mmx4_coop_balance_end();}
    CHECK(!mmx4_coop_balance_hit_allowed(BOSS,0));shot_type=255;hero=0;
    /* Native short aerial/finisher recovery must admit the next actual
     * sword and changed native pose, while preserving duplicate contacts.
     * A missed blade is not consumed, each owner/bank remains independent,
     * and recycled targets and restored snapshots retain correct lifetimes. */
    for(unsigned seat=0;seat<2;++seat)for(unsigned type=26;type<=34;type+=type==28?6:1) {
        actor(57,48);hero=1;shot_type=type;projected=(int)seat;
        psx_mod_write_byte(MMX4_PLAYER+2,1);
        const uint32_t shot=0x801406F8u;
        psx_mod_write_byte(shot+1,(uint8_t)type);psx_mod_write_byte(shot+4,0);
        cpu.gpr[4]=shot;hook(&cpu,0x80097EECu);
        CHECK(mmx4_coop_balance_contact_allowed(BOSS,shot,seat));
        CHECK(timed_hit(seat,2,64)==1);
        CHECK(mmx4_coop_balance_flash_ticks(BOSS)==4);
        for(unsigned tick=0;tick<4;++tick){mmx4_coop_balance_begin(BOSS);mmx4_coop_balance_end();}
        projected=(int)seat;
        CHECK(mmx4_coop_balance_hit_allowed(BOSS,seat));
        psx_mod_write_byte(shot+0x64,3);psx_mod_write_byte(BOSS+0x65,2);
        CHECK(mmx4_coop_balance_contact_allowed(BOSS,shot,seat)); /* Same blade, new pose. */
        CHECK(psx_mod_read_byte(BOSS+0x65)==2); /* Native check decides. */
        /* A held identical key is still rejected by retail DE30; repeated
         * admission queries must never erase it after private recovery. */
        psx_mod_write_byte(BOSS+0x65,3);
        for(unsigned repeat=0;repeat<3;++repeat) {
            CHECK(mmx4_coop_balance_contact_allowed(BOSS,shot,seat));
            CHECK(psx_mod_read_byte(BOSS+0x65)==3);
            CHECK(mmx4_coop_call(&cpu,0x8002DE30u,BOSS,shot)==0x7Eu);
        }
        psx_mod_write_word(BOSS+0x58,0x80107000u);
        psx_mod_write_byte(0x80107000u+type*2u+1u,2);
        psx_mod_write_byte(shot+0x64,4);
        cpu.gpr[4]=BOSS;cpu.gpr[5]=shot;
        mmx4_coop_balance_begin(BOSS);
        unsigned before_hp=psx_mod_read_byte(BOSS+0x5C);
        CHECK(contact(&cpu,seat)==type+1u);
        CHECK(psx_mod_read_byte(BOSS+0x5C)==before_hp-1u);
        CHECK(psx_mod_read_byte(BOSS+0x65)==4);
        mmx4_coop_balance_end();
        psx_mod_write_byte(shot+0x64,3);psx_mod_write_byte(BOSS+0x65,3);
        /* A non-initializing update must not turn it into a fresh blade. */
        cpu.gpr[4]=shot;psx_mod_write_byte(shot+4,1);hook(&cpu,0x80097EECu);
        CHECK(mmx4_coop_balance_contact_allowed(BOSS,shot,seat));
        CHECK(psx_mod_read_byte(BOSS+0x65)==3);
        io=(Mmx4SnapshotIO){saved,NULL,sizeof saved,0,MMX4_SNAPSHOT_WRITE,1};
        CHECK(mmx4_coop_balance_snapshot(&io));digest=mmx4_coop_balance_digest(1);
        /* Fresh native initialization, even with the same slot/contact key. */
        psx_mod_write_byte(shot+4,0);hook(&cpu,0x80097EECu);
        psx_mod_write_byte(BOSS+0x65,3);
        CHECK(mmx4_coop_balance_contact_allowed(BOSS,shot,seat));
        CHECK(!psx_mod_read_byte(BOSS+0x65));
        CHECK(digest!=mmx4_coop_balance_digest(1));
        io=(Mmx4SnapshotIO){NULL,saved,sizeof saved,0,MMX4_SNAPSHOT_READ,1};
        CHECK(mmx4_coop_balance_snapshot(&io));
        psx_mod_write_byte(BOSS+0x65,3);
        CHECK(mmx4_coop_balance_contact_allowed(BOSS,shot,seat));
        CHECK(psx_mod_read_byte(BOSS+0x65)==3);
        mmx4_coop_balance_forget(BOSS);
        CHECK(mmx4_coop_balance_contact_allowed(BOSS,shot,seat));
        projected=0;
    }
    /* Interleaved native blade slots cannot re-arm one another. */
    actor(57,48);hero=1;shot_type=27;projected=1;
    psx_mod_write_byte(MMX4_PLAYER+2,1);
    for(unsigned index=0;index<2;++index) {
        cpu.gpr[4]=0x801406F8u+index*0x9Cu;
        psx_mod_write_byte(cpu.gpr[4]+1,27);hook(&cpu,0x80097EECu);
    }
    for(unsigned index=0;index<2;++index) {
        shot_address=0x801406F8u+index*0x9Cu;
        projected=1;CHECK(mmx4_coop_balance_contact_allowed(BOSS,shot_address,1));
        CHECK(timed_hit(1,2,64)==1);
        for(unsigned tick=0;tick<4;++tick){mmx4_coop_balance_begin(BOSS);mmx4_coop_balance_end();}
    }
    for(unsigned index=0;index<2;++index) {
        uint32_t old=0x801406F8u+index*0x9Cu;
        psx_mod_write_byte(old+0x64,7);psx_mod_write_byte(BOSS+0x65,7);
        CHECK(mmx4_coop_balance_contact_allowed(BOSS,old,1));
        CHECK(psx_mod_read_byte(BOSS+0x65)==7);
        CHECK(mmx4_coop_call(&cpu,0x8002DE30u,BOSS,old)==0x7Eu);
    }
    shot_address=0x801406F8u;shot_type=255;hero=0;
    /* Odd native durations round up, for either owner. The last blocked
     * tick remains blocked; the partner is available throughout. */
    for(unsigned seat=0;seat<2;++seat) {
        actor(57,48);CHECK(timed_hit(seat,2,5)==1);
        for(unsigned tick=0;tick<2;++tick) {
            mmx4_coop_balance_begin(BOSS);mmx4_coop_balance_end();
            CHECK(!mmx4_coop_balance_hit_allowed(BOSS,seat));
            CHECK(mmx4_coop_balance_hit_allowed(BOSS,seat^1u));
        }
        mmx4_coop_balance_begin(BOSS);mmx4_coop_balance_end();
        CHECK(mmx4_coop_balance_hit_allowed(BOSS,seat));
    }
    /* A fresh partner attack must not clang on the shared recovery table
     * while their private timer is ready. Exercise both attacker orders,
     * the exact min1 window, the post-hit caller, no-hit cleanup, snapshots
     * and an actual invulnerable phase with no hit countdown. */
    for(unsigned first=0;first<2;++first) {
        actor(57,48);damage_table=0x80107A78u;hero=0;shot_type=0;
        psx_mod_write_byte(damage_table+1,1);
        psx_mod_write_byte(damage_table+24*2u+1,2);
        for(unsigned t=0;t<40;++t)psx_mod_write_byte(0x801060F0u+t*2u,255);
        CHECK(timed_hit(first,1,64)==1);
        word(BOSS+0x58,0x801060F0u);psx_mod_write_byte(BOSS+0x8B,1);
        const uint32_t shot=0x801406F8u;
        psx_mod_write_byte(shot+1,24);psx_mod_write_byte(shot+0x64,1);
        projected=(int)(first^1u);psx_mod_write_byte(MMX4_PLAYER+2,1);
        cpu=(CPUState){0};cpu.gpr[4]=cpu.gpr[16]=BOSS;cpu.gpr[5]=shot;
        mmx4_coop_balance_begin(BOSS);native_clangs=0;
        CHECK(contact(&cpu,first^1u)==25);
        CHECK(native_clangs==0 && psx_mod_read_byte(BOSS+0x5C)==47);
        CHECK(psx_mod_read_word(BOSS+0x58)==damage_table);
        CHECK(!psx_mod_read_byte(BOSS+0x8B));
        /* The selected hit reaches the original caller with zero shared
         * countdown so its ordinary reaction/new cooldown is not discarded. */
        word(BOSS+0x58,0x801060F0u);cpu.gpr[2]=64;hook(&cpu,0x8008CEC0u);
        psx_mod_write_byte(BOSS+0x8B,(uint8_t)cpu.gpr[2]);
        CHECK(!mmx4_coop_balance_hit_allowed(BOSS,first));
        CHECK(!mmx4_coop_balance_hit_allowed(BOSS,first^1u));
        mmx4_coop_balance_end();projected=0;
        io=(Mmx4SnapshotIO){saved,NULL,sizeof saved,0,MMX4_SNAPSHOT_WRITE,1};
        CHECK(mmx4_coop_balance_snapshot(&io));digest=mmx4_coop_balance_digest(1);
        mmx4_coop_balance_reset();
        io=(Mmx4SnapshotIO){NULL,saved,sizeof saved,0,MMX4_SNAPSHOT_READ,1};
        CHECK(mmx4_coop_balance_snapshot(&io) && digest==mmx4_coop_balance_digest(1));
        /* After private expiry, an unchanged duplicate restores the native
         * table/countdown for its original cleanup instead of restarting it. */
        for(unsigned tick=0;tick<32;++tick){mmx4_coop_balance_begin(BOSS);mmx4_coop_balance_end();}
        psx_mod_write_byte(BOSS+0x8B,1);word(BOSS+0x58,0x801060F0u);
        mmx4_coop_balance_begin(BOSS);cpu.gpr[4]=BOSS;cpu.gpr[5]=shot;
        CHECK(contact(&cpu,first^1u)==0x7Eu);
        CHECK(psx_mod_read_byte(BOSS+0x8B)==1 && psx_mod_read_word(BOSS+0x58)==0x801060F0u);
        psx_mod_write_byte(BOSS+0x8B,0);psx_mod_write_byte(BOSS+0x65,0);
        CHECK(contact(&cpu,first^1u)==0x7Fu); /* Real phase, not a hit countdown. */
        CHECK(native_clangs==1);
        word(BOSS+0x58,damage_table); /* Original expiry completes. */
        mmx4_coop_balance_end();
        word(BOSS+0x58,0x801060F0u);psx_mod_write_byte(BOSS+0x8B,1);
        mmx4_coop_balance_begin(BOSS);
        CHECK(!mmx4_coop_balance_project_contact(&cpu,0x8002DE30u)); /* Later phase cannot inherit the old window. */
        mmx4_coop_balance_end();mmx4_coop_balance_forget(BOSS);
        psx_mod_write_byte(BOSS+0x8B,1);cpu.gpr[4]=BOSS;
        mmx4_coop_balance_begin(BOSS);
        CHECK(!mmx4_coop_balance_project_contact(&cpu,0x8002DE30u));
        mmx4_coop_balance_end();
    }
    damage_table=0;shot_type=255;hero=0;projected=0;
    per_player=0;mmx4_coop_balance_activate();
    CHECK(!mmx4_coop_balance_flash_ticks(BOSS));
    io=(Mmx4SnapshotIO){NULL,saved,sizeof saved,0,MMX4_SNAPSHOT_VALIDATE,1};
    CHECK(!mmx4_coop_balance_snapshot(&io));
    actor(57,48);cpu.gpr[16]=BOSS;cpu.gpr[2]=5;
    hook(&cpu,0x800725A0u);CHECK(cpu.gpr[2]==3);
    enabled=0;mmx4_coop_balance_activate();
    CHECK(timed_hit(1,2,64)==64); /* Original balance stays native. */
    CHECK(!mmx4_coop_balance_flash_ticks(BOSS));
    enabled=per_player=1;mmx4_coop_balance_activate();actor(57,48);
    uint32_t before_memory=memory_digest();
#if MMX4_COOP_NETPLAY_DEBUG
    io=(Mmx4SnapshotIO){saved,NULL,sizeof saved,0,MMX4_SNAPSHOT_WRITE,1};
    CHECK(mmx4_coop_balance_snapshot(&io));
    CHECK(mmx4_coop_balance_development_mode(0));
    CHECK(memory_digest()==before_memory);
    io=(Mmx4SnapshotIO){NULL,saved,sizeof saved,0,MMX4_SNAPSHOT_VALIDATE,1};
    CHECK(!mmx4_coop_balance_snapshot(&io)); /* No weakened cross-mode restore. */
    CHECK(hit(2,0)==46);
    CHECK(mmx4_coop_balance_development_mode(1));CHECK(hit(2,0)==45);
    io=(Mmx4SnapshotIO){NULL,saved,sizeof saved,0,MMX4_SNAPSHOT_VALIDATE,1};
    CHECK(mmx4_coop_balance_snapshot(&io));
    CHECK(!mmx4_coop_balance_development_mode(2));
    local=1;CHECK(!mmx4_coop_balance_development_mode(0));local=0;
    projected=1;CHECK(!mmx4_coop_balance_development_mode(0));projected=0;
    ready=0;CHECK(!mmx4_coop_balance_development_mode(0));ready=1;
    mmx4_coop_balance_begin(BOSS);
    CHECK(!mmx4_coop_balance_development_mode(0));mmx4_coop_balance_end();
    enabled=0;CHECK(!mmx4_coop_balance_development_mode(1));
    enabled=1;per_player=0;mmx4_coop_balance_activate();
    CHECK(!mmx4_coop_balance_development_mode(0));
#else
    CHECK(!mmx4_coop_balance_development_mode(0));
    CHECK(memory_digest()==before_memory);CHECK(hit(2,0)==47);
#endif
    puts("Boss scaling preserves weak hits, separate pools, rejected hits and snapshots.");return 0;
}

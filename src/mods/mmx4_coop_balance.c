/* Optional double effective boss HP. Keep native HP units so fill loops,
 * phase tables, split health pools and the HUD retain their original ranges.
 * Carry half a point between accepted hits: two 1-damage hits cost one native
 * point, rather than rounding every small hit down or granting immunity. */
#include "mmx4_coop_internal.h"
#include <string.h>

#define ID "mmx4.coop"
#define POOL 0x8013BED0u
#define STRIDE 0x9Cu
#define SLOTS 48u
static unsigned enabled;
static unsigned per_player;
static uint8_t fraction[SLOTS][3];
static uint8_t immunity[SLOTS][3][2];
static uint32_t current_actor;
static uint8_t saved_fraction[3];
static unsigned accepted_damage;
static unsigned damage_owner;
static unsigned saber_chain;

static int slot(uint32_t actor) {
    if(actor<POOL || actor>=POOL+SLOTS*STRIDE || (actor-POOL)%STRIDE)return -1;
    return (int)((actor-POOL)/STRIDE);
}
static int boss(uint32_t actor) {
    if(slot(actor)<0 || !psx_mod_read_byte(actor))return 0;
    unsigned kind=psx_mod_read_byte(actor+1);
    /* Iris's armor absorbs hits to expose its separate vulnerable core. */
    if(kind==66)return psx_mod_read_byte(actor+2)!=0;
    if(psx_mod_read_word(MMX4_PLAY+0x20)==actor && psx_mod_read_byte(MMX4_PLAY+0x24))return 1;
    /* Native bosses can hide their meter during attack/phase transitions.
     * Sigma's final form switches two health pools within one actor. */
    switch(kind) {
    case 8:case 18:case 33:case 43:case 45:case 46:case 47:
    case 54:case 56:case 57:case 60:case 61:
    case 64:case 65:case 68:case 69:case 73:case 74:case 75:return 1;
    default:return 0;
    }
}
static unsigned bank(uint32_t actor) {
    if(psx_mod_read_byte(actor+1)!=74)return 0;
    return psx_mod_read_byte(actor+0x91)==1?1u:2u;
}
void mmx4_coop_balance_reset(void) {
    memset(fraction,0,sizeof fraction);memset(saved_fraction,0,sizeof saved_fraction);
    memset(immunity,0,sizeof immunity);
    current_actor=0;accepted_damage=damage_owner=saber_chain=0;
}
void mmx4_coop_balance_activate(void) {
    char value[32];
    enabled=psx_mod_current_option_value("boss_scaling",value,sizeof value) && !strcmp(value,"true");
    per_player=!(psx_mod_current_option_value("boss_cooldowns",value,sizeof value) && !strcmp(value,"shared"));
    mmx4_coop_balance_reset();
    if(enabled)psx_mod_counter_add("mmx4.coop.boss-scaling-enabled",1);
}
void mmx4_coop_balance_forget(uint32_t actor) {
    int i=slot(actor);if(i>=0) {
        memset(fraction[i],0,sizeof fraction[i]);
        memset(immunity[i],0,sizeof immunity[i]);
    }
}
int mmx4_coop_balance_hit_allowed(uint32_t actor,unsigned seat) {
    if(!enabled || !per_player || psx_mod_local_view_scope() || seat>1 || !boss(actor))return 1;
    if(!immunity[slot(actor)][bank(actor)][seat])return 1;
    psx_mod_counter_add("mmx4.coop.boss-repeat-hit-blocked",1);
    return 0;
}
void mmx4_coop_balance_begin(uint32_t actor) {
    if(!enabled || psx_mod_local_view_scope() || slot(actor)<0)return;
    if(!psx_mod_read_byte(actor+4))mmx4_coop_balance_forget(actor);
    current_actor=actor;
    accepted_damage=damage_owner=saber_chain=0;
    if(per_player)for(unsigned b=0;b<3;++b)for(unsigned seat=0;seat<2;++seat)
        if(immunity[slot(actor)][b][seat])--immunity[slot(actor)][b][seat];
    memcpy(saved_fraction,fraction[slot(actor)],sizeof saved_fraction);
}
void mmx4_coop_balance_end(void) {
    if(current_actor && !psx_mod_read_byte(current_actor+0x5C))
        fraction[slot(current_actor)][bank(current_actor)]=0;
    current_actor=0;accepted_damage=damage_owner=saber_chain=0;memset(saved_fraction,0,sizeof saved_fraction);
}
int mmx4_coop_balance_snapshot_ready(void) {return !current_actor;}
int mmx4_coop_balance_snapshot(Mmx4SnapshotIO *s) {
    mmx4_snapshot_fixed(s,enabled);
    mmx4_snapshot_fixed(s,per_player);
    for(unsigned i=0;i<SLOTS;++i)for(unsigned b=0;b<3;++b)MMX4_SNAPSHOT_BOOL(s,fraction[i][b]);
    for(unsigned i=0;i<SLOTS;++i)for(unsigned b=0;b<3;++b)for(unsigned seat=0;seat<2;++seat)
        MMX4_SNAPSHOT_U8(s,immunity[i][b][seat]);
    return s->ok;
}
uint32_t mmx4_coop_balance_digest(uint32_t seed) {
    seed=(seed^enabled)*16777619u;
    seed=(seed^per_player)*16777619u;
    for(unsigned i=0;i<SLOTS;++i)for(unsigned b=0;b<3;++b)seed=(seed^fraction[i][b])*16777619u;
    for(unsigned i=0;i<SLOTS;++i)for(unsigned b=0;b<3;++b)for(unsigned seat=0;seat<2;++seat)
        seed=(seed^immunity[i][b][seat])*16777619u;
    return seed;
}
static uint32_t scaled_damage(uint32_t actor,unsigned amount) {
    unsigned hp=psx_mod_read_byte(actor+0x5C);
    unsigned b=bank(actor);uint8_t *half=&fraction[slot(actor)][b];
    unsigned total=amount+*half;
    *half=(uint8_t)(total&1u);
    if(current_actor==actor) {
        accepted_damage+=amount;
        damage_owner=(unsigned)(mmx4_coop_projected()!=0)+1u;
        if(per_player && immunity[slot(actor)][b][(damage_owner-1u)^1u])
            psx_mod_counter_add("mmx4.coop.boss-hit-during-partner-cooldown",1);
    }
    psx_mod_counter_add("mmx4.coop.scaled-boss-hit",1);
    psx_mod_counter_add("mmx4.coop.boss-damage-before-scaling",amount);
    psx_mod_counter_add("mmx4.coop.boss-damage-after-scaling",total/2);
    return hp-total/2;
}
static void damage(CPUState *cpu,uint32_t address) {
    (void)address;
    uint32_t actor=cpu->gpr[4];
    if(!enabled || !mmx4_coop_ready() || psx_mod_local_view_scope() || !boss(actor))return;
    unsigned hp=psx_mod_read_byte(actor+0x5C),amount=cpu->gpr[2];
    if(!hp || hp>=127 || !amount || amount>255)return;
    /* This site follows SUBU, after the separate damage-multiplier hook. */
    cpu->gpr[3]=scaled_damage(actor,amount);
    /* Keep Zero's first two grounded strikes chainable in the independent
     * cooldown mode. The finisher and other moves retain their full timer.
     * A1 is the native projectile accepted by 2DE30, after overlap/tokens. */
    uint32_t shot=cpu->gpr[5];
    if(current_actor==actor) {
        saber_chain=shot>=0x801406F8u && shot<0x801406F8u+16u*0x9Cu &&
            !((shot-0x801406F8u)%0x9Cu) && psx_mod_read_byte(MMX4_PLAYER+2)==1 &&
            (psx_mod_read_byte(shot+1)==24 || psx_mod_read_byte(shot+1)==25);
    }
}
static void cut_web(CPUState *cpu,uint32_t address) {
    (void)address;
    uint32_t actor=cpu->gpr[16];
    /* Web Spider also loses four HP when his supporting web is cut. This
     * authored reaction bypasses ENEMY_HIT and must use the same health scale. */
    if(enabled && mmx4_coop_ready() && !psx_mod_local_view_scope() &&
       boss(actor) && psx_mod_read_byte(actor+1)==43 &&
       psx_mod_read_byte(actor+0x5C)>0 && psx_mod_read_byte(actor+0x5C)<127)
        cpu->gpr[2]=scaled_damage(actor,4);
}
static void rejected_hit(CPUState *cpu,uint32_t address) {
    (void)address;
    uint32_t actor=cpu->gpr[16];
    /* Cyber Peacock restores his pre-hit HP while his native cooldown runs.
     * A discarded hit must also discard its fractional damage. */
    if(enabled && current_actor==actor && !psx_mod_local_view_scope()) {
        memcpy(fraction[slot(actor)],saved_fraction,sizeof saved_fraction);
        accepted_damage=damage_owner=saber_chain=0;
    }
}
static void damage_reaction(CPUState *cpu,uint32_t address) {
    (void)address;
    /* This native strong-hit reaction compares the HP lost in this
     * update. Preserve that attack strength when displayed HP uses halves. */
    if(enabled && current_actor==cpu->gpr[17] && boss(current_actor) &&
       !psx_mod_local_view_scope() && accepted_damage)cpu->gpr[16]=accepted_damage;
}
static void cooldown(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[address==0x80080C98u?17:16];
    if(!enabled || !mmx4_coop_ready() || psx_mod_local_view_scope() || !boss(actor))return;
    unsigned ticks=cpu->gpr[2];
    if(ticks>1 && ticks<255) {
        if(per_player) {
            if(current_actor!=actor || !damage_owner)return;
            if(saber_chain && ticks>4) {
                ticks=4;
                psx_mod_counter_add("mmx4.coop.zero-combo-cooldown",1);
            }
            uint8_t *timers=immunity[slot(actor)][bank(actor)];
            timers[damage_owner-1u]=(uint8_t)ticks;
            /* Native collision/flash gates see the earliest available seat.
             * The filter above retains the full duration for the attacker.
             * Reactions and AI still execute once in the original caller. */
            unsigned next=timers[0]<timers[1]?timers[0]:timers[1];
            /* Spider installs a temporary hit table and restores it on the
             * timer's 1->0 transition. Zero would skip that cleanup forever.
             * Retain one native expiry tick even when the partner is ready. */
            cpu->gpr[2]=next?next:1u;
            psx_mod_counter_add("mmx4.coop.per-player-boss-cooldown",1);
        }else {
            cpu->gpr[2]=(ticks+1)/2;
            psx_mod_counter_add("mmx4.coop.shorter-boss-cooldown",1);
        }
    }
}

PSX_MOD_CONSTRUCTOR(mmx4_register_coop_balance) {
    /* Exact SLUS-00561 post-hit stores. Never shorten general AI/door timers
     * or rerun a boss update: native weakness reactions still execute once. */
    psx_mod_register_instruction_plugin(ID,0x8002DF08u,0x94820042u,damage);
    psx_mod_register_instruction_plugin(ID,0x8007D8CCu,0xA203005Cu,rejected_hit);
    psx_mod_register_instruction_plugin(ID,0x80088EF8u,0x2A100004u,damage_reaction);
    psx_mod_register_instruction_plugin(ID,0x80064CD4u,0xA202005Cu,cut_web);
    psx_mod_register_instruction_plugin(ID,0x80063AB4u,0xA602008Au,cooldown);
    psx_mod_register_instruction_plugin(ID,0x80047F8Cu,0xA2020061u,cooldown);
    psx_mod_register_instruction_plugin(ID,0x80047FACu,0xA2020061u,cooldown);
    psx_mod_register_instruction_plugin(ID,0x800509A8u,0xA2020061u,cooldown);
    psx_mod_register_instruction_plugin(ID,0x8006BE60u,0xA2020080u,cooldown);
    psx_mod_register_instruction_plugin(ID,0x80071E98u,0xA202008Au,cooldown);
    psx_mod_register_instruction_plugin(ID,0x800725A0u,0xA2020094u,cooldown);
    psx_mod_register_instruction_plugin(ID,0x800754CCu,0xA2020093u,cooldown);
    psx_mod_register_instruction_plugin(ID,0x800754F8u,0xA2020093u,cooldown);
    psx_mod_register_instruction_plugin(ID,0x80075520u,0xA2020093u,cooldown);
    psx_mod_register_instruction_plugin(ID,0x80078B34u,0xA2020084u,cooldown);
    psx_mod_register_instruction_plugin(ID,0x80078B48u,0xA2020084u,cooldown);
    psx_mod_register_instruction_plugin(ID,0x8007D964u,0xA2020092u,cooldown);
    psx_mod_register_instruction_plugin(ID,0x8007D970u,0xA2020092u,cooldown);
    psx_mod_register_instruction_plugin(ID,0x8007FDA0u,0xA202008Au,cooldown);
    psx_mod_register_instruction_plugin(ID,0x80080C98u,0xA2220088u,cooldown);
    psx_mod_register_instruction_plugin(ID,0x80084FBCu,0xA2020091u,cooldown);
    psx_mod_register_instruction_plugin(ID,0x80085194u,0xA2020091u,cooldown);
    psx_mod_register_instruction_plugin(ID,0x80086240u,0xA202008Du,cooldown);
    psx_mod_register_instruction_plugin(ID,0x8008CEC0u,0xA202008Bu,cooldown);
    psx_mod_register_instruction_plugin(ID,0x8008CEC8u,0xA202008Bu,cooldown);
    psx_mod_register_instruction_plugin(ID,0x8008F338u,0xA022B8A4u,cooldown);
    psx_mod_register_instruction_plugin(ID,0x800913ECu,0xA202008Fu,cooldown);
}

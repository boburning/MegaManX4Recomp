/* Optional double effective boss HP. Keep native HP units so fill loops,
 * phase tables, split health pools and the HUD retain their original ranges.
 * Carry fractional points between accepted hits, including Zero's additional
 * 20% reduction, rather than rounding small hits down or granting immunity. */
#include "mmx4_coop_internal.h"
#include <string.h>
#if MMX4_COOP_NETPLAY_DEBUG
#include "psx_cycles.h"
#include <stdio.h>
#include <stdlib.h>
#endif

#define ID "mmx4.coop"
#define POOL 0x8013BED0u
#define STRIDE 0x9Cu
#define SLOTS 48u
static unsigned enabled;
static unsigned per_player;
static uint8_t fraction[SLOTS][3];
static uint8_t immunity[SLOTS][3][2];
/* Native boss callers install a shared rejection table during recovery.
 * Private timers must virtualize that table too: otherwise a ready partner
 * can clang on 801060F0 and consume/stamp an attack before native expiry. */
static uint32_t vulnerable_table[SLOTS][3],saved_vulnerable_table[3];
static uint8_t cooldown_offset[SLOTS][3],cooldown_half[SLOTS][3];
static unsigned contact_call;
/* A sword's pose/contact byte is reusable, and changes within Kuuenzan.
 * Track native initialization so a NEW blade may reuse the preceding key.
 * An existing blade retains the native duplicate/pose checks; it must not
 * be rejected for its entire lifetime after the private timer expires. */
static uint32_t melee_generation[2][16],next_melee[2];
static uint32_t melee_hits[SLOTS][3][2],saved_melee_hits[3][2];
static uint16_t melee_used[SLOTS][3][2],saved_melee_used[3][2];
static uint32_t current_actor;
static uint8_t saved_fraction[3];
static unsigned accepted_damage;
static unsigned damage_owner;
static unsigned saber_chain;

static int melee_slot(uint32_t shot) {
    if(shot<0x801406F8u || shot>=0x801406F8u+16u*0x9Cu ||
       (shot-0x801406F8u)%0x9Cu || psx_mod_read_byte(MMX4_PLAYER+2)!=1)return -1;
    unsigned type=psx_mod_read_byte(shot+1);
    return (type>=24 && type<=28) || type==34?(int)((shot-0x801406F8u)/0x9Cu):-1;
}
static void melee_spawned(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!enabled || !per_player || !mmx4_coop_ready() || psx_mod_local_view_scope() ||
       psx_mod_read_byte(cpu->gpr[4]+4))return;
    int i=melee_slot(cpu->gpr[4]);if(i<0)return;
    unsigned seat=(unsigned)(mmx4_coop_projected()!=0);
    if(!++next_melee[seat])++next_melee[seat];
    melee_generation[seat][i]=next_melee[seat];
    /* More than one native blade can coexist. A later hit from another
     * slot must not make this older, already accepted blade eligible again. */
    for(unsigned target=0;target<SLOTS;++target)for(unsigned b=0;b<3;++b)
        melee_used[target][b][seat]&=(uint16_t)~(1u<<(unsigned)i);
    psx_mod_counter_add("mmx4.coop.boss-melee-lifetimes",1);
}

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
static void trace_contact(const char *reason,uint32_t actor,uint32_t shot,unsigned seat,unsigned amount) {
#if MMX4_COOP_NETPLAY_DEBUG
    static int tracing=-1;
    if(tracing<0) {
        const char *value=getenv("MMX4_COOP_HIT_TRACE");tracing=value && value[0]=='1';
    }
    if(!tracing || psx_mod_local_view_scope() || seat>1 || !boss(actor))return;
    unsigned i=(unsigned)slot(actor),b=bank(actor);
    int blade=melee_slot(shot);
    fprintf(stderr,"mmx4.hit cycle=%llu reason=%s scaling=%u separate=%u seat=%u "
        "boss=%08X kind=%u state=%u/%u/%u hp=%u table=%08X private=%u/%u native_offset=%u native_timer=%u "
        "shot=%08X type=%u state=%u/%u key=%u/%u lifetime=%u used=%u raw=%u "
        "boss_xy=%d/%d shot_xy=%d/%d\n",
        (unsigned long long)psx_cycle_count,reason,enabled,per_player,seat,
        actor,psx_mod_read_byte(actor+1),psx_mod_read_byte(actor+4),
        psx_mod_read_byte(actor+5),psx_mod_read_byte(actor+6),psx_mod_read_byte(actor+0x5C),
        psx_mod_read_word(actor+0x58),immunity[i][b][0],immunity[i][b][1],cooldown_offset[i][b],
        !cooldown_offset[i][b]?0u:cooldown_half[i][b]?
            psx_mod_read_half(actor+cooldown_offset[i][b]):psx_mod_read_byte(actor+cooldown_offset[i][b]),
        shot,shot?psx_mod_read_byte(shot+1):255u,shot?psx_mod_read_byte(shot+4):0u,
        shot?psx_mod_read_byte(shot+5):0u,psx_mod_read_byte(actor+0x65),
        shot?psx_mod_read_byte(shot+0x64):0u,blade<0?0u:melee_generation[seat][blade],
        blade<0?0u:(unsigned)((melee_used[i][b][seat]>>blade)&1u),amount,
        (int16_t)psx_mod_read_half(actor+10),(int16_t)psx_mod_read_half(actor+14),
        shot?(int16_t)psx_mod_read_half(shot+10):0,shot?(int16_t)psx_mod_read_half(shot+14):0);
#else
    (void)reason;(void)actor;(void)shot;(void)seat;(void)amount;
#endif
}
void mmx4_coop_balance_reset(void) {
    memset(fraction,0,sizeof fraction);memset(saved_fraction,0,sizeof saved_fraction);
    memset(immunity,0,sizeof immunity);
    memset(vulnerable_table,0,sizeof vulnerable_table);
    memset(saved_vulnerable_table,0,sizeof saved_vulnerable_table);
    memset(cooldown_offset,0,sizeof cooldown_offset);
    memset(cooldown_half,0,sizeof cooldown_half);contact_call=0;
    memset(melee_generation,0,sizeof melee_generation);memset(next_melee,0,sizeof next_melee);
    memset(melee_hits,0,sizeof melee_hits);memset(saved_melee_hits,0,sizeof saved_melee_hits);
    memset(melee_used,0,sizeof melee_used);memset(saved_melee_used,0,sizeof saved_melee_used);
    current_actor=0;accepted_damage=damage_owner=saber_chain=0;
}
void mmx4_coop_balance_activate(void) {
    char value[32];
    enabled=psx_mod_current_option_value("boss_scaling",value,sizeof value) && !strcmp(value,"true");
    per_player=!(psx_mod_current_option_value("boss_cooldowns",value,sizeof value) && !strcmp(value,"shared"));
    mmx4_coop_balance_reset();
    if(enabled)psx_mod_counter_add("mmx4.coop.boss-scaling-enabled",1);
}
int mmx4_coop_balance_development_mode(unsigned mode) {
#if MMX4_COOP_NETPLAY_DEBUG
    char value[16];
    if(mode>1 || !per_player || !mmx4_coop_ready() || mmx4_coop_projected() ||
       psx_mod_local_view_scope() || current_actor || contact_call ||
       !psx_mod_current_option_value("boss_scaling",value,sizeof value) || strcmp(value,"true"))return 0;
    /* Use only to prepare a control before combat. Returning to the saved
     * mode before loading retains the existing strict snapshot validation.
     * Both private counters and this existing mode enter the mod digest. */
    enabled=mode;mmx4_coop_balance_reset();
    psx_mod_counter_add(mode?"mmx4.coop.development-scaling-on":
                            "mmx4.coop.development-scaling-off",1);
    return 1;
#else
    (void)mode;return 0;
#endif
}
void mmx4_coop_balance_forget(uint32_t actor) {
    int i=slot(actor);if(i>=0) {
        memset(fraction[i],0,sizeof fraction[i]);
        memset(immunity[i],0,sizeof immunity[i]);
        memset(vulnerable_table[i],0,sizeof vulnerable_table[i]);
        memset(cooldown_offset[i],0,sizeof cooldown_offset[i]);
        memset(cooldown_half[i],0,sizeof cooldown_half[i]);
        memset(melee_hits[i],0,sizeof melee_hits[i]);
        memset(melee_used[i],0,sizeof melee_used[i]);
    }
}
int mmx4_coop_balance_hit_allowed(uint32_t actor,unsigned seat) {
    if(!enabled || !per_player || psx_mod_local_view_scope() || seat>1 || !boss(actor))return 1;
    if(!immunity[slot(actor)][bank(actor)][seat])return 1;
    psx_mod_counter_add("mmx4.coop.boss-repeat-hit-blocked",1);
    return 0;
}
int mmx4_coop_balance_contact_allowed(uint32_t actor,uint32_t shot,unsigned seat) {
    if(!mmx4_coop_balance_hit_allowed(actor,seat)) {
        trace_contact("private-timer",actor,shot,seat,0);return 0;
    }
    if(!enabled || !per_player || seat>1 || psx_mod_local_view_scope() || !boss(actor)) {
        trace_contact("native-policy",actor,shot,seat,0);return 1;
    }
    int i=melee_slot(shot);if(i<0) {trace_contact("admit-projectile",actor,shot,seat,0);return 1;}
    uint32_t generation=melee_generation[seat][i];
    uint32_t previous=melee_hits[slot(actor)][bank(actor)][seat];
    if(!generation) {trace_contact("untracked-blade",actor,shot,seat,0);return 1;}
    if(melee_used[slot(actor)][bank(actor)][seat]&(1u<<(unsigned)i)) {
        /* No duplicate-key reset for an already accepted blade. Retail
         * DE30 rejects the same contact key, but admits a new native pose
         * after recovery. A lifetime-wide veto incorrectly cancels these
         * later Kuuenzan contacts despite private/native timers being zero. */
        trace_contact("native-blade-contact",actor,shot,seat,0);return 1;
    }
    /* A fresh sword may reuse the old contact key without an empty overlap
     * frame (cancelled ground slashes). Only a new native blade lifetime may
     * clear that duplicate key; an old blade keeps its native token above. */
    if(previous && psx_mod_read_byte(shot+0x64) &&
       psx_mod_read_byte(actor+0x65)==psx_mod_read_byte(shot+0x64))
        psx_mod_write_byte(actor+0x65,0);
    trace_contact("admit",actor,shot,seat,0);return 1;
}
int mmx4_coop_balance_project_contact(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(address!=0x8002DE30u || contact_call || !enabled || !per_player || current_actor!=actor ||
       !mmx4_coop_ready() || psx_mod_local_view_scope() || !boss(actor))return 0;
    unsigned i=(unsigned)slot(actor),b=bank(actor),offset=cooldown_offset[i][b];
    uint32_t table=vulnerable_table[i][b];
    /* Only the inspected post-hit rejection table and its live countdown
     * qualify. A scene/attack phase that installs another table, or one
     * without an active native hit timer, retains its native invulnerability. */
    if(!offset || !table || psx_mod_read_word(actor+0x58)!=0x801060F0u)return 0;
    unsigned ticks=cooldown_half[i][b]?psx_mod_read_half(actor+offset):
        psx_mod_read_byte(actor+offset);
    /* Separate mode deliberately leaves this one native expiry tick when
     * a private seat is ready. Longer timers are not this proven boundary. */
    if(ticks!=1u)return 0;
    unsigned before=accepted_damage;
    contact_call=1;
    psx_mod_write_word(actor+0x58,table);
    if(cooldown_half[i][b])psx_mod_write_half(actor+offset,0);
    else psx_mod_write_byte(actor+offset,0);
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    if(accepted_damage==before) {
        /* No hit: let the original caller perform its one expiry/cleanup
         * pass. A duplicate contact must not reset the shared recovery. */
        psx_mod_write_word(actor+0x58,0x801060F0u);
        if(cooldown_half[i][b])psx_mod_write_half(actor+offset,(uint16_t)ticks);
        else psx_mod_write_byte(actor+offset,(uint8_t)ticks);
    }else psx_mod_counter_add("mmx4.coop.boss-ready-partner-table",1);
    contact_call=0;
    return mmx4_coop_finish(cpu,result);
}
unsigned mmx4_coop_balance_flash_ticks(uint32_t actor) {
    /* Rendering can query either local view without advancing simulation.
     * Native collision cleanup uses the earliest ready seat; show the longer
     * private recovery so a still-immune attacker sees the original flash. */
    if(!enabled || !per_player || !mmx4_coop_ready() || !boss(actor) ||
       psx_mod_read_byte(actor+4)!=1 || !psx_mod_read_byte(actor+0x5C))return 0;
    const uint8_t *timers=immunity[slot(actor)][bank(actor)];
    return timers[0]>timers[1]?timers[0]:timers[1];
}
void mmx4_coop_balance_begin(uint32_t actor) {
    if(!enabled || psx_mod_local_view_scope() || slot(actor)<0)return;
    if(!psx_mod_read_byte(actor+4))mmx4_coop_balance_forget(actor);
    current_actor=actor;
    accepted_damage=damage_owner=saber_chain=0;
    if(per_player)for(unsigned b=0;b<3;++b)for(unsigned seat=0;seat<2;++seat)
        if(immunity[slot(actor)][b][seat])--immunity[slot(actor)][b][seat];
    memcpy(saved_fraction,fraction[slot(actor)],sizeof saved_fraction);
    memcpy(saved_vulnerable_table,vulnerable_table[slot(actor)],sizeof saved_vulnerable_table);
    memcpy(saved_melee_hits,melee_hits[slot(actor)],sizeof saved_melee_hits);
    memcpy(saved_melee_used,melee_used[slot(actor)],sizeof saved_melee_used);
}
void mmx4_coop_balance_end(void) {
    int index=slot(current_actor);
    if(index>=0) {
        unsigned i=(unsigned)index,b=bank(current_actor),off=cooldown_offset[i][b];
        if(!psx_mod_read_byte(current_actor+0x5C))fraction[i][b]=0;
        if(off && (psx_mod_read_word(current_actor+0x58)!=0x801060F0u ||
           !(cooldown_half[i][b]?psx_mod_read_half(current_actor+off):
             psx_mod_read_byte(current_actor+off)))) {
            /* Do not carry a finished hit window into a later scripted
             * invulnerable phase that happens to reuse the same table. */
            cooldown_offset[i][b]=cooldown_half[i][b]=0;
        }
    }
    current_actor=0;accepted_damage=damage_owner=saber_chain=0;memset(saved_fraction,0,sizeof saved_fraction);
    memset(saved_melee_hits,0,sizeof saved_melee_hits);
    memset(saved_melee_used,0,sizeof saved_melee_used);
    memset(saved_vulnerable_table,0,sizeof saved_vulnerable_table);
}
int mmx4_coop_balance_snapshot_ready(void) {return !current_actor && !contact_call;}
int mmx4_coop_balance_snapshot(Mmx4SnapshotIO *s) {
    mmx4_snapshot_fixed(s,enabled);
    mmx4_snapshot_fixed(s,per_player);
    for(unsigned i=0;i<SLOTS;++i)for(unsigned b=0;b<3;++b)MMX4_SNAPSHOT_FIELD(s,fraction[i][b],1,9);
    for(unsigned i=0;i<SLOTS;++i)for(unsigned b=0;b<3;++b)for(unsigned seat=0;seat<2;++seat)
        MMX4_SNAPSHOT_U8(s,immunity[i][b][seat]);
    for(unsigned seat=0;seat<2;++seat) {
        MMX4_SNAPSHOT_U32(s,next_melee[seat]);
        for(unsigned i=0;i<16;++i)MMX4_SNAPSHOT_U32(s,melee_generation[seat][i]);
    }
    for(unsigned i=0;i<SLOTS;++i)for(unsigned b=0;b<3;++b)for(unsigned seat=0;seat<2;++seat)
        MMX4_SNAPSHOT_U32(s,melee_hits[i][b][seat]);
    for(unsigned i=0;i<SLOTS;++i)for(unsigned b=0;b<3;++b)for(unsigned seat=0;seat<2;++seat)
        MMX4_SNAPSHOT_U16(s,melee_used[i][b][seat]);
    for(unsigned i=0;i<SLOTS;++i)for(unsigned b=0;b<3;++b) {
        MMX4_SNAPSHOT_U32(s,vulnerable_table[i][b]);
        MMX4_SNAPSHOT_U8(s,cooldown_offset[i][b]);
        MMX4_SNAPSHOT_BOOL(s,cooldown_half[i][b]);
        uint32_t table=vulnerable_table[i][b];unsigned offset=cooldown_offset[i][b];
        if((table && (table<0x80010000u || table>=0x80200000u || (table&1u))) ||
           (offset && (offset<0x60u || offset+(cooldown_half[i][b]?2u:1u)>STRIDE)))s->ok=0;
    }
    return s->ok;
}
uint32_t mmx4_coop_balance_digest(uint32_t seed) {
    seed=(seed^enabled)*16777619u;
    seed=(seed^per_player)*16777619u;
    for(unsigned i=0;i<SLOTS;++i)for(unsigned b=0;b<3;++b)seed=(seed^fraction[i][b])*16777619u;
    for(unsigned i=0;i<SLOTS;++i)for(unsigned b=0;b<3;++b)for(unsigned seat=0;seat<2;++seat)
        seed=(seed^immunity[i][b][seat])*16777619u;
    for(unsigned seat=0;seat<2;++seat) {
        seed=(seed^next_melee[seat])*16777619u;
        for(unsigned i=0;i<16;++i)seed=(seed^melee_generation[seat][i])*16777619u;
    }
    for(unsigned i=0;i<SLOTS;++i)for(unsigned b=0;b<3;++b)for(unsigned seat=0;seat<2;++seat)
        seed=(seed^melee_hits[i][b][seat])*16777619u;
    for(unsigned i=0;i<SLOTS;++i)for(unsigned b=0;b<3;++b)for(unsigned seat=0;seat<2;++seat)
        seed=(seed^melee_used[i][b][seat])*16777619u;
    for(unsigned i=0;i<SLOTS;++i)for(unsigned b=0;b<3;++b) {
        seed=(seed^vulnerable_table[i][b])*16777619u;
        seed=(seed^cooldown_offset[i][b])*16777619u;
        seed=(seed^cooldown_half[i][b])*16777619u;
    }
    return seed;
}
static uint32_t scaled_damage(uint32_t actor,unsigned amount,unsigned zero_attack) {
    unsigned hp=psx_mod_read_byte(actor+0x5C);
    unsigned b=bank(actor);uint8_t *remainder=&fraction[slot(actor)][b];
    /* Tenths of native HP: X retains the existing 1/2 health scale. Zero
     * deals 80% of that damage (4/10), independently of seat/campaign/view.
     * Keep the native attack strength for weakness and reaction decisions.
     * A common carry prevents rounding loss when the two heroes alternate. */
    unsigned weight=zero_attack?4u:5u;
    unsigned total=amount*weight+*remainder;
    *remainder=(uint8_t)(total%10u);
    if(current_actor==actor) {
        accepted_damage+=amount;
        damage_owner=(unsigned)(mmx4_coop_projected()!=0)+1u;
        if(per_player && immunity[slot(actor)][b][(damage_owner-1u)^1u])
            psx_mod_counter_add("mmx4.coop.boss-hit-during-partner-cooldown",1);
    }
    psx_mod_counter_add("mmx4.coop.scaled-boss-hit",1);
    psx_mod_counter_add("mmx4.coop.boss-damage-before-scaling",amount);
    if(zero_attack)psx_mod_counter_add("mmx4.coop.zero-boss-damage-before-scaling",amount);
    psx_mod_counter_add("mmx4.coop.boss-damage-after-scaling",total/10);
    return hp-total/10;
}
static void damage(CPUState *cpu,uint32_t address) {
    (void)address;
    uint32_t actor=cpu->gpr[4];
#if MMX4_COOP_NETPLAY_DEBUG
    if(mmx4_coop_ready())trace_contact("native-damage",actor,cpu->gpr[5],
        (unsigned)(mmx4_coop_projected()!=0),cpu->gpr[2]);
#endif
    if(!enabled || !mmx4_coop_ready() || psx_mod_local_view_scope() || !boss(actor))return;
    unsigned hp=psx_mod_read_byte(actor+0x5C),amount=cpu->gpr[2];
    if(!hp || hp>=127 || !amount || amount>255)return;
    /* This site follows SUBU, after the separate damage-multiplier hook. */
    cpu->gpr[3]=scaled_damage(actor,amount,psx_mod_read_byte(MMX4_PLAYER+2)==1);
    /* DE30's A3 is the actual accepted damage table, before the boss caller
     * replaces it with its temporary rejection table. Never cache that
     * rejection table itself, or an uninitialized/synthetic pointer. */
    uint32_t table=cpu->gpr[7];
    if(current_actor==actor && table>=0x80010000u && table<0x80200000u &&
       !(table&1u) && table!=0x801060F0u)
        vulnerable_table[slot(actor)][bank(actor)]=table;
    /* Ground chains, finishers and aerials use one short recovery when a
     * native lifetime is known. Keep native pose/contact admission after
     * that recovery; initialization tracking only fixes reused fresh keys. */
    uint32_t shot=cpu->gpr[5];
    if(current_actor==actor) {
        int i=melee_slot(shot);
        saber_chain=i>=0 && (melee_generation[damage_owner-1u][i] ||
            psx_mod_read_byte(shot+1)==24 || psx_mod_read_byte(shot+1)==25);
        if(per_player && i>=0) {
            melee_hits[slot(actor)][bank(actor)][damage_owner-1u]=melee_generation[damage_owner-1u][i];
            melee_used[slot(actor)][bank(actor)][damage_owner-1u]|=(uint16_t)(1u<<(unsigned)i);
        }
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
        /* This is a delayed web-severing event, not a hit by the current
         * AI target. Keep its environmental penalty independent of which
         * hero happens to be projected when Spider consumes that signal. */
        cpu->gpr[2]=scaled_damage(actor,4,0);
}
static void rejected_hit(CPUState *cpu,uint32_t address) {
    (void)address;
    uint32_t actor=cpu->gpr[16];
    int i=slot(actor);if(i<0)return;
    /* Cyber Peacock restores his pre-hit HP while his native cooldown runs.
     * A discarded hit must also discard its fractional damage. */
    if(enabled && current_actor==actor && !psx_mod_local_view_scope()) {
        memcpy(fraction[i],saved_fraction,sizeof saved_fraction);
        memcpy(melee_hits[i],saved_melee_hits,sizeof saved_melee_hits);
        memcpy(melee_used[i],saved_melee_used,sizeof saved_melee_used);
        memcpy(vulnerable_table[i],saved_vulnerable_table,sizeof saved_vulnerable_table);
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
    int index=slot(actor);
    if(!enabled || index<0 || !mmx4_coop_ready() || psx_mod_local_view_scope() || !boss(actor))return;
    /* Retail BE64..BEA8 restarts Slash's weakness animation for results6/15.
     * Halved recovery admits another hit before phase10 finishes, allowing
     * an endless restart. HP, attack identity and the native damage table
     * have already been applied. For this reaction branch only, use its
     * ordinary-hit result so the running stun can finish. A fresh weakness
     * still starts normally, with either seat and either scaling policy. */
    if(address==0x8006BE60u && current_actor==actor && accepted_damage &&
       damage_owner && psx_mod_read_byte(actor+1)==54 &&
       psx_mod_read_byte(actor+4)==1 && psx_mod_read_byte(actor+5)==10 &&
       (cpu->gpr[4]==6 || cpu->gpr[4]==15)) {
        cpu->gpr[4]=1;
        psx_mod_counter_add("mmx4.coop.slash-stun-restarts-suppressed",1);
    }
    unsigned ticks=cpu->gpr[2];
    if(ticks>1 && ticks<255) {
        if(per_player) {
            if(current_actor!=actor || !damage_owner)return;
            /* Scaling halves hit cooldowns in either mode. Keeping the full
             * native duration here made separate mode block the next aerial
             * swing/combo even though reduced-shared mode admitted it. */
            ticks=(ticks+1)/2;
            if(saber_chain && ticks>4) {
                ticks=4;
                psx_mod_counter_add("mmx4.coop.zero-combo-cooldown",1);
            }
            unsigned b=bank(actor);
            uint8_t *timers=immunity[index][b];
            timers[damage_owner-1u]=(uint8_t)ticks;
            /* These registered sites are real SB/SH V0,offset(S0/S1).
             * Sigma's global timer store is deliberately not projected. */
            uint32_t instruction=psx_mod_read_word(address);
            unsigned op=instruction>>26,base=(instruction>>21)&31u;
            unsigned offset=instruction&0xFFFFu,half=op==0x29u;
            if((op==0x28u || half) && (base==16u || base==17u) &&
               offset>=0x60u && offset+(half?2u:1u)<=STRIDE) {
                cooldown_offset[index][b]=(uint8_t)offset;
                cooldown_half[index][b]=(uint8_t)half;
            }
            /* Native collision/flash gates see the earliest available seat.
             * The filter above retains the halved duration for the attacker.
             * Reactions and AI still execute once in the original caller. */
            unsigned next=timers[0]<timers[1]?timers[0]:timers[1];
            /* Spider installs a temporary hit table and restores it on the
             * timer's 1->0 transition. Zero would skip that cleanup forever.
             * Retain one native expiry tick even when the partner is ready. */
            cpu->gpr[2]=next?next:1u;
            trace_contact("timer-installed",actor,0,damage_owner-1u,ticks);
            psx_mod_counter_add("mmx4.coop.per-player-boss-cooldown",1);
        }else {
            cpu->gpr[2]=(ticks+1)/2;
            psx_mod_counter_add("mmx4.coop.shorter-boss-cooldown",1);
        }
    }
}

PSX_MOD_CONSTRUCTOR(mmx4_register_coop_balance) {
    psx_mod_register_function_entry_plugin(ID,0x80097EECu,melee_spawned);
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

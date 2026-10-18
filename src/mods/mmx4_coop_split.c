#include "mmx4_coop_internal.h"
#include "mmx4_coop_memory.h"
#include "mod_memory.h"
#include <string.h>

#define COOP_ID "mmx4.coop"
#define CAMERA 0x801419B0u
#define CAMERA_BYTES 0xFCu
#define TRAIN_POOL 0x80169CB8u
#define TRAIN_SLOTS 4u
#define TRAIN_ACTOR_BYTES 0x30u
#define VISITS_OFFSET (16u+2u*CAMERA_BYTES)
#define STAIRS_OFFSET (VISITS_OFFSET+MMX4_COOP_PLACEMENTS/8u)
#define JUNGLE_OFFSET (STAIRS_OFFSET+2u*0x30u)
#define STATE_BYTES (JUNGLE_OFFSET+2u*TRAIN_SLOTS*TRAIN_ACTOR_BYTES)

static uint32_t state;
static unsigned scanning,bounds_call,supplemental,spatial_call,background_call;
static unsigned mushroom_camera_call;
static unsigned destructible_camera_call;
static unsigned lift_camera_call;
static unsigned jungle_background_call;
static uint32_t background_actor,background_dx;
static unsigned background_parallax;

int mmx4_coop_split_snapshot_ready(void) {
    return !(scanning || bounds_call || supplemental || spatial_call || mushroom_camera_call || destructible_camera_call || lift_camera_call || jungle_background_call || background_call ||
        background_actor || background_dx || background_parallax);
}
int mmx4_coop_split_snapshot(Mmx4SnapshotIO *s) {
    /* Camera, placement and controller bytes use the guest-memory section. */
    mmx4_snapshot_fixed(s,state);
    return s->ok;
}

static void copy_camera(uint32_t destination,uint32_t source) {
    /* All callers copy one complete camera between disjoint contexts. */
    uint8_t bytes[CAMERA_BYTES];
    mmx4_coop_capture(source,bytes,sizeof bytes);
    mmx4_coop_project(destination,bytes,sizeof bytes);
}
static void read_bytes(uint32_t source,uint8_t *bytes,unsigned size) {
    mmx4_coop_capture(source,bytes,size);
}
static void write_bytes(uint32_t destination,const uint8_t *bytes,unsigned size) {
    mmx4_coop_project(destination,bytes,size);
}
void mmx4_coop_split_local_explosion_palette(CPUState *cpu,const uint8_t camera[0xFC]) {
    /* BBC14's type28 controller centres its explosions at CAMERA+(160,128)
     * in BBDF4, but BBEA4 applies red to the shared palette. Intro part2 also
     * has B5960's type2/subtype0 red pulse (0..3) around the native view.
     * Remove those local effects from a distant independent viewport.
     * This helper runs inside the local-render RAM/VRAM transaction: native
     * colors, upload flags and working palettes restore with that scope. */
    if(!state || !mmx4_coop_split_views() || !mmx4_coop_ready() || mmx4_coop_projected() ||
       !psx_mod_local_view_scope() || psx_mod_read_byte(MMX4_PLAY)!=6 ||
       psx_mod_read_byte(MMX4_PLAY+1) || psx_mod_read_byte(MMX4_PLAY+12) ||
       psx_mod_read_byte(MMX4_PLAY+0x0F) || psx_mod_read_byte(MMX4_PLAY+0x10) ||
       psx_mod_read_byte(0x80175E9Cu) || !psx_mod_read_half(0x80175EA0u) ||
       psx_mod_read_half(0x80175EA0u)>31 ||
       psx_mod_read_half(0x80141BE6u) || psx_mod_read_half(0x8013E1BEu))return;
    int x=(int16_t)((uint16_t)camera[10]|(uint16_t)camera[11]<<8)+160;
    int y=(int16_t)((uint16_t)camera[14]|(uint16_t)camera[15]<<8)+128;
    int margin=psx_mod_widescreen_x_margin();
    unsigned sources=0,pulse=0;
    for(unsigned i=0;i<32;++i) {
        uint32_t actor=0x80142F98u+i*0x30u;
        if(!psx_mod_read_byte(actor) || psx_mod_read_byte(actor+4)!=1)continue;
        if(psx_mod_read_byte(actor+1)==2 && !psx_mod_read_byte(actor+2) &&
           psx_mod_read_byte(MMX4_PLAY+13)==1 && psx_mod_read_half(0x80175EA0u)<=3)
            pulse=1;
        if(psx_mod_read_byte(actor+1)!=28)continue;
        ++sources;
        int dx=(int16_t)psx_mod_read_half(actor+10)-x;
        int dy=(int16_t)psx_mod_read_half(actor+14)-y;
        if(dx>-320-2*margin && dx<320+2*margin && dy>-240 && dy<240)return;
    }
    if(!sources && pulse) {
        /* The looping palette actor's XY is its allocation position, not
         * the region it lights. Its global pulse belongs to the native
         * camera being drawn before this private viewport is substituted. */
        int dx=(int16_t)psx_mod_read_half(CAMERA+10)+160-x;
        int dy=(int16_t)psx_mod_read_half(CAMERA+14)+128-y;
        if(dx>-320-2*margin && dx<320+2*margin && dy>-240 && dy<240)return;
        sources=1;
    }
    if(!sources)return; /* Other red flashes and ordinary fades remain native. */
    psx_mod_write_half(0x80175EA0u,0);
    psx_mod_write_byte(0x80166BB0u,(uint8_t)(psx_mod_read_byte(0x80166BB0u)|4u));
    mmx4_coop_call(cpu,0x8002A484u,0,0); /* Rebuild the unlit working palette. */
    mmx4_coop_call(cpu,0x80016004u,0,0); /* Upload within the private GPU pass. */
    psx_mod_counter_add("mmx4.coop.split-distant-explosion-flash-removed",1);
}
static int available(void) {
    return state && mmx4_coop_split_views() && mmx4_coop_ready() &&
        (!mmx4_coop_projected() || mmx4_coop_combat_actor_context() || spatial_call) &&
        psx_mod_read_byte(MMX4_PLAY)==6 &&
        !psx_mod_local_view_scope();
}
int mmx4_coop_split_sprite_outside(uint32_t actor) {
    if(!mmx4_coop_split_views() || !mmx4_coop_ready() ||
       psx_mod_read_byte(MMX4_PLAY)!=6)return 0;
    unsigned layer=psx_mod_read_byte(actor+0x14u);
    if(layer>=3)return 0; /* Screen-relative UI and scripted graphics. */
    uint32_t camera=CAMERA+layer*0x54u;
    int x=(int16_t)(psx_mod_read_half(actor+10)-psx_mod_read_half(camera+10));
    int y=(int16_t)(psx_mod_read_half(actor+14)-psx_mod_read_half(camera+14));
    int margin=psx_mod_widescreen_x_margin();
    /* 80024334 and the armor emitter use signed byte assembly offsets and
     * 16px tiles, including horizontal flips. Keep a conservative 144px
     * border, then reject before the GPU truncates coordinates to 11 bits.
     * An actor can remain live/drawable for the other camera; neither its
     * simulation visibility flag nor its lifetime may be changed here. */
    return x < -144-margin || x > 320+144+margin || y < -144 || y > 240+144;
}
static int draw_armor(CPUState *cpu,uint32_t address) {
    /* One native armor hook must retain both camera culling and rider order.
     * A second registration for this plugin replaces the first callback. */
    if(mmx4_coop_split_sprite_outside(cpu->gpr[4]))return mmx4_coop_finish(cpu,0);
    return mmx4_coop_render_rider_armor(cpu,address);
}
static int intro_boss(uint32_t actor) {
    return actor>=0x8013BED0u && actor<0x8013BED0u+48u*0x9Cu &&
        !((actor-0x8013BED0u)%0x9Cu) && psx_mod_read_byte(actor) &&
        psx_mod_read_byte(actor+1)==0;
}
static void project_intro_sprite(uint32_t actor) {
    if(!psx_mod_read_byte(actor) || psx_mod_read_byte(actor+0x14u)!=255)return;
    /* Native80041F00/8004205C change this boss between screen and world
     * coordinates; type10 attachments inherit the same layer. Rebase only
     * in the private draw transaction, before replacing the canonical camera.
     * Integer halfwords match the native emitter without fractional carry. */
    psx_mod_write_half(actor+10,psx_mod_read_half(actor+10)+psx_mod_read_half(CAMERA+10));
    psx_mod_write_half(actor+14,psx_mod_read_half(actor+14)+psx_mod_read_half(CAMERA+14));
    psx_mod_write_byte(actor+0x14u,0);
    psx_mod_counter_add("mmx4.coop.split-intro-sprite-projected",1);
}
void mmx4_coop_split_project_intro_boss(void) {
    if(!psx_mod_local_view_scope() || !mmx4_coop_split_views() || !mmx4_coop_ready() ||
       psx_mod_read_byte(MMX4_PLAY)!=6 || psx_mod_read_byte(MMX4_PLAY+12) ||
       psx_mod_read_byte(MMX4_PLAY+13))return;
    for(unsigned i=0;i<48;++i) {
        uint32_t actor=0x8013BED0u+i*0x9Cu;
        if(intro_boss(actor))project_intro_sprite(actor);
    }
    for(unsigned i=0;i<32;++i) {
        uint32_t actor=0x8013E510u+i*0x70u;
        if(psx_mod_read_byte(actor+1)==10 && intro_boss(psx_mod_read_word(actor+0x50)))
            project_intro_sprite(actor);
    }
}
void mmx4_coop_split_reset(void) {
    if(!state || mmx4_coop_projected() || psx_mod_local_view_scope())return;
    for(unsigned offset=0;offset<STATE_BYTES;offset+=4)psx_mod_write_word(state+offset,0);
    scanning=bounds_call=supplemental=spatial_call=mushroom_camera_call=destructible_camera_call=lift_camera_call=jungle_background_call=background_call=0;
    background_actor=background_dx=background_parallax=0;
}
int mmx4_coop_split_activate(void) {
    state=mmx4_coop_split_views()?psx_mod_memory_alloc(STATE_BYTES,4):0;
    mmx4_coop_split_reset();
    if(state)psx_mod_counter_add("mmx4.coop.split-state",state);
    return !mmx4_coop_split_views() || state!=0;
}
uint32_t mmx4_coop_split_digest(uint32_t seed) {
    if(state)for(unsigned offset=0;offset<STATE_BYTES;++offset)
        seed=(seed^psx_mod_read_byte(state+offset))*16777619u;
    return seed;
}
void mmx4_coop_split_actors(Mmx4CoopViewActor actors[2]) {
    unsigned projected=(unsigned)(mmx4_coop_projected()!=0);
    for(unsigned seat=0;seat<2;++seat) {
        const uint8_t *body=seat?mmx4_coop_second_body():mmx4_coop_first_body();
        int16_t world_x=seat==projected?(int16_t)psx_mod_read_half(MMX4_PLAYER+10):
            (int16_t)((uint16_t)body[10]|(uint16_t)body[11]<<8);
        int16_t world_y=seat==projected?(int16_t)psx_mod_read_half(MMX4_PLAYER+14):
            (int16_t)((uint16_t)body[14]|(uint16_t)body[15]<<8);
        actors[seat]=(Mmx4CoopViewActor){world_x,world_y,(unsigned)mmx4_coop_alive(seat)};
    }
}
unsigned mmx4_coop_split_view_seat(unsigned seat) {
    if(seat>1)seat=0;
    int owner=mmx4_coop_lifecycle_script_owner();
    if(owner>=0 && !mmx4_coop_lifecycle_paired_door())return (unsigned)owner;
    if(!mmx4_coop_alive(seat))return mmx4_coop_alive(seat^1u)?seat^1u:0;
    return seat;
}
int mmx4_coop_split_camera_copy(unsigned seat,uint8_t layers[0xFC]) {
    if(!state || !mmx4_coop_split_views() || seat>1)return 0;
    /* Initial arrivals render before the first enabled camera tick. Use the
     * authored starting camera without mutating snapshot state in a draw. */
    if(!psx_mod_read_word(state)) {read_bytes(CAMERA,layers,CAMERA_BYTES);return 1;}
    seat=mmx4_coop_split_view_seat(seat);
    read_bytes(state+16u+seat*CAMERA_BYTES,layers,CAMERA_BYTES);
    return 1;
}
static void initialize_cameras(void) {
    if(psx_mod_read_word(state))return;
    copy_camera(state+16u,CAMERA);
    copy_camera(state+16u+CAMERA_BYTES,CAMERA);
    psx_mod_write_word(state,1);
}
static uint32_t byte_word(const uint8_t *bytes,unsigned offset) {
    return (uint32_t)bytes[offset]|(uint32_t)bytes[offset+1]<<8|
        (uint32_t)bytes[offset+2]<<16|(uint32_t)bytes[offset+3]<<24;
}
static uint32_t signed_half(uint32_t value) {
    /* Retail sra retains the sign bit, including negative odd fixed units. */
    return (value>>1)|(value&0x80000000u);
}
static int destructible_camera_unlock(CPUState *cpu,uint32_t address) {
    if(destructible_camera_call || !available() ||
       psx_mod_read_byte(MMX4_PLAY+12) || psx_mod_read_byte(MMX4_PLAY+13))return 0;
    uint32_t actor=cpu->gpr[4];
    if(actor<0x80165A30u || actor>=0x80166BB0u || (actor-0x80165A30u)%0x8Cu ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=4 ||
       psx_mod_read_byte(actor+2))return 0;
    initialize_cameras();destructible_camera_call=1;
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    /* Retail C0D98 retires the intro's destroyed type4/subtype0 door and
     * writes 4544 to the current/target maximum X at CAMERA+1C/+24. The
     * shared map opens for both players, but independent camera snapshots
     * otherwise retain the old 2976 limit and trap P2 after the opening.
     * Native 2174C dispatches 32 8C-byte slots before the camera tick at
     * 211D8, so both shadows must receive the release before that tick.
     * Publish exactly those native writes; all other room bounds, origins,
     * and the one map/palette/actor finalization remain native. */
    for(unsigned seat=0;seat<2;++seat)if(mmx4_coop_alive(seat)) {
        uint32_t shadow=state+16u+seat*CAMERA_BYTES;
        psx_mod_write_half(shadow+0x1C,psx_mod_read_half(CAMERA+0x1C));
        psx_mod_write_half(shadow+0x24,psx_mod_read_half(CAMERA+0x24));
    }
    destructible_camera_call=0;
    psx_mod_counter_add("mmx4.coop.split-destructible-camera-unlocks",1);
    return mmx4_coop_finish(cpu,result);
}
static int mushroom_lift_camera(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(lift_camera_call)return 0;
    if(!available())return mmx4_coop_lifecycle_lift_start(cpu,address);
    if(psx_mod_read_byte(MMX4_PLAY+12)!=3 ||
       actor<0x80165A30u || actor>=0x80166BB0u || (actor-0x80165A30u)%0x8Cu ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=14)return 0;
    initialize_cameras();
    uint8_t before[CAMERA_BYTES];read_bytes(CAMERA,before,sizeof before);
    lift_camera_call=1;
    uint32_t result=mmx4_coop_lifecycle_lift_start(cpu,address)?cpu->gpr[2]:
        mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    /* Native C3B40 releases the lift's vertical targets from2048 to720.
     * C3BA4/C3C9C subsequently change the current limits at its top. Apply
     * those authored writes to every view in this shaft, once, without
     * repeating the lift, panels, sound, checkpoint or collision update.
     * Each camera keeps its own origin and ordinary native follow. A view
     * in another horizontal room retains its independent limits. */
    static const unsigned horizontal[]={0x1C,0x1E,0x24,0x26};
    static const unsigned vertical[]={0x20,0x22,0x28,0x2A,0x2C};
    for(unsigned seat=0;seat<2;++seat)if(mmx4_coop_alive(seat)) {
        uint32_t shadow=state+16u+seat*CAMERA_BYTES;
        unsigned same_room=1;
        for(unsigned i=0;i<4;++i)if(psx_mod_read_half(shadow+horizontal[i])!=
            (uint16_t)(before[horizontal[i]]|(uint16_t)before[horizontal[i]+1]<<8))same_room=0;
        if(!same_room)continue;
        for(unsigned i=0;i<5;++i) {
            unsigned offset=vertical[i];
            uint16_t old=(uint16_t)(before[offset]|(uint16_t)before[offset+1]<<8);
            uint16_t next=psx_mod_read_half(CAMERA+offset);
            if(next!=old)psx_mod_write_half(shadow+offset,next);
        }
        if(before[0x47]!=psx_mod_read_byte(CAMERA+0x47))
            psx_mod_write_byte(shadow+0x47,psx_mod_read_byte(CAMERA+0x47));
    }
    lift_camera_call=0;
    psx_mod_counter_add("mmx4.coop.split-lift-camera",1);
    return mmx4_coop_finish(cpu,result);
}
static int train_foreground(CPUState *cpu,uint32_t address) {
    if(background_call!=1 || cpu->gpr[4]!=background_actor)return 0;
    /* Full native800D964C adds foreground displacement only on this branch.
     * Observe it at the actual dispatch, after the native phase selector.
     * A stopped train's +7 transition does not add parallax on that tick. */
    if(psx_mod_read_byte(background_actor+7) && !psx_mod_read_byte(MMX4_PLAYER+0xBC)) {
        background_parallax=1;
        background_dx=psx_mod_read_word(CAMERA+8)-psx_mod_read_word(CAMERA+20);
    }
    background_call=2;
    uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    background_call=1;
    return mmx4_coop_finish(cpu,result);
}
static int moving_background(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    unsigned storm=address==0x800DA298u;
    unsigned kind=storm?5u:address==0x800D8648u?0u:2u;
    if(background_call || !available() || mmx4_coop_projected() ||
       psx_mod_read_byte(MMX4_PLAY+12)!=(storm?7u:8u) || actor<TRAIN_POOL ||
       actor>=TRAIN_POOL+TRAIN_SLOTS*TRAIN_ACTOR_BYTES ||
       (actor-TRAIN_POOL)%TRAIN_ACTOR_BYTES ||
       psx_mod_read_byte(actor+1)!=kind)return 0;
    initialize_cameras();
    uint8_t before[CAMERA_BYTES],after[CAMERA_BYTES];
    read_bytes(CAMERA,before,CAMERA_BYTES);
    unsigned storm_live=storm && psx_mod_read_byte(actor+4)<=1;
    background_actor=actor;background_dx=background_parallax=0;background_call=1;
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    read_bytes(CAMERA,after,CAMERA_BYTES);
    unsigned previous=psx_mod_read_word(state+8u);
    unsigned owner=previous?previous-1u:0u;
    /* QA87 traces D9218 a0=80169CB8, ra=80021BDC. Native80021B34 and its
     * allocator8002B048 agree: four30-byte actors, table800F2AB4, kinds0/2.
     * The train's mode5 helpers cannot recreate these shared actor writes.
     * They run after the current player camera snapshot in80021158.
     * Apply their writes to both saved views, never to canonical RAM again.
     * 0/2 add +18 speed and its arithmetic half. Only D9218's inspected
     * 800D964C branch also adds foreground motion; replace that contribution
     * with the other view's own displacement when the branch actually runs.
     * Storm Owl's kind5 controller uses the same pool/timing, but DA358's
     * foreground contribution uses signed truncation instead of sra. */
    for(unsigned seat=0;seat<2;++seat) {
        uint32_t shadow=state+16u+seat*CAMERA_BYTES;
        uint32_t seat_dx=psx_mod_read_word(shadow+8)-psx_mod_read_word(shadow+20);
        for(unsigned layer=0;layer<3;++layer) {
            unsigned base=layer*0x54u;
            for(unsigned offset=0;offset<0x54u;++offset) {
                if((offset>=8u && offset<16u) || (offset>=20u && offset<28u))continue;
                if(before[base+offset]!=after[base+offset])
                    psx_mod_write_byte(shadow+base+offset,after[base+offset]);
            }
            /* Shared authored limits/shake metadata are copied above, while
             * each foreground's current/previous origins stay independent. */
            if(!layer)continue;
            for(unsigned axis=8;axis<=12;axis+=4) {
                uint32_t delta=byte_word(after,base+axis)-byte_word(before,base+axis);
                uint32_t value=psx_mod_read_word(shadow+base+axis)+delta;
                if(background_parallax && seat!=owner && axis==8) {
                    uint32_t native_part=layer==1?background_dx:signed_half(background_dx);
                    uint32_t seat_part=layer==1?seat_dx:signed_half(seat_dx);
                    value+=seat_part-native_part;
                }
                if(storm_live && axis==8 && seat!=owner) {
                    /* DA358 adds the shared aircraft speed and each view's
                     * foreground displacement/2 (near) or /4 (far). Retail
                     * divides previous-current with truncation toward zero;
                     * preserve its negative odd fixed-unit rounding exactly.
                     * The one controller also advances stage events: never
                     * rerun it for the other camera. */
                    uint32_t native_part=byte_word(before,20)-byte_word(before,8);
                    uint32_t seat_part=0u-seat_dx;
                    for(unsigned n=0;n<layer;++n) {
                        native_part=signed_half(native_part+(native_part>>31));
                        seat_part=signed_half(seat_part+(seat_part>>31));
                    }
                    value+=native_part-seat_part;
                }
                psx_mod_write_word(shadow+base+axis,value);
            }
        }
    }
    background_call=background_parallax=0;background_actor=background_dx=0;
    psx_mod_counter_add(storm?"mmx4.coop.split-storm-background-ticks":
                               "mmx4.coop.split-train-background-ticks",1);
    return mmx4_coop_finish(cpu,result);
}
static int jungle_background(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(jungle_background_call || !mmx4_coop_ready() || mmx4_coop_projected() ||
       psx_mod_local_view_scope() || psx_mod_read_byte(MMX4_PLAY)!=6 ||
       psx_mod_read_byte(MMX4_PLAY+12)!=1 || psx_mod_read_byte(MMX4_PLAY+13)!=1 ||
       actor<TRAIN_POOL || actor>=TRAIN_POOL+TRAIN_SLOTS*TRAIN_ACTOR_BYTES ||
       (actor-TRAIN_POOL)%TRAIN_ACTOR_BYTES || psx_mod_read_byte(actor+1)!=1 ||
       psx_mod_read_byte(actor+4)>1)return 0;
    if(!mmx4_coop_split_views()) {
        /* This controller selects scenery directly from PLAYER's X/Y.
         * Unified camera helpers already follow a surviving P2, but cannot
         * correct this selection from the dead canonical body's old room. */
        if(mmx4_coop_alive(0) || !mmx4_coop_alive(1) ||
           mmx4_coop_lifecycle_script_owner()>=0)return 0;
        jungle_background_call=1;mmx4_coop_enter_second();
        uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
        mmx4_coop_leave_second();jungle_background_call=0;
        psx_mod_counter_add("mmx4.coop.unified-survivor-jungle-background",1);
        return mmx4_coop_finish(cpu,result);
    }
    if(!available())return 0;
    unsigned slot=(actor-TRAIN_POOL)/TRAIN_ACTOR_BYTES;
    if(mmx4_coop_lifecycle_script_owner()>=0) {
        /* Shared scenes replace complete camera records. Their mode/offset
         * may belong to the other region, so initialize each private phase
         * again when ordinary independent follow resumes. */
        for(unsigned seat=0;seat<2;++seat)
            psx_mod_write_byte(state+JUNGLE_OFFSET+(seat*TRAIN_SLOTS+slot)*TRAIN_ACTOR_BYTES,0);
        return 0;
    }
    initialize_cameras();
    uint8_t controller[TRAIN_ACTOR_BYTES],canonical[CAMERA_BYTES];
    read_bytes(actor,controller,sizeof controller);
    unsigned previous=psx_mod_read_word(state+8u),owner=previous?previous-1u:0u;
    jungle_background_call=1;
    /* Native D91A8 selects the scenery phase from player X/Y. The canonical
     * pool still runs once, with its current living camera owner. */
    if(owner)mmx4_coop_enter_second();
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    if(owner)mmx4_coop_leave_second();
    read_bytes(CAMERA,canonical,sizeof canonical);
    for(unsigned seat=0;seat<2;++seat) {
        uint32_t local=state+JUNGLE_OFFSET+(seat*TRAIN_SLOTS+slot)*TRAIN_ACTOR_BYTES;
        if(!mmx4_coop_alive(seat)) {
            /* Its camera follows the survivor while dead; discard the
             * preceding independent region phase before any revival. */
            psx_mod_write_byte(local,0);continue;
        }
        if(!psx_mod_read_byte(local) || !controller[4]) {
            write_bytes(local,controller,sizeof controller);
            /* A late enrollment must initialize this view's mode/offset,
             * even if the canonical controller already reached state 1. */
            psx_mod_write_byte(local+4,0);
        }
        uint32_t shadow=state+16u+seat*CAMERA_BYTES;
        copy_camera(CAMERA,shadow);
        if(seat)mmx4_coop_enter_second();
        /* D8ED4's live states write only actor phase bytes and layer 1's
         * mode, offsets and origins (D8FDC..D9194). Keeping those phase bytes
         * per seat preserves its native one-tick region transitions. No
         * animation, palette, collision or shared stage callback is repeated.
         * In particular mode 5's ordinary camera helper cannot update this
         * layer: it otherwise stays at the opening forest over distant rooms. */
        mmx4_coop_call(cpu,address,local,0);
        if(seat)mmx4_coop_leave_second();
        copy_camera(shadow,CAMERA);
        psx_mod_counter_add(seat?"mmx4.coop.p2-jungle-background":
                                "mmx4.coop.p1-jungle-background",1);
    }
    write_bytes(CAMERA,canonical,sizeof canonical);
    jungle_background_call=0;
    return mmx4_coop_finish(cpu,result);
}
static int frost_snow_background(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(background_call || !available() || mmx4_coop_projected() ||
       psx_mod_read_byte(MMX4_PLAY+12)!=2 ||
       actor<0x80142F98u || actor>=0x80143598u ||
       (actor-0x80142F98u)%0x30u || psx_mod_read_byte(actor+1)!=9)return 0;
    initialize_cameras();
    uint32_t snow=CAMERA+2u*0x54u;
    uint8_t before[0x54],after[0x54];
    read_bytes(snow,before,sizeof before);background_call=3;
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    read_bytes(snow,after,sizeof after);
    /* Native B75C8 owns one weather clock. B7764/B7934 add fractional wind
     * and wrap the vertical snow origin. Mode5's camera helper is a no-op,
     * so the noncanonical view otherwise keeps its initial frozen snow.
     * The origin encodes the shared weather atlas phase, independently of
     * foreground position. Publish its exact native value to both views,
     * including absolute phase resets; do not repeat the timer or update. */
    for(unsigned seat=0;seat<2;++seat) {
        uint32_t shadow=state+16u+seat*CAMERA_BYTES+2u*0x54u;
        for(unsigned offset=0;offset<sizeof before;++offset) {
            if((offset>=8u && offset<16u) || (offset>=20u && offset<28u))continue;
            if(before[offset]!=after[offset])psx_mod_write_byte(shadow+offset,after[offset]);
        }
        for(unsigned axis=8;axis<=12;axis+=4)
            psx_mod_write_word(shadow+axis,byte_word(after,axis));
    }
    background_call=0;
    psx_mod_counter_add("mmx4.coop.split-snow-background-ticks",1);
    return mmx4_coop_finish(cpu,result);
}
int mmx4_coop_split_scene_camera_begin(unsigned owner,uint8_t canonical[0xFC]) {
    if(!available() || owner>1 || mmx4_coop_lifecycle_script_owner()>=0 ||
       psx_mod_read_byte(MMX4_PLAY+0x10) || psx_mod_read_byte(MMX4_PLAY+0x1C))return 0;
    initialize_cameras();read_bytes(CAMERA,canonical,CAMERA_BYTES);
    copy_camera(CAMERA,state+16u+owner*CAMERA_BYTES);
    return 1;
}
void mmx4_coop_split_scene_camera_end(unsigned owner,const uint8_t canonical[0xFC],int committed) {
    copy_camera(state+16u+owner*CAMERA_BYTES,CAMERA);
    if(committed)psx_mod_write_word(state+8u,owner+1u);
    else write_bytes(CAMERA,canonical,CAMERA_BYTES);
}
void mmx4_coop_split_camera_prepare(unsigned owner) {
    if(!available() || owner>1)return;
    initialize_cameras();
    unsigned previous=psx_mod_read_word(state+8u);
    /* A surviving P2 must start its native camera update in its own room,
     * rather than inherit the fallen P1's origins and clamping targets. */
    if(previous && previous!=owner+1u && mmx4_coop_lifecycle_script_owner()<0 &&
       !psx_mod_read_byte(MMX4_PLAY+0x10) && !psx_mod_read_byte(MMX4_PLAY+0x1C))
        copy_camera(CAMERA,state+16u+owner*CAMERA_BYTES);
    psx_mod_write_word(state+8u,owner+1u);
}
void mmx4_coop_split_flip_cameras(void) {
    if(!available() || !psx_mod_read_word(state))return;
    for(unsigned seat=0;seat<2;++seat)for(unsigned layer=0;layer<3;++layer) {
        uint32_t camera=state+16u+seat*CAMERA_BYTES+layer*0x54u;
        uint32_t axis=layer?0x09000000u:0x07000000u;
        for(unsigned at=12;at<=24;at+=12)
            psx_mod_write_word(camera+at,axis-psx_mod_read_word(camera+at));
        if(!layer) {
            /* C4F40 also calls C5210, which replaces the room's vertical
             * follow/limit pairs. Mirroring XY alone leaves P2 clamped to
             * the old floor after the world has turned over, crushing a
             * grounded partner on the very first flipped camera update.
             * Keep each view's horizontal limits and position independent. */
            static const unsigned offsets[]={0x20,0x22,0x28,0x2A};
            for(unsigned i=0;i<4;++i)psx_mod_write_half(camera+offsets[i],
                psx_mod_read_half(CAMERA+offsets[i]));
        }else {
            /* C5210 shifts the authored parallax Y origin by 128/192.
             * Apply its delta to each view's own origin, once per flip. */
            int delta=layer==1?128:192;
            if(!psx_mod_read_byte(MMX4_PLAY+0x26))delta=-delta;
            psx_mod_write_half(camera+0x42,psx_mod_read_half(camera+0x42)+delta);
        }
        psx_mod_write_byte(camera+0x4C,1);
    }
}
void mmx4_coop_split_camera(CPUState *cpu,unsigned owner) {
    if(!available())return;
    initialize_cameras();
    uint8_t canonical[CAMERA_BYTES];read_bytes(CAMERA,canonical,CAMERA_BYTES);
    mmx4_coop_call(cpu,0x80027D40u,0,0);
    mmx4_coop_call(cpu,0x800281E8u,0,0);
    copy_camera(state+16u+owner*CAMERA_BYTES,CAMERA);
    psx_mod_write_word(state+8u,owner+1u);
    write_bytes(CAMERA,canonical,CAMERA_BYTES);
    unsigned other=owner^1u;
    /* PLAY+10 also freezes the world while a native pickup counts health.
     * That pause must not copy room bounds or clamp the distant partner. */
    unsigned paired=mmx4_coop_lifecycle_paired_door();
    if((mmx4_coop_lifecycle_script_owner()>=0 && !paired) ||
       (psx_mod_read_byte(MMX4_PLAY+0x1C) && !paired && !mmx4_coop_lifecycle_nova_active()) || !mmx4_coop_alive(other)) {
        copy_camera(state+16u+other*CAMERA_BYTES,state+16u+owner*CAMERA_BYTES);
        mmx4_coop_lifecycle_camera_bounds(cpu,owner);
        return;
    }
    uint32_t shadow=state+16u+other*CAMERA_BYTES;
    /* Both participants share the door's authored room limits while it is
     * opening, but remain independently controlled through the aperture.
     * Retain each origin and run ordinary follow for the other body rather
     * than substituting the opener's whole camera/HUD until they cross. */
    if(paired)for(unsigned offset=0x1C;offset<0x2C;offset+=2)
        psx_mod_write_half(shadow+offset,(uint16_t)(canonical[offset]|
            (uint16_t)canonical[offset+1]<<8));
    /* Current/target limits, dead zones and authored room metadata belong
     * to the individual view. Shared scripts synchronize whole records above. */
    for(unsigned offset=0;offset<8u;++offset)psx_mod_write_byte(shadow+offset,canonical[offset]);
    copy_camera(CAMERA,shadow);
    if(other)mmx4_coop_enter_second();
    mmx4_coop_call(cpu,0x80027850u,0,0);
    mmx4_coop_call(cpu,0x80027D40u,0,0);
    mmx4_coop_call(cpu,0x800281E8u,0,0);
    copy_camera(shadow,CAMERA);
    if(other)mmx4_coop_leave_second();
    write_bytes(CAMERA,canonical,CAMERA_BYTES);
    psx_mod_counter_add("mmx4.coop.split-camera-ticks",1);
    if(paired)psx_mod_counter_add("mmx4.coop.split-paired-door-camera-ticks",1);
}
static int ram_range(uint32_t address,unsigned bytes) {
    return address>=0x80010000u && address<0x80200000u && bytes<=0x80200000u-address;
}
static int in_camera_area(uint32_t parameters,Mmx4CoopViewActor actor) {
    return actor.active && ram_range(parameters,10u) &&
        actor.world_x<=(int)psx_mod_read_half(parameters) &&
        actor.world_x>=(int)psx_mod_read_half(parameters+2u) &&
        actor.world_y<=(int)psx_mod_read_half(parameters+4u) &&
        actor.world_y>=(int)psx_mod_read_half(parameters+6u);
}
static void apply_camera_area(uint32_t parameters,uint32_t destination) {
    /* Retail 800B5798: region bounds then IDs in 8010AE74's (variable,value)
     * table, resolved through 8010AF9C. Only camera variables are per view;
     * the one original callback applies any shared variables once. */
    psx_mod_write_byte(destination+0x47u,2);
    for(unsigned index=4;index<36 && ram_range(parameters+index*2u,2);++index) {
        unsigned id=psx_mod_read_half(parameters+index*2u);if(!id)break;
        if(id>74u)break;
        uint32_t pair=0x8010AE74u+(id-1u)*4u;
        unsigned variable=psx_mod_read_half(pair);if(variable>=256u)break;
        uint32_t target=psx_mod_read_word(0x8010AF9Cu+variable*4u);
        if(target>=CAMERA && target+2u<=CAMERA+CAMERA_BYTES && !(target&1u))
            psx_mod_write_half(destination+target-CAMERA,psx_mod_read_half(pair+2u));
    }
}
static int spatial_stage_actor(CPUState *cpu,uint32_t address) {
    if(spatial_call || !mmx4_coop_ready() || mmx4_coop_projected() ||
       psx_mod_read_byte(MMX4_PLAY)!=6 || psx_mod_local_view_scope())return 0;
    int split=mmx4_coop_split_views();
    if(split && !available())return 0;
    uint32_t actor=cpu->gpr[4];
    if(actor<0x80142F98u || actor>=0x80143598u || (actor-0x80142F98u)%0x30u)return 0;
    Mmx4CoopViewActor actors[2];mmx4_coop_split_actors(actors);
    unsigned kind=address==0x800B56F4u?0u:6u;
    if(psx_mod_read_byte(actor+1u)!=kind)return 0;
    if(split)initialize_cameras();
    uint32_t parameters=psx_mod_read_byte(actor+4u)?psx_mod_read_word(actor+0x18u):0;
    Mmx4CoopViewActor eligible[2]={actors[0],actors[1]};
    for(unsigned seat=0;seat<2;++seat) {
        if(!kind && ram_range(parameters,10u))eligible[seat].active=(unsigned)in_camera_area(parameters,actors[seat]);
        if(kind)eligible[seat].active=actors[seat].active &&
            ((psx_mod_read_byte(actor+2u)&0xF0u)?
                actors[seat].world_y>=(int16_t)psx_mod_read_half(actor+14u):
                actors[seat].world_x>=(int16_t)psx_mod_read_half(actor+10u));
    }
    int scene=mmx4_coop_lifecycle_script_owner();
    unsigned target=scene>=0?(unsigned)scene:mmx4_coop_view_nearest(
        eligible[0].active || eligible[1].active?eligible:actors,
        (int16_t)psx_mod_read_half(actor+10u),(int16_t)psx_mod_read_half(actor+14u),0);
    uint8_t canonical[CAMERA_BYTES];
    int16_t old_bounds[4];
    for(unsigned i=0;i<4;++i)old_bounds[i]=(int16_t)psx_mod_read_half(CAMERA+0x24u+i*2u);
    int own_camera=target && mmx4_coop_split_scene_camera_begin(target,canonical);
    spatial_call=1;
    uint8_t campaign=psx_mod_read_byte(MMX4_PLAY+0x43);
    if(target) {mmx4_coop_enter_second();psx_mod_write_byte(MMX4_PLAY+0x43,campaign);}
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    if(target) {
        psx_mod_write_byte(MMX4_PLAY+0x43,psx_mod_read_byte(MMX4_PLAYER+2));
        mmx4_coop_leave_second();
    }
    spatial_call=0;
    if(own_camera)mmx4_coop_split_scene_camera_end(target,canonical,0);
    if(!split && !kind && scene<0 && actors[target].active && actors[target^1u].active) {
        int16_t bounds[4];
        for(unsigned i=0;i<4;++i)bounds[i]=(int16_t)psx_mod_read_half(CAMERA+0x24u+i*2u);
        int narrowed=bounds[0]<old_bounds[0] || bounds[1]>old_bounds[1] ||
            bounds[2]<old_bounds[2] || bounds[3]>old_bounds[3];
        Mmx4CoopViewActor other=actors[target^1u];
        if(narrowed && bounds[0]>=bounds[1] && bounds[2]>=bounds[3] &&
           (other.world_x<bounds[1] || other.world_x>bounds[0]+320 ||
            other.world_y<bounds[3] || other.world_y>bounds[2]+256))
            mmx4_coop_lifecycle_room_entry(target);
    }
    if(split && !kind && scene<0 && !psx_mod_read_byte(MMX4_PLAY+0x10) && !psx_mod_read_byte(MMX4_PLAY+0x1C)) {
        parameters=psx_mod_read_word(actor+0x18u);
        for(unsigned seat=0;seat<2;++seat)if(in_camera_area(parameters,actors[seat])) {
            apply_camera_area(parameters,state+16u+seat*CAMERA_BYTES);
            if(seat==(unsigned)(!mmx4_coop_alive(0)))apply_camera_area(parameters,CAMERA);
        }
    }
    psx_mod_counter_add(target?"mmx4.coop.p2-spatial-stage":"mmx4.coop.p1-spatial-stage",1);
    return mmx4_coop_finish(cpu,result);
}
static void mushroom_stair_views(CPUState *cpu,uint32_t actor,unsigned section) {
    if(!available() || psx_mod_read_byte(actor+4)!=1 ||
       psx_mod_read_byte(MMX4_PLAY+1) || psx_mod_read_byte(MMX4_PLAY+0x10) ||
       psx_mod_read_byte(MMX4_PLAY+0x1C))return;
    initialize_cameras();
    Mmx4CoopViewActor actors[2];mmx4_coop_split_actors(actors);
    uint8_t canonical[CAMERA_BYTES];read_bytes(CAMERA,canonical,sizeof canonical);
    unsigned owner=psx_mod_read_word(state+8u);
    owner=owner?owner-1u:0u;
    for(unsigned seat=0;seat<2;++seat) {
        uint32_t animation=state+STAIRS_OFFSET+seat*0x30u;
        /* Retail B7528's first region is the rotating staircase, in either
         * section. Only B7140's camera/animation helper is repeated here:
         * B7394/B7440/B74EC still commit shared checkpoints exactly once. */
        if(!actors[seat].active || actors[seat].world_x>=
           (int16_t)psx_mod_read_half(section?0x8010BE1Cu:0x8010B4D0u)) {
            psx_mod_write_byte(animation+6,0);continue;
        }
        uint32_t shadow=state+16u+seat*CAMERA_BYTES;
        copy_camera(CAMERA,shadow);
        mmx4_coop_call(cpu,section?0x800BA644u:0x800B7140u,animation,0);
        copy_camera(shadow,CAMERA);
        if(seat==owner) {
            canonical[4]=psx_mod_read_byte(CAMERA+4);
            read_bytes(CAMERA+0x54u+8,canonical+0x54u+8,4);
            canonical[0x54u+0x4C]=psx_mod_read_byte(CAMERA+0x54u+0x4C);
        }
        psx_mod_counter_add(seat?"mmx4.coop.p2-stair-animation":"mmx4.coop.p1-stair-animation",1);
    }
    write_bytes(CAMERA,canonical,sizeof canonical);
}
static int mushroom_stage_camera(CPUState *cpu,uint32_t address) {
    if(mushroom_camera_call || !mmx4_coop_ready() || mmx4_coop_projected() ||
       psx_mod_local_view_scope() || psx_mod_read_byte(MMX4_PLAY)!=6 ||
       psx_mod_read_byte(MMX4_PLAY+12)!=3 || mmx4_coop_lifecycle_script_owner()>=0 ||
       !mmx4_coop_alive(1))return 0;
    uint32_t actor=cpu->gpr[4];
    unsigned section=address==0x800BA57Cu;
    if(actor<0x80142F98u || actor>=0x80143598u || (actor-0x80142F98u)%0x30u ||
       psx_mod_read_byte(actor+1)!=(section?22u:8u))return 0;
    Mmx4CoopViewActor actors[2];mmx4_coop_split_actors(actors);
    unsigned returning=!section && psx_mod_read_byte(MMX4_PLAY+0x1D)>=2;
    unsigned second_leads=!actors[0].active || (returning?actors[0].world_x>actors[1].world_x:
                                                       actors[0].world_x<actors[1].world_x);
    if(!second_leads && !mmx4_coop_split_views())return 0;
    /* Retail B7528 selects the spiral/room phase from player X/Y. B7394
     * advances to the lift checkpoint at X3536; B7440/B74EC advance the
     * leftward return at its top. These routines read player position but
     * write only shared camera/script fields. Supply the progressing seat
     * for one native update; no player controller or world loop is repeated. */
    uint32_t x=psx_mod_read_word(MMX4_PLAYER+8),y=psx_mod_read_word(MMX4_PLAYER+12);
    const uint8_t *body=mmx4_coop_second_body();
    uint32_t second_x=(uint32_t)body[8]|(uint32_t)body[9]<<8|
        (uint32_t)body[10]<<16|(uint32_t)body[11]<<24;
    uint32_t second_y=(uint32_t)body[12]|(uint32_t)body[13]<<8|
        (uint32_t)body[14]<<16|(uint32_t)body[15]<<24;
    if(second_leads) {
        psx_mod_write_word(MMX4_PLAYER+8,second_x);psx_mod_write_word(MMX4_PLAYER+12,second_y);
    }
    mushroom_camera_call=1;
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    mushroom_camera_call=0;
    psx_mod_write_word(MMX4_PLAYER+8,x);psx_mod_write_word(MMX4_PLAYER+12,y);
    mushroom_stair_views(cpu,actor,section);
    if(second_leads)psx_mod_counter_add("mmx4.coop.p2-mushroom-stage",1);
    return mmx4_coop_finish(cpu,result);
}
static unsigned eligible_regions(PSXPlacementRect regions[2]) {
    Mmx4CoopView views[2];Mmx4CoopViewActor actors[2];
    initialize_cameras();mmx4_coop_split_actors(actors);
    for(unsigned seat=0;seat<2;++seat) {
        uint32_t shadow=state+16u+seat*CAMERA_BYTES;
        views[seat]=(Mmx4CoopView){(int16_t)psx_mod_read_half(shadow+10),
            (int16_t)psx_mod_read_half(shadow+14)};
    }
    int owner=mmx4_coop_lifecycle_paired_door()?-1:mmx4_coop_lifecycle_script_owner();
    return mmx4_coop_view_regions(views,actors,owner,
        psx_mod_widescreen_x_margin(),regions);
}
static uint32_t placement_table(void) {
    int stage=(int8_t)psx_mod_read_byte(MMX4_PLAY+12);
    int section=(int8_t)psx_mod_read_byte(MMX4_PLAY+13);
    if(stage<0 || stage>=15 || section<0 || section>1)return 0;
    uint32_t table=psx_mod_read_word(0x800F43C8u+(unsigned)stage*8u+(unsigned)section*4u);
    /* Native records have an eight-byte stride relative to their table.
     * The table itself is only four-byte aligned (Sigma1: 800F848C). */
    return (table&3u) || table<0x80010000u || table>=0x80200000u?0:table;
}
static int placement_index(uint32_t record,unsigned *index) {
    uint32_t table=psx_mod_read_word(state+4u);
    if(!table || record<table || ((record-table)&7u) ||
       record-table>=MMX4_COOP_PLACEMENTS*8u)return 0;
    *index=(record-table)/8u;return 1;
}
static void reset_placements(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;mmx4_coop_split_reset();
}
static void placement_spawned(CPUState *cpu,uint32_t address) {
    (void)address;unsigned index;
    if(!available() || !cpu->gpr[17] || !placement_index(cpu->gpr[19],&index))return;
    uint32_t byte=state+VISITS_OFFSET+index/8u;
    psx_mod_write_byte(byte,psx_mod_read_byte(byte)|(1u<<(index&7u)));
    psx_mod_counter_add("mmx4.coop.split-placement-visited",1);
}
static int placement_allocate(CPUState *cpu,uint32_t address) {
    (void)address;unsigned index;
    if(!available() || cpu->gpr[31]!=0x8002916Cu ||
       !placement_index(cpu->gpr[19],&index))return 0;
    uint32_t record=cpu->gpr[19];unsigned category=psx_mod_read_byte(record+3u);
    unsigned refight_portal=category==4u && psx_mod_read_byte(record+1u)==27u &&
        psx_mod_read_byte(MMX4_PLAY+12)==12 && !psx_mod_read_byte(MMX4_PLAY+13);
    if((psx_mod_read_byte(state+VISITS_OFFSET+index/8u)&(1u<<(index&7u))) ||
       (supplemental && (psx_mod_read_byte(record)&0x70u)) ||
       (supplemental && !(category<3u ||
            (category==4u && (psx_mod_read_byte(record+1u)==4u ||
                             psx_mod_read_byte(record+1u)==8u)) || refight_portal))) {
        psx_mod_counter_add("mmx4.coop.split-respawn-suppressed",1);
        return mmx4_coop_finish(cpu,0);
    }
    return 0;
}
static void scan_rectangle(CPUState *cpu,PSXPlacementRect region) {
    CPUState saved=*cpu;
    cpu->gpr[29]-=32u;
    uint32_t argument=cpu->gpr[29]+16u,previous=psx_mod_read_word(argument);
    cpu->gpr[4]=(uint32_t)region.left;cpu->gpr[5]=(uint32_t)region.right;
    cpu->gpr[6]=(uint32_t)region.top;cpu->gpr[7]=(uint32_t)region.bottom;
    psx_mod_write_word(argument,0);
    cpu->gpr[31]=0x80028FA8u;
    psx_dispatch_call(cpu,0x80028FECu,cpu->gpr[31]);
    psx_mod_write_word(argument,previous);
    if(cpu->muldiv_ts_done>saved.muldiv_ts_done)saved.muldiv_ts_done=cpu->muldiv_ts_done;
    if(cpu->gte_ts_done>saved.gte_ts_done)saved.gte_ts_done=cpu->gte_ts_done;
    *cpu=saved;
}
static int scan_players(CPUState *cpu,uint32_t address) {
    if(scanning || !available())return 0;
    uint32_t table=placement_table();if(!table)return 0;
    if(table!=psx_mod_read_word(state+4u)) {
        for(unsigned offset=VISITS_OFFSET;offset<STATE_BYTES;offset+=4)
            psx_mod_write_word(state+offset,0);
        psx_mod_write_word(state+4u,table);
    }
    PSXPlacementRect regions[2];unsigned count=eligible_regions(regions);
    for(unsigned index=0;index<MMX4_COOP_PLACEMENTS && table+index*8u+8u<=0x80200000u;++index) {
        uint32_t record=table+index*8u;
        if(psx_mod_read_byte(record+3u)==255u)break;
        int in_region=mmx4_coop_view_contains(regions,count,
            (int16_t)psx_mod_read_half(record+4u),(int16_t)psx_mod_read_half(record+6u));
        uint32_t byte=state+VISITS_OFFSET+index/8u;unsigned mask=1u<<(index&7u);
        if(!in_region)psx_mod_write_byte(byte,psx_mod_read_byte(byte)&~mask);
        else if(psx_mod_read_byte(record)&1u)psx_mod_write_byte(byte,psx_mod_read_byte(byte)|mask);
    }
    uint8_t canonical[CAMERA_BYTES];read_bytes(CAMERA,canonical,CAMERA_BYTES);
    Mmx4CoopViewActor actors[2];mmx4_coop_split_actors(actors);
    int owner=mmx4_coop_lifecycle_paired_door()?-1:mmx4_coop_lifecycle_script_owner();scanning=1;
    uint32_t result=0;
    for(unsigned seat=0;seat<2;++seat) {
        if(!actors[seat].active || (owner>=0 && owner!=(int)seat))continue;
        copy_camera(CAMERA,state+16u+seat*CAMERA_BYTES);
        result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    }
    write_bytes(CAMERA,canonical,CAMERA_BYTES);
    /* A paired doorway can finish with the next door already inside a
     * stationary view, missing retail's moving-edge spawn strips. Include
     * unflagged General8 doors in the bounded full-view scan above; native
     * eligibility still owns admission, and shared visits prevent copies.
     * Other scripted General actors retain directional-only activation. */
    supplemental=1;
    for(unsigned region=0;region<count;++region)scan_rectangle(cpu,regions[region]);
    supplemental=scanning=0;
    psx_mod_counter_add("mmx4.coop.split-placement-scans",count);
    return mmx4_coop_finish(cpu,result);
}
static int actor_bounds(CPUState *cpu,uint32_t address) {
    if(address==0x8002B160u && mmx4_coop_lifecycle_assessment_bounds(cpu,address))return 1;
    uint32_t actor=cpu->gpr[4];
    if(bounds_call || !available() || (int8_t)psx_mod_read_byte(actor+0x14u)<0)return 0;
    uint32_t native=address;
    unsigned draw=address!=0x8002B1E8u && address!=0x8002B160u;
    if(address==0x8002B160u) {native=0x8002B1E8u;cpu->gpr[5]=cpu->gpr[6]=64;}
    if(address==0x8002B288u || address==0x8002B3C0u) {
        native=0x8002B318u;cpu->gpr[5]=address==0x8002B288u?32:96;
        cpu->gpr[6]=address==0x8002B288u?32:80;
    }
    unsigned layer=psx_mod_read_byte(actor+0x14u);if(layer>=3)return 0;
    initialize_cameras();
    uint8_t canonical[CAMERA_BYTES];read_bytes(CAMERA,canonical,CAMERA_BYTES);
    Mmx4CoopViewActor actors[2];mmx4_coop_split_actors(actors);
    int owner=mmx4_coop_lifecycle_paired_door()?-1:mmx4_coop_lifecycle_script_owner();
    unsigned visible=0;
    uint32_t result=draw?0:1;bounds_call=1;
    for(unsigned seat=0;seat<2;++seat) {
        if(!actors[seat].active || (owner>=0 && owner!=(int)seat))continue;
        copy_camera(CAMERA,state+16u+seat*CAMERA_BYTES);
        uint32_t candidate=mmx4_coop_call(cpu,native,actor,cpu->gpr[5]);
        if(draw) {
            if(psx_mod_read_byte(actor+3u)) {visible=1;result=candidate;}
        }else if(!candidate)result=0;
    }
    if(draw)psx_mod_write_byte(actor+3u,(uint8_t)visible);
    write_bytes(CAMERA,canonical,CAMERA_BYTES);bounds_call=0;
    return mmx4_coop_finish(cpu,result);
}
static int frost_ice_range(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!available() || psx_mod_read_byte(MMX4_PLAY+12)!=2 ||
       psx_mod_read_byte(MMX4_PLAY+13))return 0;
    initialize_cameras();
    /* B9E54 is a pure proximity query for the opening ice tile sequences.
     * Use its exact unsigned16 extents for each living view. The shared
     * cycle keeps its native clock, allocation mask and tile-edit queue. */
    Mmx4CoopViewActor actors[2];mmx4_coop_split_actors(actors);
    uint32_t point=cpu->gpr[4];
    int owner=mmx4_coop_lifecycle_script_owner();
    for(unsigned seat=0;seat<2;++seat) {
        if(!actors[seat].active || (owner>=0 && owner!=(int)seat))continue;
        unsigned layer=seat?mmx4_coop_second_body()[0x14]:
            psx_mod_read_byte(MMX4_PLAYER+0x14);
        if(layer>=3)continue;
        uint32_t camera=state+16u+seat*CAMERA_BYTES+layer*0x54u;
        uint16_t x=(uint16_t)(psx_mod_read_half(point)-psx_mod_read_half(camera+10)+640u);
        uint16_t y=(uint16_t)(psx_mod_read_half(point+2)-psx_mod_read_half(camera+14)+480u);
        if(x<1600u && y<1200u)return mmx4_coop_finish(cpu,1);
    }
    return mmx4_coop_finish(cpu,0);
}
PSX_MOD_CONSTRUCTOR(mmx4_register_coop_split) {
    psx_mod_register_function_filter_plugin(COOP_ID,0x800B9E54u,frost_ice_range);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800257BCu,draw_armor);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800B7078u,mushroom_stage_camera);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800BA57Cu,mushroom_stage_camera);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800D8648u,moving_background);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800D9218u,moving_background);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800DA298u,moving_background);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800D964Cu,train_foreground);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800C0D98u,destructible_camera_unlock);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800C3880u,mushroom_lift_camera);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800D8ED4u,jungle_background);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800B75C8u,frost_snow_background);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800B56F4u,spatial_stage_actor);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800B6A0Cu,spatial_stage_actor);
    psx_mod_register_function_entry_plugin(COOP_ID,0x80028DB4u,reset_placements);
    psx_mod_register_function_filter_plugin(COOP_ID,0x80028E24u,scan_players);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800293E8u,placement_allocate);
    psx_mod_register_instruction_plugin(COOP_ID,0x80029284u,0xA2620000u,placement_spawned);
    psx_mod_register_function_filter_plugin(COOP_ID,0x8002B1E8u,actor_bounds);
    psx_mod_register_function_filter_plugin(COOP_ID,0x8002B160u,actor_bounds);
    psx_mod_register_function_filter_plugin(COOP_ID,0x8002B318u,actor_bounds);
    psx_mod_register_function_filter_plugin(COOP_ID,0x8002B288u,actor_bounds);
    psx_mod_register_function_filter_plugin(COOP_ID,0x8002B3C0u,actor_bounds);
}

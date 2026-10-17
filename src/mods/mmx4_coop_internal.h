#ifndef MMX4_COOP_INTERNAL_H
#define MMX4_COOP_INTERNAL_H
#include <stdint.h>
#include "cpu_state.h"
#include "mmx4_coop_snapshot.h"

void mmx4_coop_audio_reset(void);
int mmx4_coop_audio_activate(void);
int mmx4_coop_audio_prepare(unsigned character,const uint8_t *file,uint32_t size,int initialize_guest);
int mmx4_coop_audio_load(unsigned character,const uint8_t *file,uint32_t size);
int mmx4_coop_audio_snapshot_ready(void);
int mmx4_coop_audio_snapshot(Mmx4SnapshotIO *s);
int mmx4_coop_lifecycle_snapshot_ready(void);
int mmx4_coop_lifecycle_snapshot(Mmx4SnapshotIO *s);
int mmx4_coop_combat_snapshot_ready(void);
int mmx4_coop_combat_snapshot(Mmx4SnapshotIO *s);
int mmx4_coop_web_snapshot_ready(void);
int mmx4_coop_web_snapshot(Mmx4SnapshotIO *s);
int mmx4_coop_split_snapshot_ready(void);
int mmx4_coop_split_snapshot(Mmx4SnapshotIO *s);
int mmx4_widescreen_snapshot_ready(void);
int mmx4_widescreen_snapshot(Mmx4SnapshotIO *s);
#include "mod_plugins.h"
#include "mmx4_coop_views.h"
#define MMX4_PLAYER 0x801418C8u
#define MMX4_PLAY 0x801721C0u
#define MMX4_VEHICLE 0x80173A30u
/* Private runtime interface. Projection is strictly scoped and non-nesting;
 * callers check projected() before intercepting a native routine. */
int mmx4_coop_ready(void);
void mmx4_coop_development_mask_pad(void);
int mmx4_coop_projected(void);
int mmx4_coop_split_views(void);
int mmx4_coop_both_door_players(void);
void mmx4_coop_bosses_activate(void);
void mmx4_coop_bosses_new_stage(void);
void mmx4_coop_bosses_stage_initialization(void);
int mmx4_coop_bosses_snapshot_ready(void);
int mmx4_coop_bosses_snapshot(Mmx4SnapshotIO *s);
uint32_t mmx4_coop_bosses_digest(uint32_t seed);
uint32_t mmx4_coop_counterpart_story_text(void);
void mmx4_coop_balance_activate(void);
void mmx4_coop_balance_reset(void);
void mmx4_coop_balance_forget(uint32_t actor);
void mmx4_coop_balance_begin(uint32_t actor);
void mmx4_coop_balance_end(void);
int mmx4_coop_balance_hit_allowed(uint32_t actor,unsigned seat);
int mmx4_coop_balance_snapshot_ready(void);
int mmx4_coop_balance_snapshot(Mmx4SnapshotIO *s);
uint32_t mmx4_coop_balance_digest(uint32_t seed);
int mmx4_coop_split_activate(void);
void mmx4_coop_split_reset(void);
uint32_t mmx4_coop_split_digest(uint32_t seed);
void mmx4_coop_split_camera(CPUState *cpu,unsigned owner);
void mmx4_coop_split_camera_prepare(unsigned owner);
int mmx4_coop_split_scene_camera_begin(unsigned owner,uint8_t canonical[0xFC]);
void mmx4_coop_split_scene_camera_end(unsigned owner,const uint8_t canonical[0xFC],int committed);
void mmx4_coop_split_actors(Mmx4CoopViewActor actors[2]);
int mmx4_coop_split_camera_copy(unsigned seat,uint8_t layers[0xFC]);
int mmx4_coop_split_sprite_outside(uint32_t actor);
int mmx4_coop_render_rider_armor(CPUState *cpu,uint32_t address);
void mmx4_coop_split_project_intro_boss(void);
void mmx4_coop_split_flip_cameras(void);
void mmx4_coop_flip_second(void);
unsigned mmx4_coop_split_view_seat(unsigned seat);
void mmx4_coop_lifecycle_view_save(void);
void mmx4_coop_lifecycle_view_restore(void);
void mmx4_coop_lifecycle_surface(CPUState *cpu);
void mmx4_widescreen_view_save(void);
void mmx4_widescreen_view_restore(void);
int mmx4_coop_alive(unsigned seat);
uint8_t *mmx4_coop_second_body(void);
uint8_t *mmx4_coop_first_body(void);
uint8_t *mmx4_coop_second_vehicle(void);
uint8_t *mmx4_coop_first_vehicle(void);
void mmx4_coop_enter_second(void);
void mmx4_coop_leave_second(void);
int mmx4_coop_web_activate(void);
void mmx4_coop_web_reset(void);
void mmx4_coop_web_forget(uint32_t actor,unsigned owner);
uint32_t mmx4_coop_web_digest(uint32_t seed);
int mmx4_coop_web_busy(void);
int mmx4_coop_web_contact(CPUState *cpu,uint32_t address);
uint32_t mmx4_coop_call(CPUState *cpu,uint32_t address,uint32_t a0,uint32_t a1);
int mmx4_coop_finish(CPUState *cpu,uint32_t result);
uint16_t mmx4_coop_input(unsigned seat);
void mmx4_coop_menu_assets(CPUState *cpu,unsigned seat);
void mmx4_coop_combat_reset(void);
unsigned mmx4_coop_vehicle_effect_owner(uint32_t actor);
void mmx4_coop_combat_project_end(uint32_t vehicle_mirror);
/* Current projection's native Ride Armor; Chasers are deliberately excluded. */
int mmx4_coop_armor_mounted(void);
void mmx4_coop_armor_dismount(CPUState *cpu,unsigned seat);
int mmx4_coop_combat_actor_context(void);
int mmx4_coop_combat_canonical_call(CPUState *cpu,uint32_t address,
    PSXModFunctionFilterCallback callback);
uint8_t mmx4_coop_solid_contact_bits(uint32_t actor,unsigned seat);
/* Implemented by lifecycle module, called around every private projection. */
void mmx4_coop_lifecycle_project(void);
void mmx4_coop_lifecycle_follow_progress(uint8_t *body,unsigned character);
void mmx4_coop_lifecycle_restore(void);
void mmx4_coop_lifecycle_reset(void);
void mmx4_coop_lifecycle_activate(void);
void mmx4_coop_lifecycle_enrolled(CPUState *cpu);
int mmx4_coop_lifecycle_can_enroll(void);
void mmx4_coop_lifecycle_tick(CPUState *cpu);
int mmx4_coop_lifecycle_can_tick(void);
/* -1 during ordinary play; otherwise the native shared script's seat. */
int mmx4_coop_lifecycle_script_owner(void);
/* Both players retain their own views during an unfinished paired door. */
int mmx4_coop_lifecycle_paired_door(void);
int mmx4_coop_lifecycle_hidden(unsigned seat);
int mmx4_coop_lifecycle_arriving(unsigned seat);
int mmx4_coop_lifecycle_initial_arrival(void);
int mmx4_coop_lifecycle_nova_active(void);
/* A projected portal dispatch defers its shared collision until restoration. */
int mmx4_coop_lifecycle_defer_portal_solid(uint32_t actor);
/* Shared Mushroom lift admission; returns a handled native result in v0. */
int mmx4_coop_lifecycle_lift_start(CPUState *cpu,uint32_t address);
void mmx4_coop_lifecycle_room_entry(unsigned owner);
void mmx4_coop_lifecycle_camera_bounds(CPUState *cpu,unsigned owner);
void mmx4_coop_clear_current_attacks(void);
void mmx4_coop_combat_clear_current_effects(void);
/* Current projected player and vehicle, at enrollment or incoming handoff. */
void mmx4_coop_spawn_facing_right(void);
uint32_t mmx4_coop_lifecycle_digest(uint32_t seed);
uint32_t mmx4_coop_combat_digest(uint32_t seed);
#endif

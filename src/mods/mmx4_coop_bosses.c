/* Optional story encounters. The campaign byte always belongs to P1;
 * stage bank selectors, encounter admission and dialogue use the other
 * story during the second round without changing that campaign byte. */
#include "mmx4_coop_internal.h"
#include <string.h>
#include <stdio.h>

#define ID "mmx4.coop"
#define PLAY MMX4_PLAY
static unsigned both_stories,second_round,defeated,reloading;

static int story_room(void) {
    return psx_mod_read_byte(PLAY+0x0C)==11 && !psx_mod_read_byte(PLAY+0x0D);
}
static unsigned story_character(void) {
    return psx_mod_read_byte(PLAY+0x43) ^ second_round;
}
void mmx4_coop_bosses_activate(void) {
    char value[8];
    both_stories=psx_mod_current_option_value("both_story_bosses",value,sizeof value) &&
        !strcmp(value,"true");
    second_round=defeated=reloading=0;
    if(both_stories)psx_mod_counter_add("mmx4.coop.both-story-bosses-enabled",1);
}
int mmx4_coop_bosses_snapshot_ready(void) {return 1;}
int mmx4_coop_bosses_snapshot(Mmx4SnapshotIO *s) {
    mmx4_snapshot_fixed(s,both_stories);
    MMX4_SNAPSHOT_BOOL(s,second_round);MMX4_SNAPSHOT_BOOL(s,defeated);
    MMX4_SNAPSHOT_BOOL(s,reloading);
    return s->ok;
}
uint32_t mmx4_coop_bosses_digest(uint32_t seed) {
    seed=(seed^both_stories)*16777619u;
    seed=(seed^second_round)*16777619u;
    seed=(seed^defeated)*16777619u;
    return (seed^reloading)*16777619u;
}
void mmx4_coop_bosses_new_stage(void) {
    if(!mmx4_coop_projected() && !psx_mod_local_view_scope() && !(reloading && story_room()))
        second_round=defeated=reloading=0;
}
void mmx4_coop_bosses_stage_initialization(void) {
    if(!reloading || !story_room() || mmx4_coop_projected() || psx_mod_local_view_scope())return;
    psx_mod_write_byte(PLAY+0x1D,1);
    psx_mod_write_byte(PLAY+0x1E,0);
    reloading=0;
}
static void select_story(CPUState *cpu,uint32_t address) {
    if(!both_stories || !second_round || !story_room() || psx_mod_local_view_scope())return;
    if(address==0x80022554u) {
        uint32_t text=mmx4_coop_counterpart_story_text();
        if(text)cpu->gpr[4]=text;
    }else {
        unsigned reg=2;
        if(address==0x800224D4u || address==0x80022928u)reg=4;
        if(address==0x80022880u || address==0x800228B4u)reg=3;
        cpu->gpr[reg]=story_character();
    }
}
static void story_defeated(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!both_stories || !story_room() || !mmx4_coop_ready() || psx_mod_local_view_scope())return;
    defeated=1;
    psx_mod_counter_add("mmx4.coop.story-boss-defeated",1);
    /* Finish with P1's original post-battle movie/story route even when
     * the second fight was the other character's encounter. */
    if(second_round)cpu->gpr[2]=psx_mod_read_byte(PLAY+0x43)?0x40u:1u;
}
static int story_transition(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!both_stories || second_round || !defeated || !story_room() ||
       !mmx4_coop_ready() || mmx4_coop_projected() || psx_mod_local_view_scope() ||
       !(psx_mod_read_byte(PLAY+0x0F)&0x7Fu))return 0;
    second_round=1;defeated=0;reloading=1;
    /* Schedule the ordinary loader; CD loading can suspend and must never
     * run recursively inside a synchronous host callback. The subsequent
     * stage initializer selects the verified boss-door checkpoint. */
    psx_mod_write_byte(PLAY+0x0F,0);
    psx_mod_write_word(PLAY,4);
    fprintf(stderr,"mmx4.coop: story-sequel campaign=%u mode=%u checkpoint=%u lives=%u\n",
        psx_mod_read_byte(PLAY+0x43),psx_mod_read_byte(PLAY),
        psx_mod_read_byte(PLAY+0x1D),psx_mod_read_byte(PLAY+0x44));
    psx_mod_counter_add("mmx4.coop.second-story-encounter",1);
    return mmx4_coop_finish(cpu,0);
}
PSX_MOD_CONSTRUCTOR(mmx4_register_coop_bosses) {
    psx_mod_register_function_filter_plugin(ID,0x800200D4u,story_transition);
    psx_mod_register_instruction_plugin(ID,0x800130D4u,0x00220821u,select_story);
    psx_mod_register_instruction_plugin(ID,0x80013148u,0x14400005u,select_story);
    psx_mod_register_instruction_plugin(ID,0x80080714u,0x14400005u,select_story);
    psx_mod_register_instruction_plugin(ID,0x8008BA54u,0x10400005u,select_story);
    /* The script text and portrait/voice indices also belong to the current
     * encounter, independent of the campaign or the player speaking. */
    psx_mod_register_instruction_plugin(ID,0x800224D4u,0x00041880u,select_story);
    psx_mod_register_instruction_plugin(ID,0x80022554u,0x00641821u,select_story);
    psx_mod_register_instruction_plugin(ID,0x80022684u,0x14400009u,select_story);
    psx_mod_register_instruction_plugin(ID,0x800227B4u,0x00021080u,select_story);
    psx_mod_register_instruction_plugin(ID,0x8002284Cu,0x00021080u,select_story);
    psx_mod_register_instruction_plugin(ID,0x80022880u,0x00431021u,select_story);
    psx_mod_register_instruction_plugin(ID,0x800228B4u,0x00431021u,select_story);
    psx_mod_register_instruction_plugin(ID,0x800228F4u,0x2442001Au,select_story);
    psx_mod_register_instruction_plugin(ID,0x80022928u,0x0C00A4E3u,select_story);
    psx_mod_register_instruction_plugin(ID,0x800810A8u,0xA02221CFu,story_defeated);
    psx_mod_register_instruction_plugin(ID,0x8008D324u,0xA02221CFu,story_defeated);
}

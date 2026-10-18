/* Course analysis is a single native gameplay actor. Prepare a second drawn
 * instance without calling its update, grading clock or completion producer. */
#include "mmx4_coop_internal.h"
#include "mmx4_coop_memory.h"

static uint32_t word(const uint8_t *p) {
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static void put(uint8_t *p,uint32_t value) {
    for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(value>>(8u*i));
}
int mmx4_coop_assessment_copy(uint32_t actor,uint8_t copy[0x60]) {
    if(!copy || !mmx4_coop_ready() || mmx4_coop_projected() ||
       psx_mod_read_byte(MMX4_PLAY)!=6 || psx_mod_read_byte(MMX4_PLAY+12)!=6 ||
       psx_mod_read_byte(MMX4_PLAY+13) || mmx4_coop_lifecycle_script_owner()>=0 ||
       !mmx4_coop_alive(0) || !mmx4_coop_alive(1) ||
       actor<0x80173CA0u || actor>=0x801754A0u || (actor-0x80173CA0u)%0x60u ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=34 ||
       psx_mod_read_byte(actor+4)!=1 || psx_mod_read_byte(actor+5)>2 ||
       psx_mod_read_byte(actor+0x14)>=3)return 0;
    uint32_t parent=psx_mod_read_word(actor+0x50);
    if(parent<0x80142F98u || parent>=0x80143598u || (parent-0x80142F98u)%0x30u ||
       !psx_mod_read_byte(parent) || psx_mod_read_byte(parent+1)!=38)return 0;
    const uint8_t *second=mmx4_coop_second_body();
    if(second[0x14]>=3)return 0;
    mmx4_coop_capture(actor,copy,0x60);
    uint32_t dx=word(second+8)-psx_mod_read_word(MMX4_PLAYER+8);
    uint32_t dy=word(second+12)-psx_mod_read_word(MMX4_PLAYER+12);
    /* Retain the native scan offset and interpolation history. Unsigned
     * fixed-point addition has the original guest's wrapping semantics. */
    put(copy+8,word(copy+8)+dx);put(copy+12,word(copy+12)+dy);
    put(copy+0x18,word(copy+0x18)+dx);put(copy+0x1C,word(copy+0x1C)+dy);
    copy[0x14]=second[0x14];
    return 1;
}

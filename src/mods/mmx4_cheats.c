/* Original SLUS-00561 character codes, independently selected by trusted
 * packages or an agreed co-op profile. X still obtains Ultimate Armor from
 * a native capsule; Zero uses the original palette13 with ordinary stats.
 * PLAY+37 remains P1's sole campaign flag. The co-op projection owns its
 * temporary P2 value and restores the canonical byte after each guest pass. */
#include "mmx4_cheats.h"
#include "mmx4_coop_internal.h"
#include "mod_plugins.h"
#include "mod_runtime.h"
#include "mod_snapshot.h"
#include "cpu_state.h"

#define PLAY 0x801721C0u
#define ULTIMATE_PACKAGE "mmx4.cheat.ultimate-armor"
#define BLACK_PACKAGE "mmx4.cheat.black-zero"
#define COOP_PACKAGE "mmx4.coop"

static unsigned coop_choices[2];
static unsigned selector_call;

void mmx4_cheats_configure_coop(int ultimate_x, int black_zero) {
    coop_choices[0] = ultimate_x != 0;
    coop_choices[1] = black_zero != 0;
    /* Activation is quiescent. A previous session may have soft-returned
     * from a guest call, abandoning its host stack without unwinding it. */
    selector_call = 0;
}

unsigned mmx4_cheats_character_enabled(unsigned character) {
    if (character > 1) return 0;
    const char *package = character ? BLACK_PACKAGE : ULTIMATE_PACKAGE;
    return mod_runtime_snapshot_package_active(package) ||
        (mod_runtime_snapshot_package_active(COOP_PACKAGE) && coop_choices[character]);
}

int mmx4_cheats_snapshot_ready(void) { return !selector_call; }

static int projected_second(void) {
    /* A disabled old session can leave transient title statics until its
     * next activation. Only the committed co-op package owns projections. */
    return mod_runtime_snapshot_package_active(COOP_PACKAGE) && mmx4_coop_projected();
}

void mmx4_cheats_enforce_current(void) {
    /* Actor/story passes sometimes keep PLAY+43 canonical while PLAYER names
     * P2. enter_second's scoped byte is authoritative in every such pass. */
    if (projected_second()) return;
    unsigned character = psx_mod_read_byte(PLAY + 0x43);
    if (mmx4_cheats_character_enabled(character))
        psx_mod_write_byte(PLAY + 0x37, 1);
    /* Preserve original manually entered codes when no option forces one.
     * Do not grant armor or write an additional campaign/save record. */
}

static void enforce_entry(CPUState *cpu, uint32_t address) {
    (void)cpu; (void)address;
    mmx4_cheats_enforce_current();
}

static int select_character(CPUState *cpu, uint32_t address) {
    if (selector_call || projected_second() || (!mmx4_cheats_character_enabled(0) &&
                          !mmx4_cheats_character_enabled(1))) return 0;
    selector_call = 1;
    uint32_t result = psx_mod_call_guest(cpu, address, cpu->gpr[31],
        cpu->gpr[4], cpu->gpr[5], cpu->gpr[6], cpu->gpr[7]);
    /* 80029858 clears the common code when the selected character changes.
     * Apply its own option after that original selection/recognizer body. */
    mmx4_cheats_enforce_current();
    selector_call = 0;
    cpu->gpr[2] = result;
    return 1;
}

static void activate_standalone(void) {
    /* Do not clear agreed choices: package activation order is unspecified.
     * Choices have an independent active-package gate in character_enabled. */
    selector_call = 0;
}

/* There are no standalone mutable gameplay fields beyond the native flag in
 * guest RAM. A constant provider records the configuration and supplies a
 * readiness boundary while the original selector runs on a live C stack. */
static uint32_t snapshot_bytes(void) { return 1; }
static int snapshot_write(uint8_t *out, uint32_t len) {
    if (!out || len != 1 || !mmx4_cheats_snapshot_ready()) return 0;
    out[0] = 1;
    return 1;
}
static int snapshot_validate(const uint8_t *data, uint32_t len) {
    return data && len == 1 && data[0] == 1;
}
static void snapshot_read(const uint8_t *data, uint32_t len) {
    (void)data; (void)len;
}

PSX_MOD_CONSTRUCTOR(mmx4_register_cheats) {
    static const char *const plugins[] = {
        "mmx4.ultimate_armor", "mmx4.black_zero", "mmx4.coop"
    };
    static const uint32_t entries[] = {
        0x80012EB8u, /* X normal/Ultimate or Zero ARC selection */
        0x80035240u, /* Native body/armor initialization */
        0x800361F8u, /* Zero palette13 selection */
        0x800C62DCu, /* Native capsule eligibility */
        0x800C6EDCu, /* Native capsule grant/animation */
        0x800375B4u  /* Native Nova Strike/energy policy */
    };
    static const PSXModSnapshotProvider provider = {
        1, snapshot_bytes, snapshot_write, snapshot_validate, snapshot_read,
        mmx4_cheats_snapshot_ready
    };
    for (unsigned p = 0; p < sizeof plugins / sizeof plugins[0]; ++p) {
        for (unsigned i = 0; i < sizeof entries / sizeof entries[0]; ++i) {
            /* One plugin owns one callback per address. The core owns these
             * initialization/capsule hooks and invokes the helper explicitly
             * after it has selected the correct canonical owner. */
            if (p == 2 && (entries[i] == 0x80035240u || entries[i] == 0x800C62DCu ||
                           entries[i] == 0x800C6EDCu)) continue;
            psx_mod_register_function_entry_plugin(plugins[p], entries[i], enforce_entry);
        }
        psx_mod_register_function_filter_plugin(plugins[p], 0x800297D8u, select_character);
    }
    psx_mod_register_activation_plugin(plugins[0], activate_standalone);
    psx_mod_register_activation_plugin(plugins[1], activate_standalone);
    psx_mod_register_snapshot_provider(ULTIMATE_PACKAGE, &provider);
    psx_mod_register_snapshot_provider(BLACK_PACKAGE, &provider);
}

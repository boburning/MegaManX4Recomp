#ifndef MMX4_CHEATS_H
#define MMX4_CHEATS_H

/* Native character numbers: 0 = X, 1 = Zero. Configure once at co-op
 * activation, before guest entry. Choices are ignored without active co-op. */
void mmx4_cheats_configure_coop(int ultimate_x, int black_zero);
unsigned mmx4_cheats_character_enabled(unsigned character);
int mmx4_cheats_snapshot_ready(void);
/* Refresh P1's native flag from active choices. Scoped P2 projections already
 * own the exact flag value and must not be changed by a campaign selector. */
void mmx4_cheats_enforce_current(void);
void mmx4_cheats_restore(void);

#endif

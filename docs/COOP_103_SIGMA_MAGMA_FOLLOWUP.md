# 1.0.5 Sigma, Magma and opening-stage follow-up

Reports and recordings: Thomu, supplied by the owner on 2026-10-23.
Thanks also to Moro and JMOTZU for the continuing co-op playtests.

## Corrections and native checks

All runs use the verified, unmodified USA disc, private blank/cloned memory
cards, hidden windows and muted output. Checkpoint and positioning fixtures
bypass travel prerequisites; they never set a boss victory, door completion,
movie completion or scene-success flag. Captures marked `composed` show both
local Split panes. Ordinary screenshots alone do not qualify both panes.

| Report | Result in the 1.0.5 candidate | Tracking |
|---|---|---|
| Sigma 1: Nova Strike hides the waiting partner's door | Reproduced and fixed. Native Nova exercised with X in either seat; waiting door stays visible. | beads-dwp1 |
| Sigma 1: second door disappears from the vestibule | Visible after entry in both campaign/leader controls. The reported disappearance was intermittent. | beads-dwp1 |
| Sigma 1: stairs displace the partner and lock the camera | Reproduced and fixed. Real return-strip crossing, retreat, re-entry, both doors and a live Double fight pass in both campaigns with opposite leaders. | beads-dwp1 |
| Magma: P2 arena entry pans from distant P1, corrupts scenery and loses retry checkpoint | Upper and lower Ride Armor admissions pass: camera begins beside the entering hero, both heroes remain visible, native Dragoon fight remains active, checkpoint 3 is committed. Actual team death/retry returns both to the near-boss checkpoint. Lower-route rocks were bypassed to isolate admission. | beads-qj05 |
| Magma part 1: Giga Death missing for P2 | Original type 51 visibly spawns with either leader from the authored checkpoint/approach. No additional Giga Death spawn patch is claimed. | beads-qj05 |
| Magma fireballs, platform impacts and shaking missing for P2 | Native emitters use the triggering hero's view; personal shake is limited to the nearby view. Earlier bounded native effect checks pass. | beads-e0b2 |
| Opening dragon corner rendered in P1's view | Actor, attachments and projectile ownership corrected; fresh release generation includes both 99634/99650 instruction hooks. P2 corner, return and current-build snapshot controls pass. | beads-78sc |
| Opening part 2 flying bee groups missing | All three falling-object emitter variants produce their original flying groups with either leader. Native allocation PCs and drawable actors are recorded; composed P2 captures show the flying groups while P1 waits behind. No new bee spawn rule was needed. | beads-eio.9.197 |
| Mushroom elevator death while deliberately crowding the blue obstacles | Original crush contact reproduced at the panel edge. No change to native crush rules. | beads-5k3q |
| One-off Magma process exit | Still unreproduced. Supplied log ends without a fatal/dispatch/watchdog cause. Not claimed fixed. | beads-v7yu |
| Magma part 2 idle slowdown | Not reproduced in the bounded current-host control using the supplied 1080p video settings: approximately 59.6 FPS over 14 seconds. No performance fix or cross-release performance conclusion is claimed. | beads-pu0l |
| Intermittent frame stutters | Withdrawn by the owner; not claimed fixed. | beads-pu0l |

## Mechanisms

Sigma 1 originally copied paired-door room bounds between the cameras every
frame. The trailing hero's stair-return strip could replace the partner's room
maximum (4160 with 3776), clamping the hero in the vestibule back to 4090.
The door now publishes its original enclosure once. Return strips remain local;
a hero who returns to the original admission box receives that enclosure again.
Door state and its enclosure participate in current-build snapshots and digests.

Native Nova sets a personal attack lock in `PLAY+1C`. The ordinary scene-camera
scope rejected that lock and culled the partner's waiting door against P1's
distant camera (native write 2B298 from C1994). Only an active native Nova
permits that scope; real scene/health freezes retain their existing guards.

Magma's original 7DF50 boss admission used the canonical P1 camera when P2
entered. The entering hero's camera now scopes that one admission and commits
only when the shared script selects that hero. Fight targeting and refights
retain the existing behavior. Both entry routes show intact arena scenery.

Sigma 2 reuses type 36 cannon beams already corrected for earlier stages. Its
stage guard now admits that same family in stage 11. Parent/pool/type checks
remain. Allocation, collision, animation and lifetime still run natively once.
Both beam directions stay beside the actual emitters (world X823..1344), with
either leader and either campaign; the waiting player's pane stays clear.

## Evidence and exclusions

Private evidence root: `G:/Temp/Codex/psxrecomp/x4-release105/`.
Qualified collections include `sigma-qualified-x`, `movies-en-zero` (Sigma
controls), `magma-fresh-upper`, `magma-warp-probe`,
`magma-lower-admission-final`, `native-gateways-x/continued-probes`, and
`bees-final`. The earlier before captures are under
`F:/Temp/Codex/psxrecomp/x4-sigma-magma-103/`.

Invalid early fixtures are excluded: unsupported checkpoint 3 in Sigma 1;
teleports that bypassed the tower camera strips; premature Ride Armor boarding
or destinations inside solid/lava terrain; and a development armor fixture that
also marked all bosses cleared. That fixture now grants armor without boss
clears. Bee trace filtering was corrected to retain the relevant native PCs;
a full trace buffer is not a missing-enemy result.

The original boot EXE matches `SLUS_005.61;1` read from the verified USA disc.
SHA-256: `dae43d44a9f3176208830ddbba0607bb627ebcd7faf9b36fc03dd0ffda34d2e8`.
No modified base-disc support is added. This is bounded regression coverage,
not a newly completed full campaign or a claim that every older report is fixed.

A fresh Windows same-host two-peer Split run with simulated latency/jitter
passes input-only boot, both players' controls, own-view render isolation,
independent menu ownership/freeze, repeated matching core/mod CRC checkpoints
through tick 3360 and clean disconnect. Both own-view HUD captures were
inspected. This does not establish cross-OS netplay or fan-movie netplay; the
current trusted online profile does not activate offline language/movie mods.

# Playtest 41 follow-up

Intake on October 19. These reports remain open until the corresponding
native behavior is qualified. No Playtest 42 release is claimed here.

| Item | Current evidence |
|---|---|
| 189: Slash Beast scenery freezes for P2 | **Fixed candidate, native qualified in both sections.** Native P2 jumps reproduce a private-view panorama wrap being discarded: its origin keeps growing, so every subsequent draw resets to the same scenery. Retain the authored integer wrap in the saved planes and owner writeback. Both near/far layers now remain animated in native local8:3 checks with separated, living heroes. The short phone clips were inspected. This is additional to the canonical scrolling correction shipped in41. |
| 198: intro part 1 shake missing for nearby P2 | Tester reconfirms in 41. Preserve intentional isolation of distant explosion flashes. |
| 199: first intro part 2 block | **Tester confirms fixed in 41.** No new change requested. |
| 200: intro part 2 bees missing | Tester still reports missing bees. A bounded native P2-only approach does spawn the static pair and a later type7 at703/332 while P1 stays144. This does not qualify all later flying groups. |
| 206: Magma Dragoon refight camera | **Fixed candidate, native qualified with either initiator.** The exact Playtest41 replay verifies all 13,666 native inputs. The native phase 2-to-3 release writes horizontal targets after releasing the scene. Preserve those completed writes in each saved view still in the closed arena. Both views now pan during the active fight, with either P1 or P2 initiating. Earlier arrival checks did not cover this event. |
| 208: P2 pause ownership and frozen controls | **Fixed candidate, native qualified.** Exact published41 replay verifies all12,705 native inputs. A native two-confirmation Quit, new game and Slash reload independently reproduce wrong ownership and frozen P2. Retire the old callback guard on stage initialization and reconcile the menu latch on ordinary gameplay entry. The same fixed-build flow opens Zero's own menu, rejects P1 resume/P2 Quit, resumes with P2 Start and moves P2 by61.5px while P1 remains still, both32HP. |

Both archives identify the actual version as `0.1.1-coop-playtest.41`, offline
`local_split_8_3`, Both doors, per-player boss cooldown scaling, separate
E-tanks and both story bosses enabled. The new mode metadata identifies these
without asking the tester. The Dragoon archive includes the earlier two
sessions plus a new13,667-input fortress session; duplicated sessions are not
independent reproductions.

Archive SHA256:

- `Bugs 41.zip`: `c65c20db555ed00d248e7c86951c972563ae40366137d1594756fb0db22e4f09`
- `Dragoon cam bug 41.zip`: `0ee39dc81b53af8fbf79b7dac8080c5b1fd152a86945d2353e2c48ffc8214562`

Private evidence is under `build-pr23/feedback42/`. `exact41-replay/replay.log`
and both `inspect-bugs41*/replay.log` files preserve complete replay validation.
The additional observations read the shipped process; they do not patch the
executable, change its input stream or relax replay identity/hash checks.
The first inspector's camera/mirror reads incorrectly folded expansion-memory
addresses into main RAM: exclude those fields. Its host menu/body observations
remain usable. Subsequent inspections map the expansion arena explicitly.

For208, `menu-boundary-result.json` records a regression that fails the old
source and passes the candidate. The pause-only candidate's complete Windows
36-test suite passes; final combined results are recorded below.
Native before evidence is `quit-native-before/observations.json`; fixed evidence
is `workshop-fixed/full-quit-fixed-2/observations.json` and its result receipt.
The fixed private inspector retains production generated simulation objects
and enables TCP polling/fixtures in separate host objects; it is not a release
artifact or a network-compatibility qualification. Its failed link preparations
and early Quit-page probes are excluded. The final native check waits for both
confirmation pages to be interactive instead of sending buttons during fades.

For 206, `dragoon-boundary-result.json` records old-source failures and candidate
passes in both the actor-update and saved-camera tests. Native before and after
receipts are `workshop-fixed/dragoon-p{1,2}-before-result.json` and
`workshop-camera-fixed/dragoon-p{1,2}-fixed-result.json`. Before: a P1 initiation
leaves P2 at X432; a P2 initiation leaves both at X432. After: both have native
target bounds272..432 and follow to X272..286 while live boss AI continues,
with both players32HP. The composed pair images were inspected. Only target
bounds and follow tuning change; origins, current limits, unrelated rooms and
native camera easing are preserved. This qualification covers local8:3 split
with both initiating players, not a fresh online or cross-OS match.

Linux GCC runs of the lifecycle, combat and split model tests all pass
(`linux-followup-result.json`).

For 189, the corrected read-only inspection also verifies every one of the
12,705 published inputs (`inspect-slash41/replay.log`). After P2 moves ahead,
its saved near/far origins grow beyond the authored panorama. The native
273F8..274CC draw code resets the integer X to the opposite screen, but the
private render transaction rolls that reset back. The same out-of-range
saved position therefore produces the same reset on every subsequent draw.
The new helper retains that native wrap in simulation before the next private
draw; the P2 scene transaction must retain it again on exit. It changes no
foreground position, fractional X, world collision or shared train clock.

`train-wrap-boundary-result.json` fails without those retained wraps and
passes with them through600 ticks for either controller owner. Both native
sections are then checked with native P2 jumps and20-second observation
windows: `workshop-scenery-fixed/slash-native-long-fixed-result.json` and
`slash-part2-fixed-result.json`. Both heroes stay32HP and the composed pair
captures show both panoramas changing after repeated wraps. This is a
scrolling qualification, not a complete train-event or online playthrough.
The earlier `slash-long-before` direct-position setup killed P2; its duplicated
survivor views are explicitly excluded. The valid before reproduction is
`workshop-scenery/slash-native-long-before/`.

The combined production Windows executable builds with developer tools off.
No private inspector build is offered as a compatible release or replaces the
published Playtest41 package. Final suite receipt: `ctest-final.log`.

Fresh debug pause/resume and early Slash climbing controls work and do not
reproduce the recorded failure. A debug headless software-renderer attempt
cannot enroll the co-op renderer and is excluded. A Dragoon probe timed out
waiting for a private screenshot before its approach; it establishes neither
fight activation nor a gameplay regression. No user windows were opened and
all background launches were muted.

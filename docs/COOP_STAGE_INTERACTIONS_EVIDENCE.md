# X4 shared stage interaction evidence

Static evidence uses the owner's original US executable `mmx4/SLUS_005.61`
and its trusted generated C. No original executable bytes or game assets are
included. Native runtime qualification is separate from this trace.

## Military Train background updates — feedback item 28

Slash Beast's native introduction `8006DC24` sets authored foreground room
bounds. Those bounds do not stop the separate train scenery animation.
Stage 8's first background mode table `800F32D4` selects mode 5, whose
callback `800285E0` does nothing. Shared actors advance the scenery.

Real online Split qualifications QA79/85/86 kept reproducing the other pane's
frozen background while the canonical pane advanced 8/4 pixels per tick.
Both players were healthy and idle after P2's Slash entry. Earlier models
matched native function arithmetic but assumed the wrong physical actor
pool and kind numbering: neither the 20-slot `801410C0` pool nor the
32-slot `801435B0` pool contained the live boss scenery actor. Those
inferred pool gates rejected the actual callback, so their passing models
did not establish the requested native behavior.

QA87 supplies the decisive original callback/write trace. Repeated
`800D9218` entries have `a0 = 80169CB8` and `ra = 80021BDC`; the dispatch
table register is `s1 = 800F2AB4`. Native background writes at `800D9388`
and `800D93A4` use `s0 = 80169CB8` and advance background 1/2 by exactly
8/4 pixels. `800D8648` and `800D964C` are absent during this idle boss
loop. The trace is retained privately in
`qa-real-split-train-trace-87/train-background/native-traces.json`.

The original executable independently agrees with every traced address:

| Contract | Original instruction evidence | Result |
| --- | --- | --- |
| Pool base | `80021B38`: LUI `8017`; `80021B3C`: ADDIU `-6348` hex | `80169CB8` |
| Pool end | `80021B40` adds `C0`; `80021BE4` constructs `80170000 - 6288` | `80169D78` exclusive |
| Slot stride | `80021BE8` adds `30` | Four `30`-byte records; last slot `80169D48` |
| Table | `80021B60/64` construct `800F2AB4` | Kind 0 → `800D8648`; kind 2 → `800D9218` |
| Original caller | `80021BD4` executes JALR | Return PC `80021BDC`, matching QA87 |
| Matching allocator | `8002B048` constructs the same base, adds `C0`, advances `30` | Same four-slot pool |

The signed low immediate matters: `80170000 - 6348 = 80169CB8`.
The native dispatcher `80021B34` has a different pool/table from
`8002196C` and `80021A20`. The latter two loops cannot identify the
callback merely because their overlapping tables also contain `800D9218`.

| Live scenery kind | Native update | Background behavior |
| --- | --- | --- |
| 0 | `800D8648`, active `800D872C` | Initializes actor `+18` to `00080000`; adds that speed to background 1 X at `800D878C` and its arithmetic half to background 2 X at `800D87A8`. |
| 2 | `800D9218`, active `800D9330` | Initializes the same speed; adds it to background 1 X at `800D9388` and its arithmetic half to background 2 X at `800D93A4`. The outer update also calls `800D9B48` for boss-phase layer metadata. |

Native `80021158` calls foreground `80027850` at `800211D8`, background
helpers `80027D40`/`800281E8` at `800211E0/E8`, then the actual scenery
pool `80021B34` at `800211F0`. This ordering agrees with QA87's writes:
the background camera write at `80028238` precedes the train actor writes.
Thus live scenery runs after co-op camera snapshots; calling the mode 5
helper for the other pane cannot recreate those external world deltas.

Kind 2's region selector `800D993C` uses player X thresholds
5808/7648/9984/10000 and resets subphases when the region changes. Earlier
phase 1 calls full helper `800D964C` from `800D9480` only at `+6 == 2`.
If `+7 != 0` and `PLAYER+BC == 0`, it adds
`current_foreground_X - previous_foreground_X` to background 1 and its
arithmetic half to background 2. The stopped-train `+7 == 0` branch instead
decelerates or sets `+7 = 1` and shared limits; that transition tick adds no
foreground displacement. This earlier-phase helper is additional original
static evidence, separate from the directly observed idle boss loop.

The revised Split adapter accepts only the trace-corroborated four-slot
pool and kinds 0/2. It calls each original update once and propagates captured
background deltas to both saved cameras without adding them to canonical
RAM again. Where native `800D964C` actually runs its foreground branch, the
other pane receives its own recorded foreground contribution instead of the
canonical contribution. Arithmetic half preserves native `sra` for negative
odd fixed units. Changed authored limits, dead zones, shake and layer metadata
are copied; foreground current/previous origins remain private. Shared actor
effects, particles, story commands and animation updates still run once.
Unified, player projection and local drawing bypass the adapter. Section
reset starts both snapshots from the new native origins.

Focused models now use the actual four-slot table and exact QA87 base-slot
kind 2 callback. Foreground/background camera snapshots precede shared pool
dispatch. They require one original 8/4-pixel update, absence of `800D964C`
during the idle loop, both panes advancing, and no second addition on the
next frame. First/last slots and rejection of both previously inferred pools,
below-base, exclusive-end and misaligned records are covered. Existing cases
retain independent origins, owner switches, fractional movement, freeze
gates, stopping transition, region reset, initialization metadata, uint32 wrap,
shared scenes, cleanup, wrong kind, projection, reset, local drawing, another
stage and Unified. This revision has source inspection and a clean whitespace
diff. Root owns model execution and repeat native raster/CRC qualification;
item 28 remains open until that requested behavior is validated.

Native qualification requires a healthy connected two-peer Split match:
load stage 8, section 1, checkpoint 3; place both players at X9360/Y395 and
use native Right to cross X9472 with either leading seat while the other
remains at X9360. After the warning/introduction, keep both living players
stationary and compare completed-frame captures over several seconds. Both
saved scenery panes must advance while foreground stays inside boss limits.
Read four records at `80169CB8 + slot*30`, especially the observed base-slot
kind 2: bytes `0/1/4/5/6/7/14` and word `18`. Inspect canonical backgrounds
`80141A04` and `80141A58`, current X/Y at `+8/+C`, previous X/Y at
`+14/+18`, and the saved camera state published by `mmx4.coop.split-state`.
The `mmx4.coop.split-train-background-ticks` counter should advance during
the observed original loop. Repeat both leading seats and campaign rosters;
verify matching deterministic state on both peers.

## Lightning Web wall-jump anchors — feedback item 22

The requested enhancement concerns X's Lightning Web weapon, earned from Web
Spider. The boss's damaging/capturing web actors are different: secondary
types 22/23 (`8009D74C`/`8009DD40`) have a null solid pointer and use native
player damage/capture. They are outside the requested enhancement.

Lightning Web is player shot type 1, native update `80092F08` (dispatch table
`800F25D8`). Its opening animation installs solid hitbox `80108800` at `+68`
and enables `+75` in `800932A0`. The update calls original solid contact
`8002E184`, then clears top/bottom contact bits from `PLAYER+71` according to
shot `+72` (`80092FA8`–`80092FFC`). Horizontal contact remains available to
the player's ordinary wall-jump action. Expiry clears the solid eligibility.

Native solid comparison `8002C36C` writes shot `+72` and player `+71`/`+4A`
from actual contact geometry; it has no X/Zero character check. Co-op keeps
each player's 16-shot pool private. The existing canonical solid retry reaches
P2 for P1's web, but a P2-owned web updates while projected, so the canonical
retry is skipped and P1 never contacts that web. The native post-contact
top/bottom cleanup also applies only to the singleton owner body. A narrow
enhancement must offer each live partner only the web's solid contact, retain
its per-seat contact history, and perform the same per-seat top/bottom cleanup.
It must not update the projectile animation, lifetime, movement, or damage
twice, and must not change Web Spider's enemy web behavior.

The separate `mmx4_coop_web.c` adapter handles only an active player shot 1
with enabled `+75` and the verified `80108800` solid pointer. It calls the
original `8002E184` once for X and offers the other living normal Zero body
the player portion of that collision: `8002E294` position response,
`8002E380` carry when that partner had prior contact, and `8002C36C` side
comparison. These are full native function entries in the trusted ranges;
each takes the victim as explicit `a1`, and carry takes prior side bits as
`a2`. The adapter preserves the caller's `a2` and original return value.

The noncurrent Zero body is copied to one deterministic guest mirror while
its owner and both private shot pools stay in place. Only the helpers' proven
mutable bytes return to the saved body: integer XY at `+A/+E`, separation
XY at `+6C/+6E`, wall-contact kind at `+4A`, and sides at `+71`. Fractional
coordinates, action, HP, inventory and campaign state are not copied back.
Top/bottom cleanup matches X's original web update. History is indexed by
allocating seat and the 16 native shot slots, so identical guest shot addresses
in different private pools remain distinct. Native allocator `8002AC0C`
returning a reused slot invalidates only that seat's old contact; stage reset
clears both sets, and the deterministic digest includes every history byte.

Queries exclude incoming actions 0/1, hidden/dead/vehicle bodies, scene and
door holds, unknown `+67` movement states, other relevant native holds,
and local drawing. `+67` is also used for normal airborne movement, so the
adapter accepts precisely the inspected action/control pairs: Jump 6/1
(`80034538`), Fall 7/FF (`80034604`), WallJump 10/1 (`800349F4`) and WallSlide
11/FF (`80034AFC`), plus ordinary control 0. Treating every FF value as a
script lock would disable web contact while Zero approaches or slides along
the anchor. Native `80033B34` requires Jump edge and positive wall-contact
kind at `+4A`; `800349F4` then starts actual action 10 with upward velocity
and horizontal velocity away from the surface. Both player arrangements,
active combat pose 48, and Jump/Fall/WallSlide/WallJump contact progression
have focused model coverage. Additional cases cover real contact geometry and
carry ordering, opposite wall sides, shot-pool aliasing, allocation reuse,
vertical cleanup, field/return/register preservation, story/arrival gates,
unopened/expired/other projectiles, reset and allocation failure. No projectile
update runs twice and no enemy web path changes.

Root owns integration, compilation, model execution and native wall-jump
qualification. Native qualification should select Lightning Web weapon 1 for
the X seat using the existing development weapon command, fire with original
controller input in an ordinary stage, then have Zero approach each opened
web side and press native Jump while moving into it. Verify actual normal
wall-jump action/trajectory in both rosters and camera modes, expiry/reuse,
and scene/warp exclusions. Counters `mmx4.coop.zero-web-queries` and
`mmx4.coop.zero-web-side-contacts` distinguish an offered native query from
accepted horizontal contact. The `mmx4.coop.zero-web-body-mirror` counter
publishes the existing guest query mirror address for read-only contact
inspection; it allocates no extra diagnostic sidecar. The owner has not yet visually qualified this
enhancement.

`tools/mmx4_web_probe.py` requires an explicit existing private TCP port and
never launches a process. With `--scenario web --campaign 0` or `1`, it uses
the observed Jungle platform at X56/Y139, native weapon-selection fixture 7
and Square input to create a real web. It reads both live solid boxes to align
Zero beside the opened anchor, requires direction input to produce actual
side contact, and then checks native Jump for action 10 plus an upward/away
trajectory. The safe setup is bounded to the observed initial platform's
X32–256/Y75–139 air column; a web outside that column is reported as an
unqualified setup rather than teleporting into unobserved terrain. For X/P2,
the existing diagnostic tail publishes one completed opened-web record with
WEB1 header at `+6F0` and the native `9C`-byte shot at `+700`. X/P1 uses the
original shot pool. The helper releases inputs on failure and retains JSON
observations and a native action 10 screenshot for root's visual review.

The same helper's `--scenario train --owner both` drives the original Slash
entry for each leading seat and compares background X movement in both saved
Split cameras during an idle fight. An existing private invulnerability
fixture prevents combat knockback during the measurement and is cleared
afterward. Both scenery panes must advance while the foreground remains
within one pixel of its initial camera origin. Source syntax was inspected
without connecting to or controlling a runtime; root owns these qualifications.

Actual Split views require an online match; requesting Split in an offline
configuration keeps Unified behavior. `tools/mmx4_netplay_runtime.py` supplies
`NetplayRuntime(existing_development_match)` for the unchanged native probe.
Read/capture requests use peer 0, setup fixtures use the existing admitted
`NetplayDevelopment` packets, and seat input goes to that seat's peer on the
host input layer. A both-player teleport uses one fixture 4/seat 2 command.
Direct RAM writes and other raw mutation requests are rejected. Its eight
offline fake-peer tests verify routing and completed-body reads, with sockets
prohibited; live Split/native CRC and visual qualification remain with root.


Root native QA88 qualifies the corrected range using two connected hidden
peers with real Split rendering and synchronized development fixtures.
The live base-slot actor is type2, state[1,5,0,0], speed00080000. Original
Slash entry is exercised with each leading seat. During two-second idle
windows, both saved scenery panes advance equally: BG1/BG2 +496/+248px
for owner1 and +464/+232px for owner0, while foreground X changes0px.
Both peers' before/after screenshots show the moving train structures.
Core/mod CRC samples match after each qualification, through tick8672.
All18 title CTests pass. This corrects the reproduced Split scenery freeze;
it does not claim complete campaign or production/display FPS coverage.

Native Zero wall-jump qualifications QA76/77 also pass in both player
arrangements. A real X Lightning Web is fired with native controller input;
Zero enters action10 and moves upward and away from the contacted anchor.
Those cases qualify the enhancement, beyond model-only contact fields.

## Checkpoint loading and retained-player cameras (2026-10-11)

Hidden original-disc QA15/16/17 reproduces a healthy P2 becoming a carried
corpse after a native Bio Lab section0 checkpoint1 load. P2 has HP32 at
8001FC20, then is marked dead by the next 80035240 initializer. This happens
without a native P2 damage/death update. The co-op camera constraint checks
the outgoing P2's Y against incoming room limits during loading; its direct
HP128 write is subsequently retained as a real death.

Camera target, owner scope and team constraints now require mode6/minor0
and an initialized canonical PLAYER dispatcher. A dying/dead primary
(states2/3) still permits the survivor's camera. Loading mode4/5 and the
temporary mode6/state0 boundary use the native camera. Partner drawing also
excludes that uninitialized boundary, avoiding old-body rendering in the
new room. Carried-death counters distinguish this bookkeeping from a plugin
failure. There is no opt-in setting for the correction.

QA18 replays the formerly failing checkpoint with both HP32, concurrent
arrivals, clear controls and unchanged shared lives. The reusable
tools/test_mmx4_coop_checkpoint.py then passes both pause owners, voluntary
withdrawal across a checkpoint and Select rejoin, section1/checkpoint1
arrival, P1 death with a living P2 executing its native jump, and a full
team retry spending exactly one life (2 to1). All18 title checks pass.

These are native loader/controller qualifications. The owner's exact Web
Spider enemy-shot/HUD disappearance and staircase teleporter clip remain
unqualified; this checkpoint result does not close those reports.

## Executable-relative playtest diagnostics, October 11

Actual packaged Playtest6 creates default-on recordings, but its recording
directory follows the configured memory-card root. A development launcher
probe showed that root resolving outside the presumed package saves folder.
The next title recorder therefore uses the actual application directory,
independent of cwd and card configuration. Windows uses the running module
path; native Linux uses `/proc/self/exe`; AppImage uses the portable image
path preserved by AppRun in `RECOMP_APPIMAGE_PATH`, outside the mounted image.
`diagnostics-location.txt` beside the application names the exact session or
a recording error. The input display shows ERROR on write failure.

Every session retains accepted inputs, starting cards or absence markers,
mod choices, available settings and network/build/execution metadata. The
offline replay helper uses isolated cards/settings and rejects marked network
traces before launching anything; network logs remain inspectable.

Native Windows nextQA30 presses the real hidden launcher Play button, boots
X/P1 and Zero/P2, drives movement/attacks and each native pause owner, and
exits normally. It produces2896 input records; offline playback verifies2895
input-state hashes. QA31 boots the opposite roster, produces2551 records and
verifies2550 on playback. Both deliberately store memory cards separately
under private-cards while recordings stay beside their executables.
Real two-peer LAN QA32 produces2521 records per seat and95 matching core/mod
CRC samples through3166 while each seat moves/attacks. These are local LAN
captures, not qualification of full internet replay. All18 Windows checks
pass. Linux g++ also builds/runs the recorder, replay, error and real native
executable/AppImage path tests; full Linux gameplay qualification is separate.

## Split Mushroom lift and staircase review, October 11

Commit11da504 fixes an immediate co-op fatality at the circular lift's native
section1/checkpoint2: it sets destination camera maximum720 while the actual
camera remains2047 and riders are nearY2200. The death plane now uses the
deeper of the current camera and authored room maximum. QA35 passes the
native limit change, team retry spending exactly one shared life, and continued
ascent in both rosters. X/P1 reaches camera1746 with bothHP32 atY1900/1899;
Zero/P1 reaches camera1748 with bothHP32 atY1901/1902. Reports are under
build-pr23/qa-mushroom-parity-35/{x,zero}-custom/lift/report.json.

QA34's11.75-second recording separately shows centered idle riders meeting an
overhead panel atY1233. The owner watched it and confirms authentic behavior:
players need to destroy those panels. This is no longer treated as a bug.
The reusable elevator check stops before the obstacles instead of requiring
idle riders to survive the entire ascent. No crush policy was weakened.

IMG0755's foot/tread offset is visible in the tester's footage. The blue spiral
is stage3/section1/checkpoint0 beforeX2400. Native slope positions obey
X:Y=714-floor(worldX/4) and Zero:Y=713-floor(worldX/4). Foreground mode2 uses
cameraY=offset512-trunc(cameraX/4). QA35 compares both rosters at five positions
fromX400 through1200, switching stationary bodies between4:3 and16:9.
The configured renderer's ring/map validation reports1113 checked tiles and0
differences at each boundary. Custom-renderer-disabled native4:3 reference
captures also align visually; its unconfigured generic comparator output is
not an X4 map-validation result. QA36 uses1080p, separates the players161px,
and pauses/resumes with each owner. Native heights and visible feet remain
correct. The floating pickup and tester offset are not reproduced, so
item36 remains open.

The exact section2 staircase-arrival footage for item34 is unavailable in the
specified clip range. Frame-by-frame review of IMG0757's later lift retry
at5.77–6.47seconds shows ordinary beams, overlapping circular arrival
sprites, then both normal bodies. This does not qualify the missing report.

## Offline co-op states and diagnostic baselines, October 11

Offline co-op now registers a versioned, pointer-free host snapshot provider.
It preserves both native player contexts, lifecycle/menu ownership, combat/web
state, private audio headers, Split/widescreen state and immutable texture-bank
descriptors alongside the guest/device snapshot. Resource initialization runs
before a possible first-PC load. Immutable SPU sample banks wait for hardware
initialization and rebind once per hardware epoch. The provider excludes live
guest-call/projection stacks; shared save/load and rewind defer until the
continuation context is safe. Network user state jumps remain blocked.

Native Windows QA39 uses X/P1 and QA40 uses Zero/P1. Both drive the mapped F7
panel and actual Square Save/Cross Load, move both bodies, restore positions,
restore P2 after a fatal HP fixture, and restore a settled P2-owned native pause
menu that P2 can unpause. They select/accept and cancel real F8 rewind history,
then start a new process with slot0 auto-load and attack with both characters.
QA44 repeats the final shared safe-context changes with X/P1. All retain both
healthy active bodies and the original roster; inspected warm/cold/rewound
screenshots preserve complete sprites and aligned HUDs.

Successful offline state loads and rewind start a fresh diagnostic segment
with their complete baseline.pst. Checked loaders notify only after accepting
the restored PC; failed loads roll back quietly and do not split recordings.
The replay runtime compares the actual restored image against the required
baseline before admitting input rows. QA41 cold-load playback verifies152
hashes; QA42 raw-rewind playback verifies48. QA45 repeats cold-load playback
on the final X/P1 build and verifies155 hashes. Native Linux focused serializer,
no-provider handoff, arena reset, RAM geometry, diagnostics and replay-preparation
tests pass. Direct AppImage playback by the helper is rejected with extraction
instructions; extracted native Linux payloads use copied libraries and private
mod/settings/data roots. This is not full Linux gameplay qualification.

Final two-peer native LAN QA43 reaches original gameplay, exercises movement
and attacks for each seat, and matches99 core/mod CRC samples through tick3201.
Its last closed-menu save-unavailable assertion was invalid: that field reports
the currently displayed message, not capability while the menu is closed.
Network request refusal remains covered by the shared title-policy test. This
LAN check does not qualify a Discord login, WAN path or full network playback.

Session review also found enhancement allocation cursors and packet/texture
registrations surviving lobby teardown. The runtime now retires those resources
after renderer teardown and before each activation. Focused tests qualify
five large identical allocation layouts,20 packet/bank reuses, active-pass
refusal, journal retirement and preservation of ordinary RAM/scratchpad. A
complete UI-driven rematch remains additional coverage rather than a claimed
native result.
# Independent character codes and complete room plans (October11)

The next build adds separate default-off Ultimate Armor X code and Black Zero
packages, plus corresponding host-agreed co-op options. The original X code
selects PL02 resources and makes a capsule grant the armor; it does not grant
armor at stage start. Black Zero retains original stats and palette13. P1
continues to own the campaign and saved code byte; P2 uses its selected code
only within its projected context. Both X archives are prepared before cold
restore, share verified-identical UI/audio, and occupy the existing fixed
reservation. Sprite descriptors record the archive variant; the co-op host
snapshot schema is now2.

Private QA46 (X/P1) and QA47 (Zero/P1), both codes enabled, pass native mapped
save/load, repeated loads, P2 death restore, P2 pause ownership, rewind
accept/cancel and fresh-process load followed by attacks. Inspected restored
images show X's purple code helmet and black Zero. QA49 replays QA47's exact
cold baseline and verifies158 hashes. QA51 separately boots each solo campaign
with both standalone packages enabled, observes the selected original code
body and attacks, and confirms neither campaign receives armor at stage start.
The original-EXE fixture executes resource selection, palette copy, selector
clearing, capsule eligibility/grant and Nova gating/energy instructions; it
passes on Windows and Linux. It also rejects duplicate hooks and tests P2 actor
scopes that retain P1's campaign selector.

Native QA54 publishes all six camera/HUD/code combinations with distinct
settings fingerprints. Its real launcher adopts Unified/Split host choices
with codes enabled and disabled despite opposite local preferences, rejects
wrong fingerprints and undeclared choices, and leaves persisted preferences
unchanged. The initial QA48 query failed because the shared START encoder
silently truncated its640-byte nested field. Shared commit5a2332b6a replaces
fixed scratch limits throughout encoding, queueing and inbound framing with
the declared12-package capacities, and rejects truncation before publication.
Focused actual-metadata and maximum-plan tests pass on Windows with and
without ICE and on Linux with ICE. QA53 boots two real hidden LAN peers with
both options enabled: each seat moves/attacks and100 core/mod digests agree
through tick3250. This does not qualify Discord authorization or WAN/NAT.

All21 Windows title checks pass. No game window was shown, and every native
run used explicit private memory cards. Full Linux gameplay remains additional
coverage. These changes are not in the published Playtest6.

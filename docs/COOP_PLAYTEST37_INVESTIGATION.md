# Playtest36 burndown: implementation evidence

Scope: the owner's attached goal and [the recurrence review](COOP_FEEDBACK36_REVIEW.md).
This is a working investigation record. Item157 remains open; no new release
has been qualified or published by this work.

## Original local scaling controls

An instrumented executable was built from unchanged36 gameplay sources with
`PSX_DEBUG_TOOLS=ON`, `MMX4_COOP_NETPLAY_DEBUG=OFF`. It retains the production
execution identity and co-op compatibility prefix/hash. Original diagnostic
header, mod-plan, input and recorded state checks remain enabled.

| Supplied recording | Options | Result |
|---|---|---|
| Storm Owl36 / P2 /1792313942710697 | Local, scaling on | Completes5503 verified input intervals;814 observed samples map to original recorded player state. Eleven native HP stores include three complete24/25/26 ground chains. |
| Storm Owl36 / P1 /1792313940507867 | Local, scaling off | Completes5773 verified input intervals;932 observed samples map to original recorded player state. Four native HP stores in this encounter. |

These results qualify inspection of the supplied local encounters. They do
**not** reproduce the owner's reported ON-worse-than-OFF contrast, and are not
acceptance of157. Boss HP is not part of the diagnostic player-state hash;
separate native write traces and captures establish the observations above.
The moving encounters diverge after hits, so aggregate hit totals are not a
fair gameplay-equivalence metric. Do not replace the missing contrast with
those totals.

Rendered contact sheets and full captures were inspected. Later attack
presses also occur during hurt states or while the existing aerial attack is
still running. Such presses are not automatically fresh connected blades.
A specific missed-attack timestamp was requested asynchronously; other
burndown work continues independently.

## Native independent-cooldown expiry gap

A new near-simultaneous local native test exposes a distinct pipeline defect.
With scaling/separate mode active, a native X buster hit at backend frame5121
installs the shared rejection table `801060F0` with countdown1. At5122 Zero's
fresh class24 blade reaches that rejection table and receives reactionFF,
although Zero's private timer is ready. The native caller expires its shared
timer afterward; the blade finally damages at5123. The earlier twelve-frame
overlap checks could not establish correctness at this boundary.

The candidate caches the actual table used by an accepted hit and the inspected
SB/SH post-hit timer field. Only the pending native one-tick expiry window with
the exact common rejection table is eligible for projection. A ready seat's
native contact sees the cached table and zero shared countdown, so its accepted
result reaches the original boss caller for one ordinary reaction. A no-hit or
duplicate restores the original table/countdown for native expiry cleanup.
Completed windows are disarmed; unrelated tables and real invulnerable phases
remain native. Global Sigma timer stores are not projected as actor fields.

The cache uses fixed-width serialized/digested guest values, is cleared on
activation/slot reuse, and rolls back for Cyber's discarded damage. Snapshot
format becomes X4PA. The existing health scale, timer durations and blade
lifetime policy have not been represented as resolved by this correction.

| Check | Evidence |
|---|---|
| Counterexample regression | Disabling table projection fails the ready-partner admission assertion; candidate passes both owner orders, no-hit cleanup, genuine phase, later phase, slot reuse and snapshot restoration. GCC builds without warnings. |
| Native local candidate | Adjacent X/Zero hits now register on consecutive backend frames5480/5481; two projected admissions in the bounded trial. The inspected native reaction uses the normal table rather than FF. |
| Real two-peer Split, X/P1 and Zero/P2 | Both peers record the projected ready-partner hit; actual combat core/mod checkpoints match through5924, both heroes remain healthy and no local-render state leak is reported. Captures retained. |
| Real two-peer Split, Zero/P1 and X/P2 | Reversed roster also exercises ready-partner projection on both peers and matches actual combat core/mod checkpoints through5667. |
| Linux portable unit | The same regression/counterexamples compile with GCC and pass under WSL Ubuntu; no cross-OS match compatibility is inferred from compilation alone. |
| Initial LAN geometry control | X20px from the boss did not exercise the one-tick window. It is excluded as coverage; X28px adds the native bullet travel tick and exercises the boundary. |
| Windows title suite |34 of35 pass in the initial run. The detached-pointer-unavailable logging fixture hits its10s child deadline during heavy evidence copying; its isolated rerun passes. Final release validation still needs a complete clean suite. |

Remaining slash cases, weaknesses, and broader burndown validation are still
pending. This correction alone does not close157.

## Evidence locations

Private source scripts and intake: `build-pr23/followup36/` and
`build-pr23/observe_local36.py`, `analyze_observed36.py`, `compare_observed36.py`,
`fast_overlap37.py`, `overlap37_lan.py`, `balance37_counterexample.py`.

Durable initial archive: `build-pr23/followup36/qualification37/`,623 files,
1,554,030,205 bytes verified against source hashes. It contains original local
observations/captures and the initial counterexample. Later native overlap
and LAN receipts originated under
`C:/Users/Matthew/AppData/Local/Temp/mmx4-feedback37/`; the incremental manifests
below now retain those receipts in the qualification directory. The local
candidate has its own `overlap-candidate-manifest.json`. Failed/insufficient
setups remain distinguishable from passing evidence.

## Storm Owl leading-player exit and interrupted death

A real two-peer Split36 baseline uses native dash-jump input from the observed
safe final ledge6240/426. It does not teleport past the native6352 exit test.
With P1 alive at the start, P2 crosses6376 without a forward fade even after
five world updates. With P1 killed immediately before the crossing, P2 reaches
6352 and finishes its healthy outgoing beam, but section0 remains stalled for
240+ netplay ticks. P1 is HP0/state1/action17: the real shared fade suspends the
unfinished death animation. Both baseline peers remain synchronized.

Two separate native predicates need correction. Owl's background may inspect
the farther living P2 X coordinate while retaining its original threshold and
one shared update. At stage_normal's original80020018 branch, a positive
forward-transition flag and a healthy completed P2 beam can complete the exit
when P1 is dead. This changes only the loaded predicate register, neither
body nor campaign data. Negative retry flags, temporary hidden passengers,
voluntary withdrawal, other modes and live P1 retain their original behavior.
The instruction site is emitted from both development and release configs.

Qualification currently includes:

- Local Unified, X/P1 and Zero/P2: leading-P2 and newly dying-P1 cases reach
  section1; both heroes return active, visible, healthy and controllable with
  the same lives. The new `storm_exit_race` scenario avoids the older test's
  wait for P1 to finish dying, which concealed this boundary.
- Real two-peer Split with both X/P1-Zero/P2 and Zero/P1-X/P2 rosters: both cases
  reach section1 and restore32HP, visible controllable bodies and unchanged
  lives. Matching actual core/mod checks through6400 and6113 respectively.
  Captures from both seats were inspected.
- Lifecycle regressions cover both campaigns, the incomplete P1 death, genuine
  forward completion and rejection of no fade, retry, live P1, dead P2,
  unfinished P2, withdrawn/hidden P2, render scope, other modes and inactive
  co-op. Existing passenger/catch-up and native-owner departure cases pass.
- Combat, balance and lifecycle CTests pass; regeneration emits the guarded
  native branch callback. A complete release suite remains pending.

Receipts are under the private `mmx4-feedback37` root in
`owl-exit-confirm-before`, `owl-exit-after-valid-disc`,
`owl-exit-after-zero-p1` and `owl-local-after`. The initial walk-only setup
fell normally into a gap before reaching the trigger and is excluded.
An initial candidate launch misspelled the private disc filename and exited
before boot; it is a setup failure, not gameplay evidence.

This qualifies checklist25. It does not establish that Owl's separate
post-miniboss and Buster-upgrade/death reports166/167 share the same cause.

An incremental verified archive now adds115 files (57,194,007 bytes), covering
Owl baseline/candidate receipts, recent overlapping-hit LAN receipts and the
initial Cyber portal investigation. See `incremental-owl-cyber-manifest.json`
under the qualification directory. The full Cyber qualification adds176 files,
77,745,601 bytes and22 executable hashes in `cyber-qualification-manifest.json`,
including excluded/failed setups as well as the qualified native receipts.

## Cyber timed-course portal (investigation in progress)

The old hub-only tests pass but do not cover the reported timed-course portal.
Native Split reproduction starts P1 at72/203 and moves P2 from the observed
last platform1712/154 to1792 with ordinary Right input. P2 enters command20;
P1 completes its temporary outgoing passenger beam. Both peers stay in sync,
but the course actor (kind38,80143028) remains in phase4. Its wait timer wraps
because +0x16 is never signaled. Captures match the supplied portal scene.

The original targeting graphic is particle34, native800CE894, linked to this
controller at +0x50. It follows canonical P1. At the P2 portal handoff it
reaches native camera cull8002B160 from800CEA8C, disappears, and never reaches
its ordinary completion write800CEF6C. Trace/captures establish this; no timer
or completion flag was manually forced.

An initial owner-body projection alone still allows that cull. A scoped camera
copy is also insufficient in the native test. Neither trial is accepted as a
fix. Temporary guard instrumentation confirmed correct P2 ownership and has
been removed. The current candidate keeps the one native assessment update
with the actual P2 scene owner and protects its completion producer from that
specific bounds-based deletion during projection. The original grading,
animations, timers, completion write and final self-deletion remain native.
The protection is delegated inside Split's already registered bounds handler,
before its ordinary projection/availability gate. A separate registration was
rejected by the runtime's one-handler-per-plugin/address contract. Unit stubs
now reject duplicate addresses and test this pre-availability delegation.
A literal/macro source audit finds no remaining same-plugin function-hook
collisions; different plugin IDs may intentionally share an address.

Actual native before/after now qualifies the full assessment and return with
both Split rosters. With X/P1-Zero/P2, the original S-rank result renders and
checkpoint1 loads with both heroes visible32HP/idle, no personal locks and
matching core/mod checks through4578. Reversing the roster also completes,
with matching peers through4323, and subsequent ordinary input moves both
heroes from72/96 to87.5/111.5 with32HP. Captures are retained. These are real
native grading, checkpoint loading and movement, not forced completion flags.
The expanded bounded harness covers the subsequent native100-frame result
animation; one earlier short trial stopped in this active phase5, not another
softlock. Local Unified also qualifies both rosters: the timed P2 assessment completes,
both heroes return healthy/visible and ordinary input moves each after return.
The existing hub1 P2 handoff and hub5 P1 section handoff still pass. The reversed
local roster also repeats both Owl exit-race cases successfully. Checklist165
is qualified by this bounded before/after evidence. The separate all-course P2 targeting-graphic
feature172 remains open regardless of this portal correction.

Local test setup is part of the evidence boundary: an instantaneous P1
teleport could delete the initial targeting graphic before its opening scan
finished. Another setup waited for that scan before crossing its actual native
entry threshold400. Both failed setups are excluded. The tracked native test
now uses observed36 route positions in bounded steps, waits through the opening
scan on its first platform, then uses ordinary P2 Right input across1792.
It does not force a grade, completion flag, checkpoint or warp state.

All35 Windows title CTests pass in one complete run (12.75s), including the
previous detached-pointer logging fixture. Linux portable lifecycle and Split
regressions also pass; the latter reports two existing misleading-indentation
warnings in the fixture. This is not cross-OS match compatibility qualification.
Production-package validation and the rest of the burndown remain pending;
157 is still open and no release has been authorized as validated by these
portal/Owl checks alone.


## Frame-scheduled slash controls and remaining instrumentation gap

The next discovery controls use the debug server's queued, vblank-counted pad
route instead of alternating short TCP presses with diagnostic reads. Basic
Zero/P1 air, single, combo and cancel trials complete with scaling OFF and
Separate ON. OFF admits 2/2/3/3 native damage stores; ON admits 5/4/17/6.
Those totals do not reproduce157: Frost walks past the fixed-facing player,
so many later swings are visibly out of range. Initial AI states also differ.
Neither totals nor these controls qualify the owner's contrast.

A sustained upgraded class34 aerial at a safe left-side position also completes
OFF and ON, with one initialized blade and one accepted hit in each. Captures
show the blade and impact; the existing lifetime guard rejects eight later
ON contacts, but the OFF control does not accept a second damage opportunity.
This case therefore does not prove that removing that guard fixes157. The
first two wall setups use unqualified floor coordinates; the second records
both setup deaths. They are explicitly excluded. The successful setup uses Zero1120/X1144 and
retains native boss movement. It still has different initial boss AI timing.

Source/native executable checks narrow the remaining investigation:

- Native801060F0 rejects all inspected Zero types24-28/34/35. Frost's normal
  80107A78 table gives those blades2/2/3/2/2/3/5 raw damage respectively.
  Fractional scaling therefore cannot explain a zero-integer-damage basic
  Frost slash: these particular accepted blade hits have at least two raw HP.
- Native8002DD04 normalizes the child7E duplicate result to zero before the
  co-op wrapper considers the other seat. A private immunity rejection does
  not by itself skip P2 through a nonzero outer result. Do not patch this
  ruled-out hypothesis.
- The existing queued route only drives guest port1. It cannot establish
  frame-scheduled P2 controls merely by passing a port argument it ignores.
  Shared debug-tool task `beads-eio.3.358` adds a validated1/2 selector,
  preserving port1 by default. Source-function tests cover port isolation,
  exact segment boundaries, deferred start, invalid ports, clearing and the
  legacy route on Windows and Linux. Native P2 input qualification is recorded
  below.
  This guest override is not an authoritative netplay host input route.

Private discovery archive: `qualification37/route-investigation/` beneath
`build-pr23/followup36/`, with source-script snapshots and SHA256 manifest.
No gameplay fix, acceptance of157, or release is claimed from these controls.


### Native P2 route qualification

The rebuilt debug executable830e110c46b62404a47075c06f6f8f90086e258f1717798cd76eeecb73194efd
runs the same queued basic-air pattern in fresh scaling-OFF and Separate-ON
Frost encounters, with Zero/P2 and X/P1. Both native diagnostic recordings
contain190 consecutive verified input frames: five identical38-frame cycles
(4 Cross,3 neutral,4 Square,27 neutral). P1 is neutral throughout. This verifies
actual native P2 delivery rather than accepting the new command's response.
Input receipts are `pad-route-verification.json` beside each report.

The OFF encounter admits class27 then class24 (48->44); ON admits five class27
hits, exactly38 backend frames apart (48->43). Native captures show basic green
blades, repeated jumps and impact/white-palette reactions. The OFF hit pause
changes the later player's attack state; not every Square pulse there becomes
a fresh aerial. Initial AI timing also differs. This comparison qualifies the
instrumentation and the bounded ON contacts, **not** the reported failure or
acceptance of157. Source-function checks also run on Linux GCC, without claiming
native Linux controller or cross-OS netplay qualification from those checks.

The next negative control disables only4985's ready-table projection in a
separate local executable, retaining the normal working binary and source.
Its provenance explicitly distinguishes it from an unchanged36 build; it must
not be used as a qualified netplay/replay identity. Repeat the now-verified P2
route to determine whether that correction explains the observed admissions.


The local-only no-projection negative control also completes with five class27
hits,38 backend frames apart (48->43). Its native190-frame P2 input window is
verified against the same pattern with P1 neutral. Thus4985's shared-table
correction does not explain this particular solo aerial control, even though
its independent paired-hit defect was reproduced and fixed separately. This
narrows the next comparison to the remaining upgraded/ground/cancel cases and
the actual reported encounter; it does not resolve157.

Shared input-route implementation: framework commitf2344be7,
[PR686](https://github.com/RetroPortingToolKit/psxrecomp/pull/686), assigned to
TechnicallyComputers. Its base retains the existing X4 framework line; unrelated
framework master changes have not been pulled into this investigation.
Native source/capture/input receipts are retained in the verified private
`qualification37/p2-route-controls.zip` with a separate size/SHA256 receipt.


## Storm Owl post-midboss camera release (166)

The supplied36 paired Owl recording1792318160615607 reaches part2 and ends
with living P1 around2814/971 and P2 around2552/970. The stalled P2's cause
cannot be determined from those input/position rows alone. Fresh native
Split/Both fixtures reproduce it after defeating the original Generaid Core,
kind70. The fully retired boss leaves canonical/P1 maximum X=2560, but P2's
saved current/target maximum remains2240. Holding Right clamps Zero/P2 at
2552.5 with32HP and no C0/C4 lock, while P1 proceeds to2833. Both peers still
agree. Captures show P2 pinned to the old view's right edge.

Native death handler800891C8 writes2560 to current maxX at891FC and target
maxX at89204, before continuing the original tile/death finalization. Mirror
exactly those two writes into saved views whose target interval is still the
2240 enclosure. The hook checks the actual kind70 death, zero HP, native2560
value, stage7/section1, live simulation scope and actor-pool alignment. It also
works during a legitimate P2-targeted actor update. Other camera fields,
origins and unrelated room intervals remain unchanged; native AI, death and
map edits still run once. No new persistent snapshot fields are introduced.
Both configs declare the checked A42219D4 instruction; main generated shard39
was regenerated. This fixes166, not the separate capsule/death report167.

| Check | Evidence |
|---|---|
| Counterexample | A unit models the two ordinary native stores even without the new hook. Original code fails the saved-view-byte assertion; candidate passes15 cases, including projected actor context, out-of-scope rejects and whole-view byte preservation. |
| Portable check | The same Split fixture passes MinGW and Linux GCC. Its two pre-existing misleading-indentation warnings remain; no cross-OS gameplay compatibility is inferred. |
| Real Split, X/P1 Zero/P2 | Both final-hit owners complete the original death and following movement. Both saved maximums become2560, both heroes stay visible32HP/unlocked, and matching peer state is retained through8197. |
| Real Split, Zero/P1 X/P2 | Both final-hit owners also pass, including a P2-nearest projected death update. Native release counter records both views for each death; matching peer state through7972. |
| Local Unified | The source-owned storm_midboss_exit case passes both finishing owners. Both heroes proceed past2780, reach native checkpoint2, and settle healthy/visible/idle with no personal locks. |
| Windows title suite | All35 tests pass in one14.76s run; build/test logs are owl-midboss37-all-build.log and owl-midboss37-all-tests.log under build-pr23. |

Only setup uses invulnerability, observed safe positions and the existing
synchronized enemy-HP fixture6. Native attacks supply the final hit, and the
original death animation fully retires before the tested exit. The scenario
is available as tools/test_mmx4_coop_regressions.py --cases storm_midboss_exit
for local or an attached NetplayRuntime; private two-peer captures also retain
the distinct saved views and CRC checkpoints.

Excluded attempts remain separate: an initial probe expected the wrong
actor73; a headless present-shot request failed; a network damage-multiplier
setup did not finish combat. Those failures do not count as reproductions or
acceptance. The later baseline explicitly waits for complete corpse retirement
and records both saved camera intervals. The local pre-fix control already
allowed P2 to follow; this was a Split camera release fault.

Private native inputs, observations, screenshots, source-script snapshots,
executable digests and an archive manifest are retained in
build-pr23/followup36/qualification37/owl-midboss-controls.zip with a separate
size/SHA256 receipt. The main checklist now has139 checked/34 unchecked of173.
Item157 and the remaining progression/feature scope remain open; no release
is qualified by this correction alone.

## Further native slash controls: no acceptance of157

The development checkpoint whitelist now admits all four verified Frost part2
spawns. Its pointer list occupies800F417C..800F4188; the next section's list
starts800F418C. Checkpoints2/3 point to800F3674/800F3698. The original stage's
boss checkpoint3 now loads through the ordinary initializer, with both heroes
healthy. This is development setup support, not a gameplay/slash correction.
Actual combat in both the original stage and fortress uses live kind57; a
probe that waited for kind56 in the original stage never began its trial.

| Control | Observation | Limit |
|---|---|---|
| Unified, P2 upgraded ground-to-jump cancels | Ten native blade initializations in each fresh fight. Scaling off accepts3 damage events; current Separate accepts10. Captures show ground and spinning blades. | Does not reproduce scaling-on being worse. Initial AI substates differ. |
| Unified, sustained/double-jump schedule | OFF has1 initialized spinning blade/1 hit; current ON has2/2. | Native hit pause changes the later attack state despite the same scheduled buttons. This is not a matched blade-lifetime experiment. |
| Private local-only removal of the lifetime veto | With the existing four-tick recovery unchanged, the cancel schedule admits30 hits from10 initializations; the double-jump schedule admits13 from2. | Demonstrates that deleting this guard alone changes repeated damage and aerial hit pause substantially. Does not establish the owner's failure or qualify a fix. Never used in netplay. |
| Actual two-peer Split, X/P1 Zero/P2 | Six requested jump/slash cycles per skill set. Current Separate accepts6 basic and6 upgraded hits, both heroes healthy. OFF has3 and4 health-decreasing cycles. Native traces include27/basic and34/spinning blades. | Authoritative host inputs and synchronized setup; wait-tick/capture overhead remains. Not an exact source replay. The unfiltered write dump limit also omits later individual stores, so use per-cycle health/counters rather than claiming a complete store trace. |
| Original Frost stage, local Unified | OFF accepts27 then24 (48→44); current ON accepts three27 aerials (48→45). The inspected video shows Frost moving out of the later fixed-facing swings. | Additional attacks/initializations are not proof of additional overlaps. Again no ON-worse failure. |

The real Split peers retain matching core/mod checkpoints through9539 with
scaling enabled and9475 disabled. Both option states are verified by the native
scaling activation counter. Guest input_route was not used to bypass network
input ownership. No boss positions, timers, hit tables or attack outcomes were
written during these combat trials.

Rejected setup attempts remain excluded: the old whitelist rejected original
checkpoint3; the kind56 predicate never found the live fighter; an exact
ground-Y predicate missed the one-pixel character-root difference; several
setups requested a teleport before P1's post-intro arrival was complete. The
final local setup waits for both healthy, visible, idle heroes after the actual
fight starts. These are not reported as game reproductions. A second private
half-native-timer variant also stopped in setup and supplies no gameplay result.

The idle Windows suite passes all35 checks in2.20s. An earlier concurrent run
had a detached logging child exceed its10s fixture deadline (34/35); it is
preserved alongside the clean idle run. The source change here only broadens
the verified development spawn list.

Private receipts, inputs, captures, script snapshots and variant provenance
are retained in qualification37/slash-controls-continued.zip with a separate
size/SHA256 receipt. The silent6.87s comparison video is
build-pr23/slash-comparison37.mp4: it explicitly labels fresh fights with
different boss movement, not a deterministic matched-world comparison.
Item157 remains unresolved; checklist139/34 is unchanged. No release is cut.

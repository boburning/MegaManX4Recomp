# Playtest36 recurrence review

The owner reports that scaled Zero slashes remain wrong in36. Item157 is
reopened. The Playtest36 implementation and its tests do **not** establish
that the reported scaling-off versus scaling-on difference is resolved.
The corrections below have bounded native qualification recorded separately;
they do not qualify the entire release while157 remains unresolved.

## Intended scaling behavior

- A boss requires twice its original total damage to defeat.
- Each player owns their damage cooldown. A hit by X must not impose Zero's
  cooldown, and a hit by Zero must not impose X's cooldown. Both can be active.
- Cooldowns are shorter than the original game: the owner's requested starting
  rule is half the native duration, not half the attack rate.
- Normal grounded combos, aerial slashes, cancellations and weakness reactions
  retain their native meaning. Scaling must not add a new restriction on which
  connected attacks count. Native invulnerable attack/story phases remain.
- The original balance remains available when scaling is disabled. The
  alternate reduced-shared method remains a separate selectable policy.

An illustrative native32-frame cooldown would become16 frames for the
attacker, while the partner remains eligible. This example does not assert
that every boss/attack uses32, or that a timer alone defines native admission.

## What the current code actually does

`mmx4_coop_balance.c` halves accepted damage with a fractional carry, retaining
native displayed HP ranges and phase thresholds. It intercepts selected native
post-hit cooldown stores, tracks per-seat countdowns, and gates accepted
overlaps in8002DE30. Native collision cleanup at8002DD04 still runs.

In Separate mode the native shared timer receives the smaller private timer,
with a minimum of one tick for native expiry cleanup. Boss hit handling also
uses duplicate-contact keys, damage/vulnerability tables and native reaction
states. Private timers alone therefore do not constitute independent native
hit handling.

Playtest36 additionally caps recognized Zero blade recovery at four boss
updates and records one damage admission per native projectile lifetime,
target, health bank and seat. That lifetime limit is an implementation choice,
**not an owner requirement**. It must not be treated as correct merely because
tests enforce it. Native8002DE30 rejects a repeated nonzero contact key;
8002DD04 clears that key when no contact remains. Native800981AC also changes
blade contact keys. Consequently one projectile lifetime is not, by itself,
proof of one native damage opportunity.

Fractional damage can produce an accepted hit without changing an integer HP
byte. Whether that affects the particular reaction being reported is still a
hypothesis, not an established cause. Native damage returns and reactions must
be inspected as well as HP. Replacing it with literal doubled HP would require
preserving native phase thresholds, health bars, multi-pool bosses and death;
it is not a safe one-line replacement.

## Why the prior acceptance was insufficient

The earlier source-cadence probes transplanted controller timing into fresh
native fights. They did not reproduce the tester's exact encounter. They
showed that some attacks connected, and that selected counters/timers matched
the implementation. They did not show that a connected native attack blocked
by scaling became equivalent to its unscaled control.

In particular, the prior fresh scaling-off control accepted fewer attacks than
its scaling-on counterpart. That did not reproduce the owner's observed
contrast and should have prevented closing157. Improved hit totals in another
moving fight, synthetic held-blade protection and passing unit tests do not
repair this missing comparison. Historical results remain evidence for their
bounded observations, not acceptance of the reported experience.

The review must follow attempted connected attacks through actual overlap,
native contact key, private cooldown, native vulnerability table, accepted
attack return, HP/fraction and reaction. Ground single slashes, full chains,
cancelled follow-ups, basic aerials and upgraded aerials need separate controls,
with Zero in either seat. Partner buster/fire overlap, weaknesses, deaths and
snapshot restoration are necessary counterexamples. Matching counters without
matching visible blade/impact/reaction behavior is insufficient.

## New evidence intake

Both ZIPs pass CRC checks and contain the published36 execution identity
`93f56a5e8f8d35fdee77384cb5e3772b5dfbd9b973dcbcd76206bba14f883dc5`
and co-op profile `4944a647210cafcb6e9c1a22f8ae2b3e9565eef1`.

| Archive | SHA256 | Relevant contents |
|---|---|---|
| Storm Owl36.zip | b21405deec1ab0731871b10ea60650729a80e0a24a6590b4e9b1539a608eb3e3 | Paired network sessions, local sessions, scaling-on and scaling-off settings, Owl and Cyber routes. |
| Opening stage Web Spider Frost Walrus36.zip | cb76eb51321af46e537d3991099b99604d563ef09f6d627e2903af09abb2d8e4 | Multiple sessions including repeats from the first archive; use recording identity rather than counting the repeats as separate reproductions. |

Both hashes match the intake receipt. Private receipts and safely extracted data are under
`build-pr23/followup36/`; originals remain unchanged. Settings select Both-player
doors and Split cameras. A local session can retain the Split setting while
the current game still uses Unified locally; the setting alone does not prove
local split rendering. Network recordings are not supported as deterministic
offline replays by the existing replay tool.

`IMG_1004.mov` is9.33 seconds. The inspected contact sheet shows P2 approaching
Cyber's course portal, then remaining beside it while the recorded frame
counter continues advancing. That supports the reported stuck progression;
it does not identify the native wait condition or prove an unbounded freeze.
The supplied [Owl](https://youtu.be/Eu1-SnJQHYo) and
[Intro/Web/Frost](https://youtu.be/vxliwk-y3OU) videos were queried. Full-media
downloads return403; public storyboard sheets are available. Do not describe
storyboard review as watching the full-motion videos.

One paired Owl session, `1792318160615607`, records P1 dying at2905/524 while
P2 is6348/426 at input20539. P2 also dies by20898. A later retry reaches part2.
The existing `storm_survivor_exit` substitution applies only when P1 is dead
and preserves the native X>=6352 test. It does not cover a living P2 reaching
the exit while P1 remains alive behind. The recorded6348 position is also short
of that threshold; a teleport beyond6352 is not proof that a walking player can
reach the trigger. These are investigation leads, not a validated new fix.

## Current burndown additions

| Item | Status | Scope |
|---|---|---|
|157 | Reopened; owner comparison prepared | Entire scaled independent-hit pipeline; earlier36 acceptance withdrawn. Visible, normal-audio Frost fight with same-image OFF/ON resets is ready for the owner; [handoff and limits](COOP_PLAYTEST37_INVESTIGATION.md#owner-playtest-handoff). No owner result yet. |
|25 | Validated correction | Native before/after leading-P2 and death/fade race checks reach part2 in local Unified and real Split LAN; see [qualification](COOP_PLAYTEST37_INVESTIGATION.md). |
|2 | Reopened | Intro P2 boss approach walks left, loses P1 and corrupts graphics; upstream missing platforms may be related. |
|107 | Still open | Web part1 spiked enemy also missing when both traverse. |
|165 | Validated correction | Preserve the original assessment completion producer through P2 portal admission. Native grade/checkpoint/movement checks pass local Unified and real Split with both rosters; see [qualification](COOP_PLAYTEST37_INVESTIGATION.md). |
|166 | Validated correction | Native Generaid Core death now releases both affected Split camera limits; both rosters/final-hit owners, local Unified control and regression checks pass. See [qualification](COOP_PLAYTEST37_INVESTIGATION.md). |
|167 | Reported | Owl Buster Upgrade followed by X's death strands Zero above; additional secondhand report, not automatically attributed to this archive. |
|168 | Validated correction | Native P1/P2 walking controls reproduce the falling-platform trigger exclusion. Either living hero now supplies its original threshold predicate; both local and real Split rosters pass. Boss-approach2 remains separate. See [qualification](COOP_PLAYTEST37_INVESTIGATION.md). |
|169 | Reported | Frost rising-ice shaft camera stays behind after P1 dies. |
|170 | Reported | Frost P2 camera graphics corrupt after the first death. |
|171 | Reported | Frost part2 slowdown after the ice bird freezes the stage; separate from the36 camera-selection fix162. |
|172 | Validated feature | Draw a second instance of the native course character-analysis graphic for P2 without a second assessment update. Both local rosters, real Split near/distant views, original portal completion and matching peers pass. See [qualification](COOP_PLAYTEST37_INVESTIGATION.md). |
|173 | Requested feature | Optional local8:3 composition, two side-by-side4:3 views; Unified stays default. |

Correctness intake is `beads-eio.9.188`, scaling remains `.177`, Cyber reticle
is `.189`, and local split feasibility is `.190`. Existing issue references
remain in the main numbered checklist.

## Local split feasibility

The existing code already maintains two native cameras and shared-world enemy
admission, with scoped render transactions to protect simulation and VRAM.
`present_local_view` currently chooses one seat from the netplay local port and
commits one view for that process. Local8:3 is therefore feasible, but needs
two view targets and a window compositor, viewport/aspect/resize handling,
per-view HUDs and appropriate shared-scene behavior. Simulate the world once;
two views must not run AI, damage, stage events or timers twice. Verify state
restoration and performance before offering it as an optional local mode.

Local8:3 is deferred for this correctness release: the presentation/compositor
work above would delay the active progression and hit-registration fixes.
The feature remains tracked in `.190`; no local split implementation is claimed.

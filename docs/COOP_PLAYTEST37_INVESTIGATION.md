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
and LAN receipts currently live under
`C:/Users/Matthew/AppData/Local/Temp/mmx4-feedback37/` and must also be archived
before the release qualification is finalized. Failed/insufficient setups
remain distinguishable from passing evidence.

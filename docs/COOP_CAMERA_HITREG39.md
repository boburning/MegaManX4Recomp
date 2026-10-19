# Renewed camera and saber investigation for Playtest39

The38 reports reopen157 and the later Frost shaft investigation. The owner's
earlier37 Frost comparison was bounded acceptance, not evidence that every
reported slash sequence or camera mode was corrected. The renewed checks below
separate actual reproductions from the still-unconfirmed single-slash report.

## Later Frost rising-platform shaft

The supplied timestamp is17:17 in `vxliwk-y3OU`. Its diagnostic frame is the
first sequence column in `inputs.csv`, not the second guest-frame column.
The relevant36 session reaches worldX near5500; previous opening-ice checks
around2700 did not cover this shaft. Native checkpoint3 is before it.

An original-disc local8:3 reproduction exposed two distinct omissions:

- Controller15 opens the landing by changing maxX5312 to5728 at800B8D64.
  P2's saved view retained5312, clamping the healthy body at5624 over the pit.
- Its phase-entry routine800B8D50 also changes the camera's bottom follow
  dead zone from192 to112. P2 retained192: its cameraY stayed1258 while the
  platform carried both bodies and P1's camera followed their ascent.

The candidate publishes the matching landing limit and evaluates each view's
follow phase using800B8E74's original signed position tests and threshold
table. Each view retains its own origins, other follow settings and room
limits. The canonical owner's follow setting also remains its own when the
partner leads. The shared controller runs once, supplied the living leader's
XY; its world/layer phase is never replayed per camera.

The `frost_shaft` development scenario loads the real checkpoint, prepares
positions and incoming-damage immunity, then uses ordinary Right input on
native moving platforms. Terrain, platform clocks and crush deaths remain
native. It checks both leading seats and a genuinely dead P1, stopping during
a healthy ascent before the authentic destructible ceiling. This qualifies
camera follow and landing limits, not a full stage completion.

Actual Split LAN qualification passes all three ascent cases with P2 at32HP
and matching5728 landing limits. Each captured rider's own camera ascends
with its native platform. Both peers agree on core/mod CRCs through15456
after these checks. Private receipts: `regression39/net-split/frost-shaft.json`
and its two-peer captures; the earlier local controls preserve the failing
192px dead zone and5312 limit for comparison. The first straight-Right local
scripts continued into native ceiling crushes; those are not camera failures
or full-route passes.

## Saber admission and cooldowns

Scaling simulation has no local/netplay or Unified/Split restriction. Private
render passes intentionally do not advance damage/timers. The current policy
halves registered native boss hit cooldowns, rounded up; recognized Zero saber
chains use a shorter four-update recovery. It preserves doubled effective HP
through fractional damage carry. Separate mode tracks one cooldown per seat.

That timer policy alone was insufficient. New development tracing records
actual native collision overlaps before admission, accepted damage stores and
timer installation. In a real Frost fight, a Kuuenzan blade accepted pose47,
then overlapped with poses49/50/51 while both private timers were0, the native
timer was0 and Frost's real vulnerable table was active. The co-op lifetime
veto rejected all of those contacts solely because the same blade had hit.

The correction limits initialization tracking to fresh blades that reuse a
previous contact key. Existing blades retain the original native DE30
duplicate/pose checks without a key reset. A held identical contact remains
rejected; changed native poses can be accepted after private recovery.
Cooldown constants, durability and AI are unchanged by this correction.

Both-seat regression checks exercise repeated held-key queries, original
DE30 duplicate rejection, accepted changed poses, interleaved blade slots,
snapshot restoration and fresh initialization. Original-disc saved-fight
comparisons additionally record exact selected-pad input and identical
neutral setup before OFF/ON trials; hit totals alone are not acceptance.
Captured native attacks and impacts are inspected alongside overlap reasons.

Native local Unified compares348 recorded pad frames from one saved fight.
The candidate accepts changed Kuuenzan keys47/49/51 after recovery, while
held unchanged-key queries keep the native duplicate token. A local8:3
Zero/P1 comparison qualifies460 pad frames and successive24/25/26 ground
contacts. Later fixed-facing attacks can fall out of range; their totals do
not establish a missed-hit defect or a fix.

Actual Split LAN adds native basic27 aerial contacts and upgraded34 contacts
while X's private cooldown is still active, and X contacts while Zero's timer
is active. Both peers record the same26 accepted stores at the same guest
cycles. This verifies independent admission in that bounded native fight,
not a promise about every boss reaction or every historic report.

Unified LAN repeats the native basic/upgraded encounter with Zero/P1 and X/P2.
Both peers agree on accepted events and core/mod CRCs through9888. Together
with local Unified and local8:3, these checks exercise the shared damage code
in all four camera/transport combinations. They retain the same qualification
limits above; no cross-OS match was run for this change.

The common collision routines, Zero's native sword update and Frost's native
damage/table/timer paths have been disassembled and inspected. The existing
registered timer stores are a selected set: this is **not** a completed
semantic decompilation of every boss or every invulnerability phase.
The latest historical single-slash report remains unconfirmed until the
reported contrast itself is observed;157 stays open.

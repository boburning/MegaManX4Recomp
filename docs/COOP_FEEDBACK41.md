# Post-Playtest40 feedback

The owner identified the new reports as **local co-op, experimental8:3 split**.
The numbered burndown remains authoritative for completed and open work.

## Opening-stage first falling object (199)

The executable's table at8010C160 contains15 records, numbered0 through14.
The existing leading-P2 predicate at800BEEF4 rejected subtype14. That is the
lower piece of the first pair atX752: the upper piece1 falls from216 to304;
the lower piece14 must then fall from304 to392 when the hero passesX592.

In the before control, P1 stays atX144 while P2 climbs the original wall with
native controller jump edges, reachesX636, and retains32HP. The upper piece
falls but the lower piece remains in state1.0 at304. Moving P1 through the
same approach advances that piece to state1.3 at392.

The fix includes14 in the eligibility check and continues to reject15.
The matched P2-only native approach reachesX642 and advances the lower piece
to392 while P1 remains144; both remain32HP. No object state, animation clock,
trigger result, tile edit or success flag is set by the test. Invulnerability
only protects the wall approach from enemies. Model checks cover all15 valid
subtypes and ineligible actors/players. The later final slab201 is separate.

Private reproducible evidence: `build-pr23/feedback41/native_probe.py`, focus
`intro-climb`; `survey-before/intro-climb-observations.json`,
`intro-after/intro-climb-observations.json`, paired screenshots and
`intro-and-diagnostics-qualification.json`. Earlier incomplete wall approaches
and an unsafe debug teleport are excluded from qualification.

## Automatic recording mode metadata (205)

`session.json` adds the human-readable release, effective initial mode and
committed co-op options. `mode-changes.csv` records changes in actual runtime
local/netplay, Unified/local8:3/independent cameras, enrollment and campaign.
It records native input sequence and game frame. Saved preferences are
included separately so a stale netplay preference cannot mislabel local play.

The native local8:3 run writes release0.1.1-coop-playtest.40 and
`local_split_8_3` while `coop_options.cameras` remains`unified`. The partner's
initialization transition is present. Windows and GCC/Linux recorder/writer
checks cover single-player, local Unified, local8:3, netplay Unified and
netplay independent labels, multiple transitions, producer nonblocking
behavior and I/O failures. Both Linux outputs parse as JSON.

Mode events use the existing background queue and do not enlarge its event
records or add per-frame filesystem calls. Input-v2 rows and replay loading
remain unchanged. No new two-peer gameplay qualification is claimed by the
recorder unit tests. New fields are additive; older recordings cannot acquire
mode metadata retroactively.

Private receipts: `candidate-tests.log`, `linux-diagnostic-checks.json`, and
the actual native recording under `intro-after/saves/diagnostics/`.

## Slash Beast follow-up

The owner reports that stage triggers now work, but backgrounds stop in both
players' views. Items191-193 are checked as **tester confirmation**, not new
native automated clearance. Background189 remains open for the broader
both-view report, with one confirmed cause fixed locally.

The actual opening-section train controller still runs when P2 leads, but
the lifecycle's uncommitted P2 camera transaction restores the old canonical
camera after scrolling has been published to both saved views. The next P1
camera snapshot then erases P1's scrolling. The native before control records
unchanging canonical/P1 near-layer origins84934656/85458944 across frames5553
through5666, while P2's origin and the native controller counter advance.

After that transaction, restore the canonical view's already-published
scenery layers. Preserve the lifecycle's foreground camera and scene decisions;
the native controller, sounds, timers and trigger admission still run once.
A composed camera/lifecycle model fails before the fix and passes afterward
for either owner. All36 Windows checks pass; GCC/Linux split and lifecycle
boundary checks pass too.

In the native local8:3 retest, both near/far layers move during all19 sampled
intervals with stationary heroes and horizontal foreground cameras. Both far
layers advance452px. P1's near origin normalizes during the window, so its
absolute delta is-632 while P2's is904; a positive-unbounded-origin assertion
was invalid. The raw failed assertion remains in the private survey receipt.
The corrected sustained-motion check also verifies the canonical camera keeps
the published origins. Before/after composed images show changed scenery in
both panes, including8488/7714 changed pixels in background-only crops; the
original2px vertical train shake remains. These images were visually inspected.
This qualifies the reproduced handoff freeze, not every later train phase or
a new two-peer session. Receipt: `slash-fixed-qualification.json` and
`slash-waterfall-final/` beneath `build-pr23/feedback41/`.

## Mushroom survivor rainbow palettes (203)

The reproduced ordering is important: P1 dies **before** P2 enters the boss
room. Killing P1 after admission misses the defect because the shared rainbow
palette has already been initialized. The before run shows black afterimages
in both panes, with P2 healthy and the ordinary boss sprite still colored.

Mushroom's native initialization calls8007B7B4 to copy64 colors from the stage
colors into its working palette. P2's actor projection substitutes hero
palette pointers and then restores the previous palette-upload flags. That
writes the rainbow colors to the wrong palette and loses the upload request.
During this boss's initialization only, preserve the canonical stage color
and palette pointers and carry the native dirty bit through the projection.
Boss AI, reaction phases and hero resource ownership otherwise remain native.

The matched native local8:3 survivor retest has colored trails in both panes;
all128 working-palette bytes match the original color source. Its before
control contains black0000/8000 entries instead. Private evidence is
`workshop-current/clones-survivor-entry-2/` versus
`workshop-fixed2/clones-survivor-fixed/`. Native fatal P1 damage and a room
position fixture prepare admission; boss HP12 prepares the low-health pattern.
Neither run forces palette colors, boss phases or an attack result.

The boundary regression fails on the old implementation and passes with the
fix. It checks both owners/campaigns and camera policies, stage3 and fortress
palette offsets, initialization versus live updates and unrelated bosses.
Windows and GCC/Linux combat checks pass. This is native local-render
qualification plus portable boundary coverage, not a fresh two-peer run.

## Remaining bounded investigations

- **Elevator185:** checkpoint2 early-ascent probes on unchanged code and the
  deferred repeated-command candidate both keep cameraY equal. The sampled
  held/double-jump control on unchanged code reaches105px relative height
  across205 observations with equal cameraY. This early-ascent check does not
  clear every later lift phase. The speculative change is removed from the implementation.
  It remains privately retained for further investigation. Authentic ceiling
  crush rules remain unchanged.
- **Waterfall204:** actual native movement from432 to726 measures343 frames
  in5.719s with both players together (59.98fps), and330 in5.484s with P1 at
  the observed safe original spawn (60.18fps). Both remain healthy. This first
  stretch does not reproduce the report or clear the full moving-log section.
  The earlier unsafe separated-position teleport is excluded. Full timings,
  body traces and background phase profiles are in
  `slash-waterfall-final/waterfall-route-timing.json`.

## New Sigma recording intake (206-207)

`Sigma stage3 bugs40.zip` has four native diagnostic sessions. The longest
contains202878 inputs, including63261 gameplay rows in the refight section.
It is Playtest40, offline local8:3, Both-player doors and boss scaling enabled.
The video recording failure does not mean these inputs are missing. Its
archive SHA256 isbac76e69799077d46c55579fa5cf31cb50716e1d1f6090b49b152071b5b17a08.

Track Dragoon's refight camera lock as206 and Slash Beast's repeated weakness
stun as207. Native room controls and scaling-on/off comparisons are pending;
input positions alone cannot establish a camera's bounds or a boss reaction.

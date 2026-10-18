# Playtest35 Frost follow-up

Tracking: `beads-eio.9.177`, `.182` through `.187`. Work branch:
`fix/frost-slashes36`. Playtest36 qualification evidence; production/package
and publication receipts are retained under `build-pr23/release36-qa/`.

## Intake

Both ZIPs pass CRC checks. SHA256:

- `Same single slash hits Frost Walrus infinite ice path.zip`:
  `0a2d23fce569a9207b463f4d390382f506ec59e617d57e272fc39821f0304c97`.
- `Frost Walrus stage bugs 35.zip`:
  `1b596fcd53ca6f2d310c63a195a64e84636e3b542b0b533d0041c931bba1b0d2`.

Both identify Playtest35's co-op build `c873d56d7163d2ee34d3e4aa66e58900ec4f4bb8`,
Split netplay, Both-player doors, scaling enabled and default per-player
cooldowns. The longer archive contains the shorter session plus a new paired
71,287/71,290-input Frost run. The short run has 18,579/18,582 native inputs,
visits outdoor Frost and its fortress refight, and ends with P2 attacking while
P1 remains nearby. An 18-input offline startup segment is not gameplay evidence.

Videos: [slash/path](https://youtu.be/Jutt5UUfkis),
[Frost stage](https://youtu.be/NbBeLU_arDQ). Full media downloads returned403;
public storyboard sheets are available. Still inspection is not full-motion QA.
Original netplay identities are retained; none is relabeled as an offline replay.

## Active reports

- 157: missed follow-up slashes persist. The tester specifically isolates boss
  scaling. Compare actual connected attacks and attack lifetimes; damage totals
  alone are insufficient.
- 160: later ice path does not collapse for P2. Native solid type17's C41C8
  geometry loop calls2C160 against only PLAYER. Contact creates one native
  type21/subtype4 stage controller, starting the authored collapse sequence.
  Extend that contact test, not the platform clock or a guessed expiry timer.
- 161: corrupt enemy sprites with P2 leading in part2.
- 162: ice-bird freezing effect focuses distant P2's view on P1.
- 163: ice birds do not respawn when trailing P1 arrives.
- 164: post-second-door partner warp, possibly native first-entry dialogue.

Private intake: `build-pr23/feedback36/`. Native and production qualification
results will be recorded here as each report is resolved or bounded.

## Scaling slash correction

The owner confirms the symptom disappears with scaling off. The separate
cooldown policy still gave finishers and aerial blades a halved native64-tick
recovery (32), while opening grounded strikes received4. Contact-history cleanup
alone did not shorten that finisher/aerial lockout.

The candidate stamps native sword initializations at80097EEC. Known grounded,
finishing, dash and aerial blades receive4 boss-update ticks of recovery.
Admission also remembers each live native blade slot per target, health bank
and player: changing a spinning blade's pose or interleaving another slot does
not allow that blade to hit the same target repeatedly. Fresh blades may reuse
native contact keys after recovery. Non-melee attacks retain the existing half
cooldowns and elemental reaction path. New persistent history participates in
snapshot validation and the simulation digest.

The tester's P2 ground-combo input excerpt18052–18290 was transplanted into a
fresh original-disc Frost refight. This is controller-cadence transplantation,
not deterministic playback of the netplay session. The prior gameplay build
accepted9 native HP stores; the candidate accepts all12 sword lifetimes, including
the first strikes of the third and fourth chains that were previously blocked.
Inspected captures show native swings, impact effects and boss recovery.
Damage totals alone are not the acceptance criterion.

The final candidate's upgraded aerial trial admits7 separately initialized
Kuuenzan blades in260 frames. A single aerial spin initializes once and damages
once over260 frames despite repeated overlap. These are live native input tests
with positions and invulnerability set only before each trial. No hit outcomes,
boss timers or attack states are edited during the trials.

All35 Windows title tests pass. Added counterexamples cover both owners, native
blade reuse, changing held poses, simultaneous blade slots, recycled targets,
snapshot restoration and projection restoration. Native tests use hidden,
muted private processes and separate cards.

## Frost qualification limits so far

The first world probe teleported P2 directly into the gap before the ice had
formed, and another setup point placed him in solid tiles in part2. Both caused
native deaths; the subsequent rejected teleport was a dead-player fixture, not
a reproduction of the submitted enemy-graphics issue. Those failed receipts
are retained and are not counted as passing gameplay validation.

Native ice-bird code at8004D160 retires completed birds via8002B13C, retaining
their placement flags. Its separate offscreen-return branch at8004D19C calls
8002B0C8 to clear those flags. Thus a completed bird need not respawn for a
trailing player; blindly clearing visited records would create extra enemies
and repeat the environmental effect. Native field controller800B913C sets
PLAY+10 through+13 while its fade runs. Treating PLAY+10 alone as a request to
show the canonical camera explains the reported temporary camera focus.

The corrected Split LAN path approach uses the observed safe ledge3246/361,
waits for native formation, then crosses using controller input. P1 stays at
X144 and both players retain32HP. P2 alone starts collapse controllers for
segments8 and9, followed by the native retirement state for8. Core/mod CRCs
agree through4773. This passing path phase is separately asserted in
`build-pr23/feedback36/path-phase-qualified.json`; the aggregate run's later
unsafe part2 fixture remains a failure.

Further qualification exposed two fixture limitations: source positions can
lie inside ice blocks already destroyed in the original run, and checkpoint1
in Frost part2 is an early checkpoint, not the final boss antechamber. An
early input experiment also mixed host frame counts with native update counts;
it is discarded. None qualifies the reported enemy corruption or second-door
warp. Repeating invalid setup is not evidence those reports are fixed.

Development netplay now accepts `frost-field 0..3` through the existing
synchronized packet helper. This creates the original type17 stage controller
only in Frost part2; its native fade, edits and lifetime execute afterward.
Production builds exclude the entire command path. This bounded event fixture
allows presentation qualification without skipping the level's camera-region
triggers. It is explicitly separate from end-to-end bird-encounter replay.

## Final native qualification

The synchronized field fixture passes in real two-peer Split LAN. Between two
observations with the native freeze flag still set, P1 renders7 own-view passes
and P2 renders8. Captures retain the distant X and Zero cameras and their own
HUDs during the field's bright fade and afterward. There are no guest-state or
VRAM verification mismatches; both players finish at32HP and peer core/mod CRCs
agree through5059. This qualifies the presentation fix under the original
native field effect, rather than claiming an exact replay of the supplied run.

The corrected native-clock input excerpt also drives P2 fromX120 toX693 through
the early part2 ice blocks with P1 stationary atX96. Brown flying enemies, ice
blocks, particles, Zero's body and saber render normally in the inspected
captures. That bounded section does not reproduce161 and does not clear the
entire later-stage report. Enemy corruption161 and first-entry door warp164
remain unchecked for a more specific capture/timestamp.

Zero as P1 also completes fresh ground chains and basic aerial slashes against
the original Frost refight. Both native blade classes27 and34 therefore have
gameplay qualification in addition to the both-seat lifetime unit coverage.
The P1 ground trial accepts11 initialized blades. Its moving basic-air trial
initializes9 blades of classes27/28, with6 accepted hits; this is not claimed
as9 connected attacks or a deterministic retail comparison. The primary
recovery counterexample remains the formerly blocked fresh ground-chain
openers and the both-seat lifetime/cooldown tests.
Private receipts reside in the per-case directories under
`C:/Users/Matthew/AppData/Local/Temp/mmx4-feedback36/`; source-cadence and field
reports are archived under the private `build-pr23/feedback36/` evidence root.

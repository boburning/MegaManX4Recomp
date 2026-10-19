# GitHub review for local split development

Reviewed against Playtest37 (master06e940c) on 2026-10-18, with subsequent
fixes qualified on the local development branch. The changes are not yet a
public playtest release. Private attachments remain in build-pr23/issue-review38; no reporter
message or issue closure has been posted.

| Issue | Evidence and disposition | Next useful check |
| --- | --- | --- |
| [41](https://github.com/mstan/MegaManX4Recomp/issues/41): intro gate explosions never finish | Initially unconfirmed because only the old startup attachment had been reviewed. Now attempted on current code: native gate traversal succeeds in both X/Unified and Zero/local Split campaigns, with both players healthy, checkpoint2 reached, maximum X released to4544 and control unfrozen. A further X/Unified attempt also completes Dragoon's dialogue/rescue and loads playable part2 with both heroes. | Keep open as not reproduced, not fixed. A full current session recording of the failure is still needed to identify its cause. The old log alone cannot establish it. |
| [39](https://github.com/mstan/MegaManX4Recomp/issues/39): Cyber weapon and Space Port/Final Weapon backgrounds | Reproduced and fixed. The beam updater read Zero/P1 while X/P2 owned Aiming Laser; two native background controllers updated only the canonical camera after private camera snapshots. Actual two-peer Split baselines fail and the corrected native cases pass. | Qualified locally; include the fixes in the next public build. See implementation and evidence below. |
| [32](https://github.com/mstan/MegaManX4Recomp/issues/32): missing X hover jets | The latest supplied sessions match31 (a2ed2a04). Checklist149 and3ac1021 document the actual native effect33 owner correction, released32. Current37 includes it; prior native X/P2 hover qualification matched the initialized X/P1 blue/white jets, followed the owner and expired on landing. | Ask for a newer recording only if this exact symptom remains; do not re-open149 solely from31 evidence. |
| 32: Slash Beast background and peer death effects | The video frames show both views and the peer's missing death burst. Existing train per-view code is not proof that this particular surviving-peer phase works. The frames alone cannot establish animation cadence. Neither remainder is qualified fixed by the hover correction. | Replay the paired31 encounter or a bounded equivalent on37; compare actual death-effect frames on each view and train layer phase while the other seat is stationary/dead. Keep open. |
| 32: diagnostics cannot be disabled | The diagnostics manifest explicitly says the netplay co-op playtest profile includes recording automatically; coop_inputs calls the recorder on netplay even when the standalone diagnostics package is disabled. This explains why unchecking the separate mod does not disable netplay recording. | Describe this as current playtest policy, and improve the setting text if it implies that this checkbox also disables mandatory netplay diagnostics. A display-only preference can be considered separately. |

Attachment review used eight evenly spaced decoded frames from each supplied
video plus archive metadata. These contact sheets establish visible states;
they do not substitute for a complete motion/cadence comparison or an actual
current-build reproduction. Each ZIP contains many historical sessions; always
select the session matching the reported stage, seat and build profile before
running a replay.

## Issue39 implementation and validation

Aiming Laser uses a private type6 shot for its reticle, then968A4 creates a
separate shared type9 beam in801435B0's32-by60-byte pool. Its nativeD67DC
update checks PLAYER+93 for weapon6 and takes its origin from PLAYER. Restored
Zero/P1 makes that check fail, so the beam disappears before being drawn.
The new bounded filter projects X/P2 for that one deferred update. Native
lock acquisition, target health checks, geometry, flicker and expiry remain
native. X/P1, unrelated types and other actor pools bypass the filter.

Space Port's kind7/DA878 controller adds native scrolling speed and half the
foreground displacement. Final Weapon's kind6/DA7C0 controller changes a
four-cell atlas every eight ticks. Both execute after the saved player-camera
updates, and mode5's ordinary parallax helper does not recreate their writes.
Run each controller once, then publish the resulting animation to both saved
layers. Scrolling retains each view's own foreground displacement and Y;
atlas animation shares the actual native phase and dirty metadata. Neither
fix advances enemy AI, the animation clock or actor allocation twice.

The source and release TOML hook lists include all three callbacks. The
recompiler regenerated full_64/full_65; generated code was not hand-edited.
The first full test run caught the missing hook-list entries, which were
corrected before final native qualification.

Private evidence: build-pr23/issue-review38.

- lan-baseline: real two-peer Split, Zero campaign. Stationary P2's Space Port
  background remains24805376 while P1 scrolls. Final Weapon P2 remains0 while
  P1 changes through all four atlas phases.
- lan-aiming-baseline: real two-peer Split, healthy X/P2 locks onto the native
  Jungle plant, but no beam survives. Jungle supplies a repeatable target;
  its health is increased to24 through the synchronized setup fixture so
  target death cannot hide the beam lifetime. Both players retain32HP.
- x-control-gate: native X/P1 control locks the plant and produces its beam.
- lan-qualified: both saved views scroll in Space Port and cycle through
  0/512/1024/1536 in Final Weapon. X/P2's native beam survives and appears in
  the captured green-beam frames; native beam function traces agree on both
  peers (66 matching entry records each, including interception plus the guarded
  native call;33 world updates). The two peers match core/mod checksums after each
  case, through ticks4199,5573 and6951. These are same-build Windows loopback
  matches, not cross-build or cross-platform compatibility claims.
-36/36 title tests pass, including hook registration and staged mod validation.
  Linux GCC builds and runs the changed combat and camera helper tests.
  Tests cover owner restoration, unrelated actors, independent scrolling,
  signed odd displacement, all atlas phases and wraparound, native-once
  updates and private-render exclusion.

Final tested development executable SHA256:
3baf71bb3291ab1ff78ed39337d1f24ec6dbf1312c75fe1a43d5a8792b089359.
Stage/checkpoint/weapon/health protection and coordinates are setup fixtures;
actual targeting, firing and controller animation use native game logic.
An initial Space Port aiming setup lost X and is explicitly discarded as
weapon evidence. The existing Jungle rendering observation in the local split
notes remains separate; these checks do not claim to fix every visual defect.

## Issue41 current gate attempts

The reporter supplied only a startup log from the Playtest21 co-op profile.
Previously no current-build gate attempt had been made; that was why its
status was unconfirmed. Two bounded native attempts now start on the observed
floor before the original first breakable gate (stage0/section0,3100/426),
use controller movement and repeated attacks to break it, and continue into
the following explosion area:

- Zero campaign, local Split:1858 native frames; final positions4042.5/4212.5,
  checkpoint2, both32HP, freeze0, story lock0 and camera maximum4544.
- X campaign, Unified:2354 native frames; final positions4216.5/4154.5,
  checkpoint2, both32HP, freeze0, story lock0 and camera maximum4544.

The earlier attempt held the attack button continuously and did not break the
gate; it is not accepted as traversal evidence. No correction is claimed for
41 because these actual gate traversals did not reproduce the endless loop.
Keep the issue open for a failing full session, rather than conflating it with
the separately fixed intro part2 falling platforms.

A final extended X/Unified attempt (intro-rescue-pulsed) continues through the
actual Dragoon rescue and native dialogue, rather than stopping after the gate.
At frame7881 (4425 frames after setup), stage0/section1 is playable: both heroes
active and visible at144/443 and144/442, both32HP, story/freeze0, no script or
door locks. The earlier extended harness held Cross continuously at dialogue
and waited; using separate native confirm presses completes it. That harness
input mistake is not accepted as a reproduction of the reporter's freeze.

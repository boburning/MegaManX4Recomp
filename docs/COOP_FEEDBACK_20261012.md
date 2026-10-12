# October 12 co-op feedback assessment

The supplied material confirms empty startup logs, a visible P2/HUD removal in
Jungle, and the reported staircase sequence in which an apparent X charge
release has no clearly visible travelling shot. It does not establish a gameplay
cause or a fix. Repairing automatic startup capture for Playtest9 is the first
priority because both archives lack the state needed to distinguish withdrawal,
death, co-op failure, and projectile allocation from drawing failure.

Assessment: `beads-eio.9.70`. Existing checklist numbers and qualifications in
[the October 10 feedback](COOP_PLAYTEST_FEEDBACK_20261010.md) remain unchanged.
That checklist has 48 stable items, 40 checked. New evidence below does not
revalidate earlier staircase, elevator, graphics, or netplay fixes.

## Sources and limits

All times below are elapsed source-video times. Frame sampling can narrow a
visible transition; it does not identify its original game tick or input.

| Source | Reviewed material | What it establishes |
| --- | --- | --- |
| `E:/Downloads/IMG_0786.mov` | 9.667 seconds, 480×854, 30 fps; phone recording of the rotating staircase | Both bodies stay on screen; a charge-like effect ends with no clear travelling shot in the visible area. Perspective, overlapping bodies and a cropped HUD limit interpretation. |
| [YouTube YTcB2JQEFdM](https://www.youtube.com/watch?v=YTcB2JQEFdM) | Downloaded video, 116.228 seconds, 1280×720, 60 fps; title `2026 10 12 10 05 44` | Startup through loading a save, selecting Jungle, repeatedly firing/cycling weapons at the opening waterfall, and removal of P2's body/HUD. |
| `E:/Downloads/New Compressed (zipped) Folder.zip` | CRC-valid archive; 3 files | Three `diagnostics/startup-*.log` files, each zero bytes. |
| `E:/Downloads/Diagnostics.zip` | CRC-valid archive; 15 files | Fifteen `diagnostics/startup-*.log` files, each zero bytes; includes the same three names as the smaller archive. |

Neither archive contains `inputs.csv`, session metadata, starting memory cards,
settings, or mod state. No REC indicator is visible in the reviewed gameplay.
This means an input recording was not supplied; it does not prove that Playtest
Diagnostics was enabled or that the input recorder failed. Offline replay of
these ZIPs is impossible.

At YouTube 00:02, Explorer shows the folder
`MegaManX4Recomp-v0.1.1-coop-playtest.8-windows-x64` with the game executable
selected. This corroborates the reported Playtest8 package folder, but neither
the video nor the empty archives verify an execution ID, binary hash, or exact
running build. Execution metadata and freeze dump filenames are visible in
Explorer but their contents were not supplied. The launcher at 00:04 verifies
the NTSC-U disc `SLUS-00561` and shows Player 1's connected Xbox controller.
Actual Mods selections are not shown in the reviewed startup frames. The
gameplay HUD identifies X/P1 and Zero/P2; their armor/palette appearance must
not be treated as captured mod configuration. Network gameplay is not shown.

Private audit and frame artifacts are in the ignored
`build-pr23/feedback-20261012/` directory: `archive-audit.json`,
`youtube-YTcB2JQEFdM.info.json`, the downloaded `.webm`, `img0786-sheet.jpg`,
`img-shot-sheet.jpg`, `img-release-agent-sheet.jpg`, `yt000-sheet.jpg`,
`yt040-sheet.jpg`, `yt080-sheet.jpg`, `yt-weapons-sheet.jpg`,
`yt-disappear-sheet.jpg`, and `yt-transition-agent-sheet.jpg`. Source media and
private captures are not repository artifacts.

## Timestamped observations

| Source/time | Observed | Qualification |
| --- | --- | --- |
| YouTube 00:20–00:24 | Continue leads to Load Data; memory-card slot 1 and Data 1 are selected. | A native saved game is loaded. Its contents are unavailable for replay. |
| YouTube 00:34–00:42 | Jungle is selected, READY/arrival completes, and both HUD groups appear at the opening waterfall ledge. | This footage supplies a concrete scene preceding the disappearance; it is not the previously reported boss-door scene. |
| YouTube 00:42–01:48 | The characters remain near the starting ledge while P1's weapon icon/energy changes repeatedly and several shot/charge effects are visible. | Weapon cycling and attacks are visible; exact button sequences, selected weapon IDs and internal allocations are unknown. |
| YouTube 01:26–01:28 | Large green triangular/cone geometry extends from the character while P1 has a magenta diamond weapon icon. | Appearance only. This may be an authored aiming/scan effect. A matching native comparison is needed before calling it corruption; it is not a confirmed bug. |
| YouTube 01:48.43–01:48.47 | P2's lower HUD is still full green at the earlier sample and gone by the later sample; the partner body is also no longer visible. P1 continues firing a visible green shot in the same scene. | Removal is visually established. No death sequence, health depletion, room transition or camera relocation is apparent immediately before it. Manual withdrawal or a co-op failure remain possible without inputs/logs. |
| YouTube 01:48.50–01:54.50 | P1 remains visible and active with the upper HUD; P2's HUD does not return in these samples. The recording is black by 01:55.00. | Persistent removal across the remaining visible gameplay is established. The footage does not establish why P2 left, whether a rejoin input was attempted, or permanent failure beyond this recording. |
| MOV 00:00–00:05 | Both bodies overlap on the rotating staircase; a brief jump and return are visible. | This is useful location evidence. Phone perspective and overlap do not qualify the older staircase-height report. |
| MOV approximately 00:05–00:07.02 | A charge-like glow grows/flashes around the overlapping bodies, then ends. | Charging is visually suggested; an actual attack-button release is not recorded. |
| MOV approximately 00:07.02–00:07.08, then to clip end | An apparent right-facing buster pose follows the glow; no clear projectile travels through the visible area. Both bodies and partially cropped HUD groups remain present. | Supports the owner's missing-shot report as a visual symptom. It does not prove whether a shot spawned, was immediately retired, failed to draw, or could damage a target. |

## Burndown

| Order | Issue / status | Evidence and next useful action |
| --- | --- | --- |
| 1 | Empty automatic startup capture — existing item48 / `beads-eio.9.71`, fixed and qualified for Playtest9 | Confirmed by both archives and independently reproduced using the actual public8 executable. Under a detached/no-console Windows launch, the stderr FILE reports `_fileno=-2`; replacing fd2 leaves that FILE unattached, so stdio writes fail while begin reports success. Reopen the actual FILE and attach raw/native stderr, check header/location writes, and report capture failure. The production candidate captures startup and later application errors while preserving redirected stderr. Nine Windows logging cases, all31 title checks and four Linux cases pass. Published Playtest8 assets remain unchanged; Playtest9 is not yet published. This is a logging fix, not a gameplay fix. |
| 2 | P2 plus HUD removal — existing item31 / `beads-eio.9.61`, deferred and unresolved | New footage narrows the scene to Jungle's opening waterfall after sustained weapon cycling, with the visible transition near 01:48.45. Retain this evidence with the existing disappearance family; common cause with the earlier boss-door, midboss and intro reports is unproved. Once capture works, reproduce the shown saved-game sequence with inputs and presence/disable reason logs before choosing a gameplay fix. |
| 3 | Missing visible staircase shot — new item49 / `beads-nbnc`, open | New distinct symptom; keep separate from the floating-staircase and elevator reports. Use a matching recorded staircase route/settings to inspect ordinary and charged X shots with each seat, then projectile spawn/update/draw and damage against a valid target. The supplied footage qualifies only the visual symptom. |
| 4 | Green cone appearance — comparison only, no new bug issue | Compare the same weapon and armor state against native solo rendering if this appearance is reported as wrong. Do not classify an aiming effect as malformed geometry from its size alone. |
| — | Existing items34/36/46 and online items30/32/42/43 | No new matching evidence establishes the previously reported teleporter corruption, floating pickup/feet, later elevator failure, WAN boot failure, independent-view switching, or performance cause. Their existing qualifications and unresolved limits remain intact. |

Existing source triage identifies a co-op failure flag that can suppress P2's
control/HUD, with malformed actor packets and texture-bank/cache exhaustion as
possible paths. None of those paths is observed in the supplied material.
Intentional withdrawal is another route to disappearance. A large weapon effect
before removal does not prove that it caused a packet or cache failure.

## Reproduction sequence supported by the video

1. Launch the matching Windows package with X/P1 and Zero/P2; preserve the
   original armor, display, co-op and controller choices if recovered.
2. Use native Continue, load memory-card slot 1 / Data 1, then select Jungle.
   Do not substitute the previous boss-door or secret-waterfall test route.
3. Complete the normal arrivals and remain near the opening waterfall ledge
   with both players alive. Repeatedly cycle P1's weapons and fire/charge as
   shown from roughly 00:42 onward; keep P2 present through the sequence.
4. Record both seats' actual inputs, including Select, and capture presence
   transitions, co-op disabled reason, player health/position and resource
   counts while approaching the removal near 01:48.45.
5. Separately reach the native rotating staircase for the MOV report. Repeat
   ordinary and charged X shots with the bodies separated for observation,
   then with the overlapping arrangement shown in the clip.

This is a route recipe, not a deterministic replay: the exact starting cards,
settings, mod configuration and input timing are missing. This assessment did
not launch gameplay, change gameplay source, or qualify a gameplay fix.

## Logging fix validation

The same final Windows fixture compiled against the old logger reproduces
zero-byte logs with genuinely unattached FILE=-2, absent/null/invalid native
handles. The repaired logger records its header and over20KB of subsequent
stdio, raw fd2 and native Windows writes while still running; redirected
stderr retains those messages. Failure cases reject unusable capture/location
paths and expose error text instead of a stale success pointer. The detached
pointer-failure test additionally asserts the actual error message.

Independent production application checks use private copies, hidden windows,
and isolated settings. The public8 binary produces0-byte logs with both
DETACHED_PROCESS and CREATE_NO_WINDOW and unredirected stdio. The repaired
production candidate writes669/670 bytes containing its header, main entry
and a deliberate real renderer rejection. Its redirected case writes671 bytes
and preserves original stderr. A detached hidden launcher has440 bytes at
startup, grows to1355 after a deliberately refused loopback connection, and
exits normally0. This validates failure logging, not the parked online-startup
or Discord authentication reports.

All31 Windows title checks pass; four Linux logging cases pass. A separate
read-only adversarial review found no blocking correctness issue; its
pointer-error visibility gap was addressed in the retained test. The inherited
file-handle fixture qualifies redirected-stderr preservation, not an
interactive console window. Private reports are
`feedback-20261012/package-logs-before/report.json`,
`feedback-20261012/package-logs-after/report.json`,
`feedback-20261012/package-logs-live-after/report.json`, and
`startup-log-investigation/retained-before-after.json` /
`linux-focused-report.json`, beneath `build-pr23/`.

This fix is compiled in `build-playtest9-logfix-production/`, with development
controls off. The package version remains Playtest8 until a new release is
prepared. Logger fix commit: `ea81d58`. No existing public release files were
replaced.

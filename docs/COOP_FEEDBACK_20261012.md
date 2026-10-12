# October 12 co-op feedback assessment

Automatic startup capture is corrected and production-qualified for Playtest9.
Independent native follow-up now reproduces the Jungle P2/HUD removal and
identifies a reused weapon slot's stale graphics pointer as its rendering cause.
The source fix is implemented; native validation after the fix is pending.
The staircase missing-shot report remains unreproduced after bounded native
attempts. The owner authorizes Playtest9 after the disappearance fix is validated.

Assessment: `beads-eio.9.70`. Existing checklist numbers and qualifications in
[the October 10 feedback](COOP_PLAYTEST_FEEDBACK_20261010.md) remain unchanged.
That checklist has 49 stable items, 40 checked. New evidence below does not
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
| 2 | P2 plus HUD removal — existing item31 / `beads-eio.9.61`, reproduced; fix validation pending | QA111/112 reproduce the removal at Jungle's opening waterfall with both players at HP32 and failure8 (`actor-texture`). Native R1-only cycling in QA115 confirms the same failure without the weapon-selection fixture. A reused buster slot retains Gravity Well's compressed pointer even though its native flags select static artwork; the private renderer incorrectly decodes that stale pointer. Rendering now follows the native flags. Keep item31 unchecked until native validation after the fix; the earlier boss-door, midboss and intro reports are not separately qualified by this occurrence. |
| 3 | Missing visible staircase shot — new item49 / `beads-nbnc`, deferred and unresolved | QA113 shows ordinary and three charged travelling X/P1 shots on the green and purple stairs. QA114 shows ordinary and three charged X/P2 shots on the purple stairs. Its first-section hurt-lock setup is excluded. The supplied missing-shot state remains unreproduced; no projectile damage was measured. Keep separate from staircase-height and elevator reports. |
| 4 | Green cone appearance — comparison only, no new bug issue | Compare the same weapon and armor state against native solo rendering if this appearance is reported as wrong. Do not classify an aiming effect as malformed geometry from its size alone. |
| — | Existing items34/36/46 and online items30/32/42/43 | No new matching evidence establishes the previously reported teleporter corruption, floating pickup/feet, later elevator failure, WAN boot failure, independent-view switching, or performance cause. Their existing qualifications and unresolved limits remain intact. |

The original video alone cannot distinguish intentional withdrawal from co-op
failure. The independent native reproduction below establishes the latter path
for the matching Jungle weapon-cycle scene. Its failure occurs at 271–281 sprite
banks, excluding the 4096-entry cache limit for this occurrence. The green aiming
cone is not the diagnosed failure.

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
settings, mod configuration and input timing are missing. The initial media
assessment did not launch gameplay; the native follow-up is recorded below.

## Native follow-up and pending disappearance fix

QA111 and QA112 load a generated native all-clear card through Continue, select
Jungle, then cycle P1 weapons0–8 while charging/releasing at the opening ledge.
Returning to the buster reproduces the video's visible P2/body-HUD removal.
Both players retain HP32 and the co-op failure code is8 (`actor-texture`), with
271–281 sprite banks rather than cache exhaustion. These runs initially use
development fixture7 for weapon selection.

QA115 removes that fixture: native R1 input alone cycles all nine weapons,
followed by a charged buster release after returning to weapon0. The same
failure occurs at frame2845. The rejected P1 shot is `801406F8`, frame38,
CLUT`7803`, page`002A`. Its retained table is `9F02BEF4`; original table
`801A47A4` minus native sprite base `801789A0` is offset`2BE04`, PL00's
compressed family9 used by Gravity Well. Native and immutable frame entries
agree (`0CA98711`), so this is not disagreement between the archive and live RAM.

The original buster initializers at `80092328` and `80092840` set actor flags
`21` without clearing a reused slot's `+38` pointer. Native drawing at
`800244D0`/`8002452C` uses flags`40`/`20` to select static artwork and map its
UV/page. The private renderer classified any pointer within the retained
compressed range as compressed, then interpreted the stale Gravity Well entry
as a buster frame. Texture rejection set failure8, disabling P2's control/HUD.

The implemented fix follows those native flags: static actors ignore stale
compressed pointers and use the existing immutable raw-artwork path, including
its original UV/page and owning-character palette. Parent-table fallback and
compressed remapping remain available for actors that use compressed artwork.
The fix does not clear native actor RAM, suppress an error, or introduce an
opt-in setting. Item31 remains unchecked until native validation after the fix.
The retained `weapon_cycle` scenario uses native R1 input, two cycles of all nine
weapons, and frame-based charge/release windows; it supports either X seat.

The separate item49 attempt remains unqualified as a fix. QA113 visibly produces
ordinary and three charged travelling X/P1 shots on both green and purple
Mushroom stairs. QA114 produces ordinary and three charged X/P2 shots on the
purple second-section stairs; inspected capture
`stairs-1-400-1-True-2.png` shows the travelling shot. Its first-section setup
entered a development-induced hurt lock and is excluded from qualification.
Neither result proves the exact missing-shot state in IMG_0786.mov resolved,
and no damage conclusion is drawn. Item49 stays unchecked and deferred.

Private evidence: `build-pr23/qa-items31-111/`, `qa-items31-112/`,
`qa-items31-115/`, `qa-items49-113/`, and QA114's corresponding captures/report.
Other parked feedback items remain outside this focused reproduction work.

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

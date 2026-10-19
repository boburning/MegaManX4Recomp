# Local side-by-side prototype

Local co-op now has a separate Local co-op display dropdown: Unified (default) or
Side by side 8:3 (Experimental). Each pane uses a native4:3 camera and the
corresponding character's HUD. The shared world still updates once. Online
camera selection remains the separate Netplay cameras option.

The title reuses its existing independent-camera world admission and authored
map rendering. Two own-view draws start from the same CPU/RAM/device/VRAM
checkpoint; both must finish before a complete pair is published. A failed
pair falls back to the canonical frame. Native menus and shared story locks
use one canonical view; normal rendering resumes after the menu closes.
The game restores host contexts around each callback and around the entire
pair, including callback watchdog unwinds. The generic API rejects online
sessions even when generic temporal replay has been enabled.

## Qualification

Original-disc, hidden-window, muted Windows check in
build-pr23/local38/native-input:

- Both characters visible with their own HUDs in the1280x480 composition.
- P1 stationary at500/427 while controller-driven P2 reaches1194.75/333.15,
  with both healthy. The setup fixture positions the nearby pair; the distant
  separation itself uses jump/dash/right inputs.
- Native pause UI uses one4:3 view; unpause resumes two panes.
- Resizing to960x720 preserves8:3 composition with letterboxing.
-848 verified render transactions,0 mismatches, watchdogs, aborts, rejected
  device reads or VRAM leaks;424 complete pairs.
-36/36 title CTests pass. Windows and Linux sandbox/abort helpers pass,
  including failure of the second view, scope cleanup, recovery and online
  refusal. This is not a full Linux title or cross-platform netplay qualification.

The first pause check sent Start before the opening animation finished. The
corrected check waits for the native menu and resumes with controller input.
The first distant screenshots preceded the camera's catch-up after a700px
teleport; later checks wait for each player to be inside its own viewport.

## Remaining limits

This is a development prototype, not a new public release or a merged change.
A short two-controller human check and further shared-scene/survivor coverage
remain. Do not label the whole checklist item173 fully qualified yet.

A band of incorrect-looking tiles at the top of Jungle's distant view remains
visible with normal P2 movement. The same band appears in a same-location
native tile-ring Unified control with Custom Renderer disabled (both players
at1296/395). That control also used a position fixture, so it does not prove
retail authenticity or conclusively identify the cause. This observation is
recorded rather than presented as a fixed rendering bug. A speculative
CPU-authority policy change was removed after this control; the normal offline
GPU policy is retained. CPU mirror synchronization before a local capture is
retained for callbacks that read native texture/palette bytes.

Source review also identified native-wide per-layer view caches absent from
the existing GPU checkpoint. Their relevance to the4:3 band is unproven; this
prototype does not claim to have repaired that separate cache concern.

Private screenshots and reports are retained beside the native run, including
local-close.png, local-distant.png, local-pause.png and local-resized.png.
GitHub32/39/41 intake is recorded separately in COOP_GITHUB_REVIEW38.md.

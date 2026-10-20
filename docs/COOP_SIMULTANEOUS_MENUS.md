# Independent co-op pause menus

Requested October 20, after Playtest 42. Item 209, `beads-eio.9.207`.
Implemented on `feature/paired-pause-menus`; not yet in a published release.

The co-op option **Independent pause menus** defaults on. Either living,
present character can pause. Both character menus open; each controller
navigates its own native menu, weapon selection and E-tank contents. Either
living character can resume the team. The campaign, acquired upgrades and
save data still belong to P1.

Local co-op displays two menus side by side, including when gameplay uses a
Unified camera. Local 8:3 keeps its paired canvas. Each netplay peer displays
its own character menu in both Unified and Split. Menu drawing is a private
render transaction; it never advances gameplay or tank consumption.

While this option is on, E-tank contents are always private. The existing
E-tank checkbox is explicitly a preference for the older, one-menu-at-a-time
mode and cannot turn off this requirement. Switching independent menus off
restores that preference without rewriting it. W-tank energy remains shared.
Both the option and effective E-tank policy are recorded in diagnostics;
the option enters the native match configuration and source compatibility ID.

Tank use blocks unpause while the partner keeps navigating and confirming
equipment. Options remains usable, and healing continues behind P1's shared
Options/Quit frontend. Escape and Quit cannot leave the menu during healing.
Requests on the same frame are deterministic: tank confirmation takes priority
over resume; simultaneous tank requests start P1's operation first. A second
tank cannot start until the first finishes, including the shared W-tank.
Blocked exit and tank requests are consumed, requiring a fresh press after
completion; holding Start through healing never resumes gameplay afterward.

Dead or withdrawn players cannot use a tank, change equipment or resume.
Their native menu can still be shown. P1 retains Quit and escape authority;
its shared Options/Quit frontend blocks P2 interaction and team resume.
Start closes P1's native Options page first; another fresh press is required
to resume gameplay. Options/Quit pages never draw a partner world sprite.
Native fades, sound transitions and the resume world update run once.
Only living characters apply native equipment changes on exit. Stage resets
retire both menus and their guards. Both personal menu records, their original
weapon selections, inventories and input history participate in snapshots and
deterministic state digests. Older snapshot layouts are rejected.

Boundary qualification passes the two rosters, either opener,
shared/split models, equipment changes during healing, healing priority,
no held-input resume, private contents, shared-W request serialization,
snapshot restoration, guest restrictions and surviving-player resume. The
focused Windows CTests pass, including catalog defaults and emitted hooks.
The Linux production build and all 30 CTests pass. Current lifecycle,
diagnostics and catalog checks also pass on Windows.

Original-disc native checks use controller input for all menu actions;
fixtures only prepare the stage, damage and supplies. No fixture selects a
weapon, heals a character, moves a cursor or dismisses a menu.

| Mode | Campaign | Native qualification |
|---|---|---|
| Local 8:3 | X, with Zero as P2 | Independent management, healing locks, both resume directions, paused snapshot restoration, selected weapon firing, shared confirmations and dead-P1 management pass. |
| Local Unified | Zero, with X as P2 | The same checks pass with the characters reversed. |
| Split LAN | X, with Zero as P2 | Independent management, healing locks, shared confirmations, surviving P2 and matching authoritative core/mod CRCs pass on both peers. |
| Unified LAN | Zero, with X as P2 | The same checks pass with the characters reversed. |

A further Local Unified run prepares only 32 HP of tank energy per seat and
heals 48 HP across the team. That exceeds a shared pool and verifies the
required private contents with the separate-tank preference off. Its 2,756
render verification checks pass without mismatches or aborted draws.

The healing-input refinement passes a fresh Local 8:3 controller run: X
changes weapon while Zero is still healing, returns to Lightning Web, holds
Start past completion, then resumes with a new press. Its 3,158 render checks
pass. A separate native Unified run opens P1's Options during P2's E-tank
use; P2's health rises from 4 to 9 under that frontend and reaches 32 before
resuming. All 298 render checks pass, and its Options capture is clean.
The same live equipment/held-Start check passes Unified LAN with X as P2
and Zero as P1; both peers retain matching authoritative core/mod checksums.
The LAN harness holds each chord for six frames so it crosses the configured
input delay before observation. Its initial two-frame observation failed to
see the change; that attempt is retained and is not counted as a pass.

Composed screenshots were inspected for both character menus and shared
frontends, including a clean Options page after suppressing the stray P2
sprite. `PSX_RENDER_PASS_VERIFY=1` remains enabled: the native checks require
zero restoration mismatches, aborted passes, watchdogs or leaked VRAM writes.
Immutable menu textures and palettes use journalled renderer uploads inside
private draws; they do not stream GP0 payloads into authoritative CPU VRAM.
The broader GP0 upload-journal defect is tracked as `beads-eio.3.375`.

Evidence is retained under `build-pr23/paired-menu/` and
`/home/matthew/mmx4-paired-menus-20261020/`: `qa8` contains the local native
checks; `qa10` contains the final clean-Options check and both connected modes;
`qa11-energy-proof/runtime` records the limited-energy proof above.
`qa12-tank-navigation/local-split-0` records live equipment management during
healing; `qa13-options-tank/runtime` records healing under Options.
`qa14-tank-lan/netplay-unified-1` records the final connected recheck.
The LAN runs qualify identical Linux development artifacts, not cross-build,
cross-OS or public-lobby compatibility. Windows full-runtime linking is being
checked separately; the Windows model/catalog results are not a claim of
native Windows gameplay qualification.

The generated output was regenerated for four new native hook entries and
then reproduced without changes by the pinned Linux emitter. Private QA
artifacts and startup attempts live under `build-pr23/paired-menu/`; those
inspection executables are not release or compatibility artifacts.

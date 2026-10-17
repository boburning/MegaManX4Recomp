# Playtest32 release dispositions

The numbered [feedback checklist](COOP_PLAYTEST_FEEDBACK_20261010.md) retains
every report and its evidence. A checkmark means its stated validation passed.
An unreproduced report stays unchecked and tracked for new diagnostics; this
release does not silently relabel it as a fix.

## Validated changes since Playtest31

| Items | Result |
|---|---|
| 145 | Mushroom hatch waits through a nearby living partner's temporarily absent collision box. Native supplied replay fork and both-roster clearance checks pass. |
| 146 | Surviving P2 admits the Mushroom part2 lift. The original stalls atY2216; the corrected native control ascends toY2059 with P2 healthy and P1 still dead. |
| 147 | Cyber's new course transport completes an overtaken passenger's native outgoing beam instead of leaving hidden P1 in incoming state1. The previously failing P2-led Split LAN overlap now reaches checkpoint2 with both healthy and controllable; cached return data remains intact. |
| 148 | A grounded occupied Ride Armor supplies the partner's safe return floor. Native old-gate/fixed A/B and complete Dragoon admission preserve the Armor and restore the partner. |
| 149 | Native effect33 follows the X who created the hover jets. Actual X/P2 hover in Zero's campaign matches the initialized X/P1 blue/white jet control and expires on landing. |

Production assets must also pass Windows/Linux title checks, packaged launcher,
compatibility-offer, portable logging and fresh default-recording checks before
publication. Development controls are excluded from public builds.

## Bounded checks that did not reproduce the report

These dispositions state the limits of the check. They are permission to ship
the qualified changes under the owner's reproduce-or-release instruction,
not evidence that every reported configuration is unaffected.

| Items | Check and limit |
|---|---|
| 34 | Mushroom teleporter entry/return checks complete. The momentary old visual glitch lacks a matched frame/trigger. |
| 36 | Both staircase sections/rosters, multiple aspects and ring/tile comparisons pass. The reported floating-foot/pickup offset was not matched. |
| 41, 53 | Native intro-door/warning controls cover both campaigns and survivor seats. The reported old process crash or permanent warning lock was not reproduced. |
| 43 | Split presentation remains experimental. Prior X5-derived optimization and measured checks do not imply a universal frame-rate guarantee. No new isolated slowdown cause qualified. |
| 46 | Native lift ascent/exit checks pass. Owner confirmed the ceiling-panel crush is authentic; the distinct survivor admission146 is fixed separately. |
| 49 | Ordinary and charged staircase shots appear in both rosters. No usable original recording of the isolated missing shot was available. |
| 51 | Original frozen-Zero recording verifies14704 inputs; its final1129/1135 P2 inputs are neutral, and injected native movement works. The input-delivery cause remains unknown. |
| 107 | P2-led Spider spiked groups76 at1976/899 and3520/1384 spawn with matching peers. The reported omission was not matched. |
| 109 | Fresh Split/Both LAN natural approaches with either leader spawn actual bee offspring28 and parent29; composed P2 image shows flying wasps with P1 at the start. Core/mod checks agree through8002. This goes beyond the earlier unrelated ground-enemy probe; the supplied31 recurrence remains tracked. |
| 112 | Frost's timed ice phase and motion continue after native P1 death. The reported permanent platforms were not matched. |
| 119 | Native Sigma2 kinds51/58 choose the nearer P2 during initialization; existing actors retain their earlier facing. The reported exact enemy/respawn is unbound. |
| 121 | Both Slash background panes scroll8px/tick and change across all four alive/four survivor captures, with matching peers. The reported freeze's exact location remains unbound. |
| 126 | Native low-HP Owl grabs release after P2 approaches or interrupts with Rakuhouha. A permanent trapped-P1 condition was not matched. |
| 130 | X/P2 Twin Slasher damages native refight Spider48 to47. The separately authored supporting-web cut condition remains unmatched. |
| 133 | Earlier native hero-death/particle ownership checks pass. The newer report does not identify which hero/boss or event lost sprites; no matching additional defect qualified. |
| 135 | Native Lightning Web hits wall-mounted Mushroom48 to46 and changes phase7 to2. The intermittent missed weakness condition was not matched. |
| 136 | Native final-room admission and real team-death retry restore both controllable heroes. The specifically missing intro/dialogue is unbound. |
| 140 | Nearest-living-hero selection and P1 tie handling pass. The reported Sigma phase-specific positions/cached targets were not matched. |
| 144 | Native Slash live-HUD/scenery captures and earlier survivor scrolling controls remain healthy. Metadata8 has no bound composed corruption frame; no speculative palette rewrite made. |
| 150 | Native X/P2 live HUD covers weapons1/3/7 in Spider, Mushroom and Slash in Zero's campaign. Icons retain their colors across these stage palettes; the earlier pause-menu capture also looks correct. |
| 152 | P2 aerial class34 damages native Frost48 to46 in matching Split LAN peers. Both-seat expiry tests retain64 ticks and release on tick64. The separate unscaled aerial control missed and is excluded from effective-delay proof; the exact reported excess remains unmatched. |
| 153 | Actual P2 ground mash emits24/25/26 and damages Frost48 to38. A fresh native Zero/P1 scaling-off control emits the same classes, damages48 to38 and samples64 native ticks; independent finisher expiry is also exactly64. No longer finisher timer reproduced. |

## Explicitly outside this release gate

- **30/32, Discord/WAN startup:** owner requested parking those investigations.
  Startup/error logging is shipped and package logging checks remain required;
  private LAN checks are not represented as WAN qualification.
- **151, both heroes in additional story scenes:** an elective enhancement,
  separate from correctness fixes and existing simultaneous arrivals/victory poses
  (`beads-eio.9.173`).

For a recurrence, retain **Input Display and Recording**, quit normally, and send
the complete `saves/diagnostics` directory beside the executable/AppImage, release
number, stage/area, camera mode and video timestamp. Include both peers for netplay.

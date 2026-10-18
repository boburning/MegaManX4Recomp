# Playtest34 changes and qualification limits

Playtest34 includes commit9122a77. The numbered burndown remains157 items:
133 checked and24 unchecked. Older unmatched reports remain tracked.

| Items | Included behavior | Native qualification |
| --- | --- | --- |
| 155 | Charged fire effects follow their allocating player's projected shot. | The current-source reset reproduced with charged X/P2 Rising Fire plus Zero/P1 Ryuuenjin. After correction, both rosters defeat Frost and return healthy after23/18 issued charged overlaps. Flames and burn reactions were inspected. Sigma's arena setup was not matched; this qualifies the shared attachment fix and exact Frost failure. |
| 126 | Owl's own release/abort events also release the other captured body. | The actual fortress refight strands one body before the fix. Afterward both heroes are captured, released and controllable in both rosters. Pending native damage commands remain intact; Owl still updates once per world frame. |
| 156 | Refight hub arrivals validate the partner's landing before the beam. | Actual Slash defeat/return passes with both rosters and with P1 dying before P2's final blow. Both heroes return safely and regain control. Units cover all eight hub-return records; native gameplay coverage of all eight refights is not claimed. |
| 157, partial | Boss recovery flashing remains visible during private cooldowns. | Reviewed the supplied local clips, fresh single/ground/aerial/source-cadence fights, normal damage, a co-op-disabled control and before/after rendering. Real Split LAN accepts partner-overlap damage and retains matching core/mod CRCs through6668; both views show recovery flashing. |

The broader missed-damage condition in157 remains unconfirmed. The source input
excerpt was transplanted into fresh fights, not replayed as the original netplay
session. Earlier failed/missed native probe setups are excluded from passes.
[Full hit-registration evidence](COOP_HITREG_FEEDBACK_20261017.md) and
[fire/grab/return evidence](COOP_PLAYTEST34_INVESTIGATION.md) retain these limits.

Windows ZIP and Linux x86_64 AppImage use production settings with private debug
controls disabled. Both netplay peers must update. Unified cameras, Stacked
lifebars and Either-player doors remain the defaults. Split remains experimental;
boss scaling and the optional Double/Iris sequence remain off by default.

Release receipts: `build-pr23/release34-qa/`. Tracking: `beads-eio.9.178`.

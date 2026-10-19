# GitHub review for local split development

Reviewed against Playtest37 (master06e940c) on 2026-10-18. This is intake and
source/evidence review, not a claim that every historical failure is reproduced
or fixed. Private attachments remain in build-pr23/issue-review38; no reporter
message or issue closure has been posted.

| Issue | Evidence and disposition | Next useful check |
| --- | --- | --- |
| [41](https://github.com/mstan/MegaManX4Recomp/issues/41): intro gate explosions never finish | The attached startup log matches Playtest21's co-op profile38b6c001, not37. It reaches stage0/section0 and records damage near the gate, but supplies neither the input trace nor completed-frame scene state needed to establish the endless explosion loop. Current fix status remains unconfirmed. | Bounded current-build native gate attempt; if absent, request the full session directory named in the log and a fresh37 recording. Do not equate the different intro0/1 platform correction with this gate0/0 report. |
| [39](https://github.com/mstan/MegaManX4Recomp/issues/39): Cyber weapon and Space Port/Final Weapon backgrounds | Paired videos and both-peer diagnostics are usable. The ZIPs include cumulative older sessions: latest Space Port sessions match34 (7669a86f), whereas Final Weapon sessions match35 (c873d56d). The issue's general35 label does not identify every enclosed run. Current source has no demonstrated fix for these exact Space Port/fortress controllers. | Use the matching encounter sessions to compare X/P2 Aiming Laser's reticle/beam and per-view animated layers on37. This is distinct from the Cyber course analysis graphic. Keep open. |
| [32](https://github.com/mstan/MegaManX4Recomp/issues/32): missing X hover jets | The latest supplied sessions match31 (a2ed2a04). Checklist149 and3ac1021 document the actual native effect33 owner correction, released32. Current37 includes it; prior native X/P2 hover qualification matched the initialized X/P1 blue/white jets, followed the owner and expired on landing. | Ask for a newer recording only if this exact symptom remains; do not re-open149 solely from31 evidence. |
| 32: Slash Beast background and peer death effects | The video frames show both views and the peer's missing death burst. Existing train per-view code is not proof that this particular surviving-peer phase works. The frames alone cannot establish animation cadence. Neither remainder is qualified fixed by the hover correction. | Replay the paired31 encounter or a bounded equivalent on37; compare actual death-effect frames on each view and train layer phase while the other seat is stationary/dead. Keep open. |
| 32: diagnostics cannot be disabled | The diagnostics manifest explicitly says the netplay co-op playtest profile includes recording automatically; coop_inputs calls the recorder on netplay even when the standalone diagnostics package is disabled. This explains why unchecking the separate mod does not disable netplay recording. | Describe this as current playtest policy, and improve the setting text if it implies that this checkbox also disables mandatory netplay diagnostics. A display-only preference can be considered separately. |

Attachment review used eight evenly spaced decoded frames from each supplied
video plus archive metadata. These contact sheets establish visible states;
they do not substitute for a complete motion/cadence comparison or an actual
current-build reproduction. Each ZIP contains many historical sessions; always
select the session matching the reported stage, seat and build profile before
running a replay.

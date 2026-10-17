# Playtest33 changes and qualification limits

Playtest33 includes the fixes from PR34 and PR35. The numbered burndown remains
154 items:130 checked and24 unchecked. This release does not relabel older
unmatched reports as fixes.

| Items | Included behavior | Native qualification |
| --- | --- | --- |
| 152/153 | Boss scaling halves boss hit cooldowns with either method, rounding up. Separate mode retains independent timers and Zero's short opening combo timing. | Frost refight A/B shows earlier repeat aerial and post-finisher damage with Zero/P2; the reverse roster also passes. Three completed Split LAN phases qualify a held slash damaging once, repeated combos and consecutive aerial damage, with25 matching core/mod CRC samples through6980. |
| 154 | X's armor capsules use X/Dr Light dialogue in Zero's campaign, retaining P1's campaign and palette ownership. | The real Spider boots capsule changes from HUNTER H.Q. text to the correct Dr Light hologram/pages. The full conversation completes, X receives the boots, and both heroes resume control with HP32 and Zero's campaign unchanged. An X-campaign native control and lifecycle/story-selector tests supplement this check. |

The additional native simultaneous-hit probe did not produce accepted hits
inside the overlap window. It is not counted as passed. Both-owner unit checks
verify independent eligibility, exact32-tick expiry for a native64-tick timer,
odd-duration rounding, snapshots and native timing with scaling off. These
checks do not claim a new full-game or Windows-to-Linux gameplay qualification.

The release provides a Windows ZIP and Linux x86_64 AppImage. Both peers must
update. Unified cameras, Stacked lifebars and Either-player doors remain the
defaults. Split remains experimental; scaling and the optional Double/Iris
sequence remain off by default.

Private build, package and publication receipts are under
`build-pr23/release33-qa/`. Release tracking: `beads-eio.9.175`.

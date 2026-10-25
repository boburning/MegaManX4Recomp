# Stable 1.0.7 co-op correction bundle

This integration starts from stable `8032596` (1.0.6) and combines three
independently tracked corrections:

| Track | Original changes | Tracking |
| --- | --- | --- |
| Magma Dragoon first-slope rumble | `09a66af`, PR67 | `beads-eio.9.225` |
| Web Spider survivor ladder | `25f8004`, PR68 | `beads-eio.9.226` |
| X stored-charge HUD ownership | `7513962`, `74bc5e0` | `beads-eio.9.238` |

The production changes apply to separate code paths. The Magma and Web Spider
native regression harness additions overlapped at the scenario methods and CLI
choices; integration retains all three new entries: `magma_slope_rumble`,
`spider_ladder_pair` and `spider_ladder_survivor`.

The framework remains `a251a98616f0c8f9e45c90c7ca71d20da71b8b21` and recomp-ui
remains `b964eabe5a3438b8ed121064354ba27c1accef3f`. Four-player code, free-cutscene
control, preview presentation changes and preview performance changes are not
part of this stable bundle. Diagnostic recording defaults remain off; public
packages must continue to exclude CD Speed, Fast Loading and development cheats.

## Combined checks completed

- The split, combat and lifecycle ownership suites compile and pass together
  against the stable framework headers. Split and combat use Clang C99 with
  `-Wall -Wextra -Werror`; lifecycle additionally suppresses the existing
  typedef-redeclaration and partial-initializer warnings. No unrelated warning
  cleanup is included.
- The combined native scenario harness parses and retains both tracks' methods.
- The hook-registration suite passes, including the new `800B2C8C` stock-HUD
  updater in both development and release recompiler configurations.
- `git diff --check` passes. Framework/UI pins and the title mod catalog retain
  their stable values.

Private integration receipts are retained under
`G:/Temp/Codex/psxrecomp/x4-stable-bug-audit/model-checks/`.

## Release qualification still pending

These combined host-model checks do not establish native visual correctness.
The release owner must regenerate the stock-HUD entry hook and build the exact
stable bundle before final original-disc and extracted-package qualification.
Keep the independent native evidence in the three track documents separate
from the final combined results until those checks complete.

The highest-value interaction check is the survivor stage-pool camera
transaction: its stage-independent correction also supplies the real surviving
view to Magma's emitters. Recheck the recorded first-slope route and native
pillar control with the ladder fix present. Retain Web Spider descent/return
and Sigma 1 survivor controls, and inspect stored-shot HUDs with both character
orders in Split and Unified. Do not infer a full-campaign or new online gameplay
qualification from these bounded checks.

# Original-disc validation hotfix

The v1.0.3 Windows release accepts the verified original USA CUE/BIN with
Custom Renderer enabled, including fixed 16:9. The reported package-target
error could not be reproduced with that image. No original-disc target was
relaxed. The affected player's selected image has not been independently
identified.

Investigation found that disabling all game-specific mods could bypass their
full-disc hash checks. v1.0.4 makes original-disc verification a required title
policy, independently of mod selections. A mismatch or read failure rejects
selection/launch with an original-disc error. The untouched base is checked
again at session boot, before any mod-derived private image is used.

## Required image

Original USA SLUS-00561 raw BIN SHA-256:

```
3ceab06ac99add4035f912188bcbf5b16c02056f46a33d38ff0c8dbce6cb613b
```

CUE and direct BIN resolve to the same data. CHD verification hashes decoded
raw sectors. A matching serial or executable alone does not qualify a disc.
All changes are mods over the original image. Native memory cards are
unaffected.

## Validation

- Framework unit checks cover BIN/CUE/decoded CHD identity, a changed asset
  with unchanged serial, same-path modification and restoration, missing
  inputs, truncated CHD, and malformed/empty policy rejection.
- Existing native and forced-portable mod-runtime checks pass with the
  mandatory policy active, including an uninitialized mod catalog.
- The portable release config is checked against the development config's
  original-disc policy.
- Production packaging and exact-archive launch results will be recorded
  before publication.

The hotfix is based on v1.0.3. It does not qualify additional stage fixes,
fan-subtitled FMVs, full campaign coverage, or fresh online gameplay.
The Linux AppImage remains pending.

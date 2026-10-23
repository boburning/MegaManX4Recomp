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
raw sectors. The mounted track map must also match the original. Changed
track offsets or added audio tracks are rejected even over an unchanged BIN.
A matching serial or executable alone does not qualify a disc.
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
- All38 Windows production checks pass. Five framework checks also pass on
  Linux, including native, cached and forced-portable mod preparation,
  canonical identity and strict config parsing.
- The extracted Windows candidate accepts original USA CUE and direct BIN.
  Native hidden/muted launches reach runtime execution with Custom Renderer
  at Adaptive and16:9, without a package-target error.
- With every mod disabled, a changed asset with unchanged serial, added CUE
  audio track, changed INDEX offset and missing image fail identity queries
  and direct no-launcher boot. Selecting the original for a subsequent launch
  succeeds both with all mods off and with widescreen enabled again.
- Rendered recomp-ui checks toggle Custom Renderer off/on, select Adaptive
  and16:9, and launch through PLAY. Modified media produces verdict3 and a
  clear original-disc message; PLAY stays blocked even after enabling the
  renderer. Windows same-process Browse recovery was not automated because
  the native file picker is outside the hidden launcher protocol.
- Release staging verifies the catalog, static system-only dependencies,
  execution identity, notices/OpenBIOS and absence of private data. Neither
  CD Speed nor Fast Loading is included.

Game code is based on160818e(v1.0.3), with framework91fc45d and unchanged
recomp-ui b964eabe. Private evidence is retained in the
`x4-release-widescreen-audit`, `x4-widescreen-diagnosis`, and `x4-release104`
task directories. Final artifact hashes are published in SHA256SUMS.txt.

The hotfix is based on v1.0.3. It does not qualify additional stage fixes,
fan-subtitled FMVs, full campaign coverage, or fresh online gameplay.
The Linux AppImage remains pending.

# Fan subtitles for Japanese FMVs

The **Japanese Media → Japanese FMVs → FMV subtitles** choice offers original
Japanese video, English subtitles from acediez/Hondoori, or Spanish subtitles
from El Retro Dan. Import and select the matching movie data first. Both small
metadata packages are bundled; the selector activates the matching package
automatically. Missing resources reject launch. Leaving
Japanese FMVs off does not activate the selected subtitles.

Each pack also has its own **Languages → Japanese FMVs — … subtitles** feature.
Enable it for standalone use, without a Japanese donor disc or Japanese Media.
Both packs may be installed, but enabling both is rejected. A fan pack replaces
the ten opening/story movies once, including when original Japanese FMVs are
enabled. The original Capcom logo movie stays unchanged. Fresh boot required.

Only the original fan-subtitled movies and their Japanese audio are imported.
In-game script retranslations, variable-width fonts, translated menu art,
READY/WARNING changes, gameplay voice restoration and optional hex cheats from
the patches are outside this port. Existing Spanish/Portuguese dialogue drafts,
Japanese voices and title art remain independent. The runtime uses the stock disc.

## Import

Use Python 3.11+, [xdelta3](https://github.com/jmacd/xdelta), and 7-Zip for the
English archive. Supply the original USA raw MODE2/2352 BIN in [DISC.md](../DISC.md).
The importer checks the exact archive, patch and source disc before extraction.

```powershell
$env:TEMP = 'F:\Temp\Codex\psxrecomp\x4-subtitles'
$env:TMP = $env:TEMP
New-Item -ItemType Directory -Force -Path $env:TEMP | Out-Null
python tools/import_fmv_subtitles.py `
  --archive 'E:\Downloads\Mega Man X4 (USA) [Undub v1.13 + Retranslation].7z' `
  --disc 'mmx4\Mega Man X4.bin' `
  --output 'F:\Temp\Codex\psxrecomp\x4-subtitles\English-FMVs.psxmod' `
  --scratch 'F:\Temp\Codex\psxrecomp\x4-subtitles\scratch' `
  --xdelta 'F:\Tools\xdelta3.exe' `
  --seven-zip 'C:\Program Files\7-Zip\7z.exe'
```

For Spanish supply `MEGA MAN X4 RETRADUCIDO FINAL.zip` and a different output
name. Language is identified by SHA-256, not filename. At least 2 GiB free
scratch space is required. Windows scratch/output on C: is rejected. Existing
outputs are never overwritten. Tool subprocesses have bounded timeouts.

The importer writes two files:

- **`.psxmod`**: standalone metadata archive for builds without the bundled
  entry. Updated builds already have it; use that entry rather than installing
  a duplicate version. Contains metadata, author/source links,
  `CREDITS.md`, exact supplied readmes and `provenance.json`.
- **`.x4fmv`**: keep in a persistent location, then select it in the installed
  pack's **Attributed fan movie pack** picker. Size and SHA-256 are verified
  into the immutable session/netplay identity. Large media stays external,
  preserving the framework's 256 MiB mod-archive expansion limit.

XDelta creates only a private temporary extraction oracle, deleted after import.
The installed mod has no XDelta dependency, executable, EXE replacement, script,
or disc-image operation. Every 2336-byte movie sector is preserved verbatim,
including XA subheaders, MDEC video, audio, subtitles and credits. Native CD,
MDEC and XA playback retain timing, skipping and story continuation.

## Attribution and provenance

See [complete contributor roles and source links](fmv-subtitles/CREDITS.md),
[acediez's supplied Readme](fmv-subtitles/acediez-Undub-v1.13-Readme.txt), and
[El Retro Dan's supplied LEEME](fmv-subtitles/El-Retro-Dan-Final-LEEME.txt).
No new redistribution license is present in the readmes. Generated audiovisual
packs are local artifacts, not automatically bundled in public releases.

| Supplied archive | SHA-256 |
|---|---|
| Mega Man X4 (USA) [Undub v1.13 + Retranslation].7z | `24d2a8a7c142d724736c999ef74e98350821a17d62de76e6b94a27cc00e610b3` |
| MEGA MAN X4 RETRADUCIDO FINAL.zip | `e0ca3dc5686bc94309e523a6438ec1b2f6ae3b991f111c1bc98f5b4a4b8fd189` |

`provenance.json` records patch, extraction-oracle, movie-pack and per-movie
SHA-256 identities, native ISO paths, sector counts, MDEC frames and XA audio
inventory. Selector labels, package/feature authors, source links, activation
logs and credits identify the original fan creators throughout.

## Validation

`mmx4_localization_assets_test --fan <original-US-BIN> <pack.x4fmv>` checks real
movies against the original executable. Only the ten movie LBA/size/header
triplets change; every other EXE byte, script and font remains identical and no
ARC is replaced. It checks EXE-entry publication, duplicate campaign suppression,
and malformed pack rejection before mounting. Catalog checks cover conditional
requirements, missing resources and mutually exclusive subtitle packs.

Native playback, visible subtitles, XA audio progression, skipping and actual
scene continuation require game runs. Extraction and passing asset checks alone
do not establish those behaviors; retain per-movie results separately.

Qualification on 2026-10-22:

- Both supplied archives imported successfully. All twenty movie streams match
  every source sector byte for byte. Full FFmpeg MDEC/XA decode passes without
  errors for all ten English and all ten Spanish movies.
- Real pack/executable checks pass for both languages, including movie-only
  isolation, native CD capacity and no duplicate donor campaign mount. Catalog
  checks cover standalone installation, verified external resources, differing
  language fingerprints, mutual exclusion and both conditional selector choices.
- Muted Windows/OpenGL native runs pass for standalone English in X's campaign
  and Spanish via the Japanese Media selector in Zero's campaign. Visible fan
  subtitles were inspected. Opening skip works; each first story movie advances
  through MDEC/XA and completes naturally, then the original story continues to
  Intro gameplay through native controller input.
- The existing sixteen Japanese-media/Spanish-dialogue asset combinations and
  catalog-authoring checks pass. Default movie options remain off. Plugin audits
  include both metadata packages; CD Speed/Fast Loading remain excluded.
- Other individual story-movie triggers/end transitions, audible listening
  review, co-op/netplay movie transitions and Linux runtime playback remain
  unverified. Full-decode and table-isolation evidence is not full-game QA.

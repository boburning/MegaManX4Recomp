# Fan subtitles for Japanese FMVs

Enable **Languages → Japanese FMVs**, then choose **Subtitles: No subtitles / English / Español**.

This is one mod with one selected movie source. English and Español cannot be
selected together. Japanese voices, Japanese title art, in-game text languages,
Custom Renderer and local co-op are separate choices and can be combined with
it. Restart the game after changing movie sources. The title's current online
co-op profile activates its own simulation and renderer settings; offline
language/movie selections are not carried into that profile. Fan movies are
therefore currently an offline option, including local co-op.
The two standalone subtitle toggles have been removed.

- **No subtitles** uses your verified original Japanese Rockman X4 disc.
- **English** uses the imported acediez/Hondoori `.x4fmv` pack.
- **Español** uses the imported El Retro Dan `.x4fmv` pack.

Only the selected choice's picker is shown and its media required, verified
and loaded. Both fan packs may remain selected on disk; a dropdown permits
only one movie source per boot. English/Spanish do not require a Japanese
donor. Turning Japanese FMVs off keeps the original USA movies.
The fan packs replace the ten opening/story movies; the original Capcom logo
movie stays unchanged. Requires a fresh boot after changing the choice.

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
scratch space is required. Choose a scratch drive with enough free space. Existing
outputs are never overwritten. Tool subprocesses have bounded timeouts.

The importer writes two files:

- **`.psxmod`**: the combined Japanese FMVs metadata, for updated builds
  without the bundled entry. Install it once; both imports use the same mod ID
  and three-choice manifest. Updated builds already include this mod. It
  contains all author/source links, `CREDITS.md`, both original project readmes
  and both languages' preserved provenance.
- **`.x4fmv`**: keep each language's sidecar in a persistent location. In
  Japanese FMVs choose its subtitle language, then select the matching pack.
  Exact size and SHA-256 enter the immutable session identity. Unselected
  large media is not loaded. External media preserves the framework's
  256 MiB mod-archive expansion limit.

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
and malformed pack rejection before mounting. Catalog checks cover conditional resources, missing selected media and
mutually exclusive subtitle choices.

Native playback, visible subtitles, XA audio progression, skipping and actual
scene continuation require game runs. Extraction and passing asset checks alone
do not establish those behaviors; retain per-movie results separately.

Historical extraction/playback qualification on 2026-10-22 (before UI consolidation):

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

## Single-selector follow-up

The owner requested the consolidated control on 2026-10-23. Framework resource
conditions ensure only the chosen source is required and exposed in the UI.
Qualification of the consolidated control:

- All39 Windows title checks pass. Framework checks pass on Windows(3/3)
  and Linux(4/4), including cached preparation and portable hashing paths.
- Both real imported packs pass the native table/isolation test, all16
  Japanese-movie/voice/title/text asset combinations pass, and all three
  selector choices resolve exactly one verified media resource with distinct
  session identities. Missing or corrupt selected media rejects; inactive
  media is not required or loaded.
- Muted Windows/OpenGL native runs for English/X, Spanish/Zero and original
  Japanese/no subtitles all publish extended opening-movie tables, progress
  MDEC and XA, accept native skip/input and reach intro gameplay. English and
  Spanish are run without a Japanese donor configured.
- Rendered recomp-ui checks show one Japanese FMVs entry, the three dropdown
  choices, and exactly its matching credited picker. The two old subtitle
  entries are absent. Metadata exports from either real sidecar produce the
  same combined manifest, preserving both source readmes and provenance.

Private evidence: `x4-fmv-consolidation` and `x4-fmv-one-selector` task folders.
The subsequent [1.0.5 native matrix](FMV_105_MATRIX.md) qualifies all nine story
movies naturally and with Start skipping for all three selector choices, plus
three natural openings. It includes local co-op and native Linux playback.
Audible listening, full campaign traversal and fan-movie netplay are not claimed.

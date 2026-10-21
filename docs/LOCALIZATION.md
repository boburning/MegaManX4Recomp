# Languages and Japanese media

The optional Languages package has independent switches for Japanese FMVs,
Japanese voices, the Rockman X4 title screen, and text translation. Nothing is
enabled by default. All selections require a fresh boot.

The base game remains the USA SLUS-00561 revision. Select an owner-provided
Japanese SLPS-00902 CUE/BIN, CHD, or raw 2352-byte ISO in the Japanese Media
feature. A cooked 2048-byte ISO cannot preserve the XA video/audio streams.
The mod contains no Japanese disc assets. The disc's immutable 2336-byte Mode-2
snapshot is verified before any adapter receives it.

Spanish dialogue is the first text implementation. Its JSON inventory contains
104 campaign/message records: 102 readable messages have a Spanish draft, and
two non-text terminators retain their original data. The Ultimate Armor X
archive shares X's message IDs. Menus drawn as separate artwork and text burned
into FMVs are separate surfaces and are not translated by this dialogue mod.
Spanish voices are a future audio mod; this implementation supplies text and
glyphs for that effort. The Spanish draft still needs native-speaker review.

## Editing a translation

Copy `mods/preloaded/packages/mmx4.localization/1.0.0/catalog/es.json`, then edit
the plain `translation` values. Each entry retains a stable `id`, readable
`original`, and `source_sha256`. IDs identify the original campaign and message
number, so wording changes do not rename the translation key. The source digest
guards the exact native record independently of its address.

Newlines are `\n`; a textbox/page break is `\f`. The runtime wraps at word
boundaries in the original 20-column, three-row textbox and adds native page
controls when needed. Empty translations leave that message unchanged. A word
that cannot fit, an unsupported glyph, a duplicate ID, or stale source data is
an error rather than a clipped or garbled translation.

Package the edited JSON with:

```sh
python tools/build_translation_mod.py --catalog my-spanish.json --output my-spanish.psxmod
```

Install that archive through the normal Mods installer. Its installed
`mmx4.localization` package shadows the bundled version while retaining the
independent Japanese media controls. Repackage after editing: the catalog's
verified size and SHA-256 are part of the committed content identity. An edited
file cannot silently change a running session or claim the old identity.

The `language` field can name a community language without recompiling. The
first adapter supports the native Latin font and Spanish accented letters,
including `ñ`, `ü`, `¿`, `¡` and their uppercase forms. Other glyph repertoires
need explicit native-font composition rules; missing characters never become
spaces or an unrelated host font.

## Native implementation

Shared psxrecomp owns JSON parsing, Unicode validation, glyph mapping, word
wrapping, indexed-font composition, verified Mode-2 media, and derived file
mounting. This extends the shared Tsumu translation work. The font composition
follows the Gundam Wing Endless Duel approach: copy the original indexed glyph
and add marks using its own palette. The game renders the result itself.

X4 owns its ARC member types, message record format, source identities and LBA
tables. Language preparation rebuilds ARC members in memory; original guest
loaders, textbox state, GPU uploads, XA decoder and SPU drivers consume them.
Guest directory reads and host asset readers resolve the same derived files,
including co-op's private character banks.

`LOAD_U.ARC` member `0x10001` contains the dialogue atlas. Spanish cells occupy
blank `0xE0..0xEF`; the original letters, indices and palette remain intact.
The source messages are checked for references to those cells. Japanese title
art replaces the loading atlas member `0x10008` and the main title scene's
`ST0E_U0.ARC` artwork/layout plus `COL0E_U0.ARC` palette. Title-scene audio stays
US unless the independent voices option is selected. The US dialogue font is
preserved. Gameplay
voice substitutions replace complete VAB metadata/sample pairs, not just sample
bytes. Streamed voices also receive the Japanese per-channel cue tables.

The owner-disc qualification program accepts US disc, Japanese disc and catalog
paths and exercises all 16 selections, including translation plus each Japanese
media combination. That asset qualification is distinct from native playback,
visual, audio and co-op validation; record those results separately.

Native smoke qualification is available as `tools/qualify_mmx4_languages.py`.
It requires `--exe`, `--game`, `--disc` and a new `--output` directory. Add
`--jp-disc` and `--japanese fmvs voices title-screen` for all Japanese media;
`--catalog` exercises an edited community package, and `--campaign 1 --coop`
checks Zero with a native X partner. It uses muted hidden windows and isolated
profiles. Inspect the saved images; a completed script alone does not establish
visual correctness or every scene's cue timing.

Implementation qualification on 2026-10-20:

- Shared catalog/Unicode/font, media, runtime replacement, package and existing
  text-hook tests pass. X4 archive, catalog authoring and preloaded catalog tests
  pass. All 16 real owner-disc asset selections pass.
- Native US/OpenBIOS boot with Japanese media and Spanish reaches gameplay.
  Rockman title art and Spanish mission dialogue were visually checked.
  The original SPU contained 63 Japanese-specific 512-byte player sample blocks.
- An edited community catalog displays `¡Sí! ¿Dónde está el señor? ¡Acción, X!`
  using native glyphs. Spanish without a Japanese donor reaches Zero's campaign
  with both co-op players active. The native accents and both bodies were
  visually checked.
- That edited-catalog run exposed a two-byte-aligned message-bank end. Rebuilt
  banks now preserve the guest loader's four-byte alignment; the failing case
  passes after the fix and has a focused regression check.
- A final combined run enables all three Japanese media switches, Spanish and
  X's co-op campaign. Native MDEC decoded the donor opening, both actors were
  active, Spanish dialogue and the Rockman title were visually checked, and 63
  Japanese-specific sample blocks were again found in live SPU memory.
- Full story playback/cue timing, every boss and weapon voice, cold save-state
  restore, online netplay, other platforms and native-speaker Spanish review
  remain unverified. This is an experimental first dialogue implementation.

Reference format checks used the original discs and the text/sprite formats in
[TeheManX4 Editor](https://github.com/Kuumba123/TeheManX4_Editor). No editor asset
files are included in this package.

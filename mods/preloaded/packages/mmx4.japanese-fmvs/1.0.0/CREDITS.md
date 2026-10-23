# Fan-subtitled Japanese movies for Mega Man X4

This is a movie-only extraction of existing fan work. The native adapter and
importer are by the MegaManX4Recomp contributors. They do not claim authorship
of the translations, subtitles, footage, music or original patches.

## Original projects and contributions

- **acediez** — Mega Man X4 Undub v1.13 (2021-08-09), project author and
  PS1-specific subtitled movie preparation. The supplied readme says these
  320×240 movies use **Hondoori's translation**, rather than the PC Final Weapon
  video encodes. [Original project thread](https://www.romhacking.net/forum/index.php?topic=26255.0).
- **Hondoori** — translation on which the Undub movie subtitles are based.
  [Rockman X4 script translation](https://hondoori.wordpress.com/2016/01/24/rockman-x4-script-translation-zero-side/).
- **NectarHime** — Final Weapon PC relocalization, whose script was used by the
  supplied Undub + Retranslation patch. That in-game script is **not imported by
  these movie packs**. Credit is retained as part of the source project's lineage.
  [Final Weapon announcement](https://www.reddit.com/r/Megaman/comments/b8a4k9/final_weapon_a_retranslation_undub_and_musicsound/).
- **DarkSamus993** — ARC and table-of-contents documentation, XA audio table
  documentation, and Maverick voice restoration in the Undub project. Those
  restored gameplay voice banks are **not included in this movie-only port**.
- **LameGuy64** — [mkpsxiso](https://github.com/Lameguy64/mkpsxiso), credited by
  the original Undub project. The native runtime does not rebuild a disc image.
- **El Retro Dan** — MEGA MAN X4 RETRADUCIDO FINAL: Spanish retranslation,
  subtitled cinematics, larger final-version subtitles, Japanese opening lyrics
  with Spanish subtitles, and Spanish artwork. Only the movies are extracted;
  menus, dialogue, font changes and artwork remain outside this port.
  The supplied LEEME is unsigned; author identification comes from the author's
  [2026-07-13 final-release announcement](https://www.youtube.com/watch?v=dM_LnAhPoPI)
  and [2026-07-10 final-version stream](https://www.youtube.com/watch?v=BJUfm6cpXTA).
  The LEEME credits acediez, DarkSamus993, NectarHime and Hondoori as its foundation.
- **Kuumba123** — [TeheManX4 Editor](https://github.com/Kuumba123/TeheManX4_Editor),
  used for most changes in El Retro Dan's project and acknowledged in the LEEME.
- **Blade133bo** — assistance translating menus and READY/WARNING artwork in
  the Spanish final version, as explicitly credited in its English LEEME section.
  Those non-movie changes are not imported here.

**Capcom and the original production staff, animation staff, Japanese voice
performers, songwriters, musicians and other rightsholders** retain credit for
the underlying game, animation, voices and songs. Existing on-screen credits,
including the Spanish project's Japanese cast/song credits, stay inside the
original stream bytes. This extraction neither transcodes nor removes them.

The full supplied English Readme and Spanish LEEME are preserved verbatim in
`docs/fmv-subtitles/` in the source repository. Each generated mod includes its
own supplied readme; the Spanish pack also includes the inherited Undub readme.
The mod's `provenance.json` records archive, patch, movie-pack and individual
movie SHA-256 identities, extraction scope and native sector/frame inventories.

## Rights and distribution

No new license grant for either fan patch is present in the supplied readmes.
Repository licensing does not relicense this third-party work. This tool imports
the owner's supplied patch and verified original USA disc locally. Generated
movie packs contain Capcom audiovisual material and fan subtitles; they are
local artifacts, not source assets committed to this repository or automatically
bundled in public releases. Attribution does not imply permission to redistribute.

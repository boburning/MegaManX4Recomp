# 1.0.5 native movie transition qualification

The consolidated Japanese FMVs selector passes all **54 story-movie checks**:
nine story movies, natural completion and native Start skipping, for each of
its three sources. The three neutral cold-boot openings also complete naturally.

| Selector choice | Platform | Story movies 2–10, natural | Story movies 2–10, Start skip | Opening, natural |
|---|---|---|---|---|
| English — acediez / Hondoori | Windows / OpenGL | 9/9 | 9/9 | Pass |
| Español — El Retro Dan | Linux / OpenGL, Xvfb | 9/9 | 9/9 | Pass |
| No subtitles — original Japanese donor | Windows / OpenGL | 9/9 | 9/9 | Pass |

All runs use the verified original USA base disc, local Split co-op, Both doors,
both story encounters, boss scaling and separate E-tanks. Each language is
tested in both campaigns. No fan-patched image is used as a runtime base disc.
English and Spanish run without a Japanese donor configured. The no-subtitle
choice uses the verified original Japanese disc as external movie media.

Each story check asserts one expected original movie call, more than 30 MDEC
decodes, XA activity and its original return. Natural completion requires the
authored terminal frame; skip checks send Start only after video/audio advance
and require an earlier return. Ordinary story continuations reach actual
gameplay with both heroes active, visible and healthy. Final movies reach the
native ending mode. Colonel's interlude passes through the original save prompt,
chooses No with native input, then returns through the stage selector.

Campaign prerequisites and travel are explicitly bypassed with offline developer
fixtures. The original dispatcher initializes the frontend, dialogue, movies,
save prompt, ending and subsequent scene. Fixtures do not write completion or
victory flags. A current-build snapshot resets prerequisites between cases.
Most matrix runs enable fast-forward during observation; English/X natural
movies 2–5 also pass at ordinary pacing. Fast-forward does not press Skip or
supply a completion result. Captures show the supplied English/Spanish subtitles.
Earlier full-stream FFmpeg checks additionally decode all twenty fan streams
without errors and establish byte-for-byte identity with their source patches.

The initially guessed X movie 4 prerequisite and a rejected snapshot slot are
excluded. Movie 7's original save prompt was initially misclassified by the
test; corrected native navigation passes. An early direct-movie admission
overlapped a confirm edge with the first movie frame and returned before XA;
the check rejected it. Direct admissions now remain neutral, and fresh Japanese
8/9/10 skip checks pass. These fixture/observer failures are not game fixes.

The offline observer and gateway fixtures are absent from production binaries.
Windows production checks pass 39/39 and Linux production checks pass 34/34.
This is a transition matrix, not a full campaign traversal, full credits audit,
audible listening review, or a claim that every language was tested on both OSes.
The current trusted online profile does not activate offline movie/language mods.

Private evidence: `G:/Temp/Codex/psxrecomp/x4-release105/`, including
`fmv-qualified-summary.json`, `movies-en-*`, `linux-spanish-*`, `movies-jp-*` and
their per-movie timelines/captures. The last three Japanese skip rows are in
`movies-jp-zero-skip-final`. Source attribution and original readmes remain in
[the full credits](fmv-subtitles/CREDITS.md); [setup and coexistence](FMV_SUBTITLES.md).

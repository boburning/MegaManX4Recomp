Mega Man X4 Recompiled AppImage
===============================

The AppImage keeps its immutable program payload inside the mounted image and
stores settings, saves, mods, and generated cache data in:

    ~/.local/share/MegaManX4Recomp

Set MMX4_RECOMP_DATA_DIR to use another directory. The release scripts accept
both Linux paths and Windows paths such as F:\Releases when run under WSL.

Playtest diagnostic recordings are the exception: they go into
`saves/diagnostics/session-...` beside the `.AppImage` file, independently of
the configured memory-card directory. `diagnostics-location.txt` beside the
AppImage identifies the latest recording or a write failure. Quit normally
and zip the entire session folder when reporting a bug. It contains starting
game saves, input events, mod choices and session metadata. Netplay recordings
can be inspected; offline replay is available only for local recordings.

The AppImage contains no retail game disc, retail BIOS, save data, or other
copyrighted game assets. See START_HERE.txt after the first launch.

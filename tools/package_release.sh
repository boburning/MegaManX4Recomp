#!/usr/bin/env bash
# Stage an already-built production runtime through the shared release gates.
set -euo pipefail
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
fw=${PSXRECOMP_ROOT:-"$root/psxrecomp-v4"}
# The runtime build copies the developer config. Public staging must use the
# title's portable release config and controls, including for external builds.
build_dir=""
args=("$@")
for ((i=0; i<${#args[@]}; ++i)); do
    if [[ ${args[i]} == --build-dir ]]; then build_dir=${args[i+1]:-}; break; fi
done
[[ -n "$build_dir" ]] || { echo "--build-dir is required" >&2; exit 2; }
[[ -d "$build_dir" ]] || build_dir="$root/$build_dir"
[[ -d "$build_dir" ]] || { echo "build directory not found: $build_dir" >&2; exit 2; }
for file in game.toml input.ini START_HERE.txt; do
    cp "$root/packaging/release/$file" "$build_dir/$file"
done
# Preserve README image/guide paths inside the package. Framework staging
# already includes its own license, runtime notices and launcher notices.
docs=()
for path in "$root"/docs/*.md "$root"/docs/images/* \
  "$root"/docs/fmv-subtitles/*.md "$root"/docs/fmv-subtitles/*.txt; do
    [[ -f "$path" ]] && docs+=(--runtime-file "${path#"$root"/}")
done
exec bash "$fw/tools/package_game_release.sh" --root "$root" \
    --zip-prefix MegaManX4Recomp --exe-name MegaManX4Recomp \
    --display-name "Mega Man X4 Recompiled" --runtime-target psx-runtime \
    --runtime-file input.ini --runtime-file START_HERE.txt \
    --doc README.md --doc LICENSE --doc RELEASE_NOTES.md --doc THIRD_PARTY_NOTICES.md --doc DISC.md \
    "${docs[@]}" \
    --ship-without-overlay-cache-because "Original-disc inventory found no separate X4 executable overlay; the complete resident boot program is statically compiled. Eligible uncovered dirty-RAM code can use the bundled native toolchain. No private capture or unqualified cache is shipped." \
    "$@"

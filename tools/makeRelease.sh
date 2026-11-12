#!/bin/bash
# Builds the players' version for Linux and packs it for GitHub Releases
# (the Linux side of makeRelease.ps1).
#
#   tools/makeRelease.sh 1.0
#
# - Its own build folder (TatouSource/build/release-linux, or $BUILD_DIR).
# - CLASSIC_REBORN_DEV_TOOLS=OFF: no item window (F9) nor object editor (F10);
#   the visual fixes of 3D objects are still included.
# - CLASSIC_REBORN_VERSION=<version>: the launcher shows it and compares it
#   with the latest release to offer updates.
# - Release configuration: optimised, no debug data. The C++ runtime goes
#   inside the executables (as on Windows): nothing else to install.
# - The archive holds only Launcher, Tatou, README.md, LICENSE and the menu
#   font's license (Gelasio-OFL.txt; the font is inside the programs). It is a
#   .tar.gz, not a .zip, so both programs keep their "executable" mark.
# The result goes to release/ClassicReborn-<version>-linux-x64.tar.gz.

# CLASSIC_REBORN_EDITION=timegate builds Time Gate: Classic Reborn instead
# (its own build folder, TimeGateClassicReborn-<version>-...), with its music's
# General MIDI soundfont and its licence (TimGM6mb.sf2, TimGM6mb-copyright.txt)
# and the licence of its menus' font (Inter-OFL.txt).

set -euo pipefail

if [ $# -ne 1 ]; then
    echo "Usage: $0 <version>" >&2
    exit 1
fi
version="$1"
repo="$(cd "$(dirname "$0")/.." && pwd)"
source="$repo/TatouSource"
edition="${CLASSIC_REBORN_EDITION:-alone}"
if [ "$edition" = timegate ]; then
    build="${BUILD_DIR:-$source/build/release-linux-timegate}"
    product=TimeGateClassicReborn
else
    build="${BUILD_DIR:-$source/build/release-linux}"
    product=ClassicReborn
fi
output="$build/Fitd/Release"
releaseFolder="$repo/release"
name="$product-$version-linux-x64"

echo "== Configuring ($build)"
cmake -G Ninja -S "$source" -B "$build" -DCMAKE_BUILD_TYPE=Release -DCLASSIC_REBORN_DEV_TOOLS=OFF -DCLASSIC_REBORN_VERSION="$version" -DCLASSIC_REBORN_EDITION="$edition" \
    -DCMAKE_EXE_LINKER_FLAGS="-static-libstdc++ -static-libgcc" > /dev/null

echo "== Building Release"
cmake --build "$build" --target Launcher Fitd

# The development tools must not be in the executable
echo "== Checking that F9 and F10 are not included"
for marker in "(prueba, F9)" "(prueba, F10)"; do
    if grep -qF "$marker" "$output/Tatou"; then
        echo "Tatou still contains the development tools ($marker)" >&2
        exit 1
    fi
done

echo "== Packing"
# Prepared in a temporary folder of the system: on a Windows disk (WSL) the
# file permissions cannot be set
temporary="$(mktemp -d)"
staging="$temporary/$name"
mkdir -p "$staging" "$releaseFolder"
cp "$output/Launcher" "$output/Tatou" "$staging/"
cp "$repo/README.md" "$repo/LICENSE" "$staging/"
cp "$source/Shared/assets/fonts/OFL.txt" "$staging/Gelasio-OFL.txt"
chmod 755 "$staging/Launcher" "$staging/Tatou"
chmod 644 "$staging/README.md" "$staging/LICENSE" "$staging/Gelasio-OFL.txt"
if [ "$edition" = timegate ]; then
    cp "$output/TimGM6mb.sf2" "$output/TimGM6mb-copyright.txt" "$source/Shared/assets/fonts/Inter-OFL.txt" "$staging/"
    chmod 644 "$staging/TimGM6mb.sf2" "$staging/TimGM6mb-copyright.txt" "$staging/Inter-OFL.txt"
fi

archive="$releaseFolder/$name.tar.gz"
rm -f "$archive"
tar -czf "$archive" --owner=0 --group=0 -C "$temporary" "$name"
rm -rf "$temporary"

echo "== Done: $archive ($(du -h "$archive" | cut -f1))"

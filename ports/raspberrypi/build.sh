#!/bin/sh
# Name:        build.sh
# Description: the Raspberry Pi's package (Raspberry Pi 3 and later, Raspberry Pi OS 64-bit): SegaSonic and
#              SegaSonic Arcade built on Debian 12 for arm64 and packed as segasonic-raspberrypi-arm64.tar.gz
# Version:     0.1.0
# Created:     27-11-2026
# Modified:    27-11-2026
# Author:      Sebastian Toyos
#
# Usage: sh ports/raspberrypi/build.sh [--tools]
#
# A Raspberry Pi is a computer: its games are the Linux ones (the window, the first run that takes the assets out of
# the player's ROM set), for 64-bit ARM. Every Raspberry Pi from the 3 on runs the 64-bit Raspberry Pi OS (3B, 3B+,
# 3A+, Zero 2 W, 4, 400, 5, 500 and their Compute Modules: raspberrypi.com/software/operating-systems), and the
# game's aarch64 build is the one the Switch, Android and the Anbernic already have; a 32-bit package is not made.
#
# Built on Debian 12 "Bookworm" for arm64 (Raspberry Pi OS Legacy's base: glibc 2.36), so that the games run on it and
# on what follows it (Raspberry Pi OS on Debian 13 "Trixie", glibc 2.41), with SDL 3 built from its source into them
# (SEGASONIC_STATIC_SDL: Bookworm has no SDL 3, Trixie an older one) and the headers of what SDL talks to on a Raspberry
# Pi: Wayland and X11 (the desktop), KMS/DRM (Raspberry Pi OS Lite: no desktop, the whole screen), ALSA, PulseAudio
# and PipeWire. SDL loads those libraries itself when a game starts: the games need none of them to start.
#
# Where: the CI's raspberrypi job runs it in Debian 12's own image on an arm64 runner (--tools installs what the
# build needs: run as root). On a computer of another kind, the same system under QEMU: wsl.sh, beside this file,
# makes it in WSL and runs this script in it. SEGASONIC_BUILD_DIR, SEGASONIC_DIST_DIR and SEGASONIC_JOBS say where the
# build and the package go and how many compilers run at a time (by default build/raspberrypi, dist and every
# processor).
set -e

if [ "$1" = "--tools" ]; then
	export DEBIAN_FRONTEND=noninteractive
	apt-get update
	apt-get install -y --no-install-recommends ca-certificates build-essential cmake ninja-build python3 pkg-config \
		libasound2-dev libpulse-dev libpipewire-0.3-dev libx11-dev libxext-dev libxrandr-dev libxcursor-dev \
		libxfixes-dev libxi-dev libxss-dev libxtst-dev libxkbcommon-dev libwayland-dev wayland-protocols \
		libdecor-0-dev libdrm-dev libgbm-dev libegl1-mesa-dev libgles2-mesa-dev libgl1-mesa-dev libudev-dev \
		libdbus-1-dev
	shift
fi

SOURCE=$(cd "$(dirname "$0")/../.." && pwd)
BUILD=${SEGASONIC_BUILD_DIR:-$SOURCE/build/raspberrypi}
DIST=${SEGASONIC_DIST_DIR:-$SOURCE/dist}
JOBS=${SEGASONIC_JOBS:-$(nproc)}
NAME=segasonic-raspberrypi-arm64

case "$(uname -m)" in
	aarch64|arm64) ;;
	*) echo "This builds on 64-bit ARM (Debian 12 for arm64), not on $(uname -m): see ports/raspberrypi/wsl.sh" >&2; exit 1 ;;
esac

cmake -S "$SOURCE" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release -DSEGASONIC_REFERENCE_TOOLS=OFF -DSEGASONIC_STATIC_SDL=ON \
	-DSONIC_BIN_DIR="$BUILD/bin"
cmake --build "$BUILD" --parallel "$JOBS" --target SegaSonic sonic sonic_game_headless segasonic_console_headless

# What a player runs: the folder segasonic with the two games, their pictures beside them, the README and the licenses
# (the computers' packages' own: .github/workflows/build.yml's Pack), in a tar, which keeps the games' permissions
PACKED="$DIST/segasonic"
rm -rf "$PACKED" "$DIST/$NAME.tar.gz"
mkdir -p "$PACKED/licenses"
cp "$BUILD/bin/SegaSonic Arcade" "$BUILD/bin/SegaSonic" "$BUILD/bin/font.bmp" "$BUILD/bin/input_prompts.bmp" \
	"$BUILD/bin/segasonic.png" "$PACKED/"
cp "$SOURCE/LICENSE" "$PACKED/LICENSE.txt"
cp "$SOURCE/recomplauncher.json" "$PACKED/"    # (RecompLauncher's setup, settings and start, beside the games)
cp "$SOURCE/docs/package/README.txt" "$PACKED/README.txt"
cp "$SOURCE/tools/reference/mame/LICENSE" "$PACKED/licenses/MAME.txt"
cp "$SOURCE/tools/reference/ymfm/LICENSE" "$PACKED/licenses/ymfm.txt"
cp "$SOURCE/lan/LICENSE" "$PACKED/licenses/arcade-lan.txt"
cp "$BUILD/_deps/sdl3-src/LICENSE.txt" "$PACKED/licenses/SDL3.txt"
cp "$SOURCE/ports/sdl/controllerdb/LICENSE" "$PACKED/licenses/SDL_GameControllerDB.txt"
cp "$SOURCE/ports/sdl/font/License.txt" "$PACKED/licenses/Kenney-Fonts.txt"
cp "$SOURCE/ports/sdl/prompts/License.txt" "$PACKED/licenses/Kenney-Input-Prompts.txt"
chmod 644 "$PACKED"/*.txt "$PACKED"/*.json "$PACKED"/*.bmp "$PACKED"/*.png "$PACKED"/licenses/*
chmod 755 "$PACKED/SegaSonic Arcade" "$PACKED/SegaSonic"
(cd "$DIST" && tar czf "$NAME.tar.gz" segasonic)
ls -la "$PACKED" "$DIST/$NAME.tar.gz"

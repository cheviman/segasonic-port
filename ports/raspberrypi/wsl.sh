#!/bin/sh
# Name:        wsl.sh
# Description: the Raspberry Pi's package made on a PC: Debian 12 for arm64 in a folder of WSL's (or of any Linux
#              on a PC), run under QEMU, and build.sh run in it
# Version:     0.1.0
# Created:     27-11-2026
# Modified:    27-11-2026
# Author:      Sebastian Toyos
#
# Usage (as root, from the repository's folder as WSL has it; from Windows: wsl -u root -- sh ports/raspberrypi/wsl.sh):
#   sh ports/raspberrypi/wsl.sh                  the package: build/raspberrypi-dist/segasonic-raspberrypi-arm64.tar.gz
#   sh ports/raspberrypi/wsl.sh COMMAND ...      a command in that system, at the build's folder (/build; the
#                                                repository is /src, read only): the checks on the arm64 games
#
# The Raspberry Pi's games are built by Debian 12's own compiler and libraries for arm64 (build.sh says why). On a PC
# that system runs under QEMU's user emulation, which the kernel starts for every arm64 program (binfmt_misc: Ubuntu's
# package qemu-user-binfmt, which this needs installed): slower than a Raspberry Pi 5, and the same programs.
#   SEGASONIC_RASPBERRYPI_ROOT (by default /opt/segasonic-raspberrypi-bookworm-arm64) is that system's folder: Debian's own
#   root file system for arm64 (the one its Docker image is made of), unpacked once; build.sh --tools installs the
#   compiler and the libraries' headers in it the first time (a mark in the folder says it was done). QEMU's emulator
#   goes into it under the name the kernel looks for, a static one (Ubuntu's qemu-user-static, taken out of its package
#   without installing it). The build is in its /build, on Linux's own disk (the repository's, Windows', is slow).
# Nothing of this goes into the package: it holds the two games, as build.sh packs them.
set -e

ROOT=${SEGASONIC_RASPBERRYPI_ROOT:-/opt/segasonic-raspberrypi-bookworm-arm64}
SOURCE=$(cd "$(dirname "$0")/../.." && pwd)
ROOTFS_URL=https://raw.githubusercontent.com/debuerreotype/docker-debian-artifacts/dist-arm64v8/bookworm/oci/blobs/rootfs.tar.gz
INTERPRETER=$(sed -n 's/^interpreter //p' /proc/sys/fs/binfmt_misc/qemu-aarch64 2>/dev/null || true)

if [ "$(id -u)" != 0 ]; then
	echo "As root (chroot and mount need it): wsl -u root -- sh ports/raspberrypi/wsl.sh" >&2
	exit 1
fi
if [ -z "$INTERPRETER" ]; then
	echo "No QEMU for arm64 programs here (binfmt_misc's qemu-aarch64): apt install qemu-user-binfmt" >&2
	exit 1
fi

if [ ! -f "$ROOT/etc/debian_version" ]; then
	echo "Debian 12 for arm64 into $ROOT"
	mkdir -p "$ROOT"
	curl -fsSL "$ROOTFS_URL" | tar -xz -C "$ROOT"
fi
if [ ! -x "$ROOT$INTERPRETER" ]; then
	# (the emulator the kernel starts, by the path it has for it, inside the system: a static one)
	WORK=$(mktemp -d)
	(cd "$WORK" && apt-get download qemu-user-static >/dev/null && dpkg-deb -x qemu-user-static_*.deb unpacked)
	mkdir -p "$ROOT$(dirname "$INTERPRETER")"
	cp "$WORK/unpacked/usr/bin/qemu-aarch64-static" "$ROOT$INTERPRETER"
	rm -rf "$WORK"
fi

mkdir -p "$ROOT/src" "$ROOT/build"
mountpoint -q "$ROOT/src" || mount --bind -o ro "$SOURCE" "$ROOT/src"
mountpoint -q "$ROOT/proc" || mount -t proc proc "$ROOT/proc"
mountpoint -q "$ROOT/dev" || mount --bind /dev "$ROOT/dev"    # (apt checks its lists with gpgv, which needs /dev/null)
cp /etc/resolv.conf "$ROOT/etc/resolv.conf"
leave() {
	umount "$ROOT/dev" 2>/dev/null || true
	umount "$ROOT/proc" 2>/dev/null || true
	umount "$ROOT/src" 2>/dev/null || true
}
trap leave EXIT

run() {
	chroot "$ROOT" /usr/bin/env -i PATH=/usr/sbin:/usr/bin:/sbin:/bin HOME=/root LANG=C.UTF-8 \
		SEGASONIC_BUILD_DIR=/build SEGASONIC_DIST_DIR=/build/dist SEGASONIC_JOBS="${SEGASONIC_JOBS:-$(nproc)}" "$@"
}

if [ $# -gt 0 ]; then
	run /bin/sh -c 'cd /build && exec "$@"' sh "$@"
	exit $?
fi
if [ ! -f "$ROOT/build/.tools" ]; then
	run /bin/sh /src/ports/raspberrypi/build.sh --tools
	touch "$ROOT/build/.tools"
else
	run /bin/sh /src/ports/raspberrypi/build.sh
fi
mkdir -p "$SOURCE/build/raspberrypi-dist"
cp "$ROOT/build/dist/segasonic-raspberrypi-arm64.tar.gz" "$SOURCE/build/raspberrypi-dist/"
ls -la "$SOURCE/build/raspberrypi-dist/segasonic-raspberrypi-arm64.tar.gz"

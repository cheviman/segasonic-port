# SegaSonic on Anbernic's H700 handhelds

The console version of the game (`SegaSonic`) on Anbernic's H700 handhelds with their stock firmware, as a
port in `Roms/PORTS`: checked on the RG40XX H (Ubuntu 22.04, aarch64, glibc 2.35, Anbernic's SDL 2.0.12
drawing through its Mali GPU). It is the same game as on a computer, with what a handheld asks for:

- **The game's own 4:3 picture**, twice its size on the 640 x 480 screen (640 x 448). OPTIONS has no VIDEO
  page.
- **The handheld's buttons**, set by the game: A jumps and picks, B goes back in the menus (it does nothing
  in a stage), START starts and pauses, the d-pad and the left stick move. OPTIONS' CONTROLS gives each
  action another button.
- **MENU leaves the game**, in a stage, a menu or the pause, as it leaves the firmware's other games; the
  volume keys turn the game's volume (OPTIONS' VOLUME).
- **60 frames a second**: the board's video is drawn on the four cores (`runtime/src/workers.c`).
- **No first run on the handheld**: the game's graphics, sound and data come in its folder, taken out of
  the player's ROM set on the player's computer.

What the game writes (its settings, the EEPROM, TIME ATTACK's times) goes to its folder on the card, with
`segasonic.log`: what SDL found (its version, video and audio, the screen, the controls) and every 5
seconds the frames a second.

## Installing (for players)

Each release has `segasonic-anbernic.zip`, the folder `PORTS` without an assets pack (the CI builds it as
below, `.github/workflows/build.yml`): play `SegaSonic` on your computer once with your `sonic.zip`, put the
`segasonic.assets` its first run writes in `PORTS/segasonic/`, and copy `PORTS` into `Roms/` on the
handheld's card. The package's `README.txt` says the same, and where the computer keeps the pack.

## Building

On Windows, Linux or macOS, with [Zig](https://ziglang.org) as the compiler for the handheld: it builds C for
aarch64 Linux against the exact glibc the firmware has, with nothing else installed. Unpack it in
`build/downloads/zig/` (or give its path as `-DZIG=`), with a computer build that has the assets pack
(`build/bin/segasonic.assets`, made from your ROM set) and SDL2's development package in
`build/downloads/sdl2/` (for SDL's headers):

```bash
cmake -S . -B build/anbernic -G Ninja -DCMAKE_TOOLCHAIN_FILE=ports/anbernic/zig_aarch64.cmake -DSEGASONIC_ANBERNIC=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build/anbernic --target SegaSonic
```

It makes `build/anbernic/PORTS/`:

| File | What it is |
|---|---|
| `SegaSonic.sh` | the launcher: it starts the game in its folder, its output to `segasonic.log` |
| `segasonic/SegaSonic` | the game, an aarch64 executable that needs the firmware's `libSDL2-2.0.so.0`, `libm` and `libc` (glibc 2.35 at most) |
| `segasonic/segasonic.assets` | the assets pack of your ROM set (yours alone: never share it) |
| `segasonic/input_prompts.bmp` | CONTROLS' button pictures (Kenney's Input Prompts Pixel, CC0) |

Copy both to `Roms/PORTS/` on the handheld's card, and the game is in the firmware's PORTS.

## How it is built

- `zig_aarch64.cmake`: CMake's toolchain, Zig's `cc` for `aarch64-linux-gnu.2.35`.
- `anbernic.cmake`: the console game with `SEGASONIC_HANDHELD` (the picture fixed, the handheld's log, MENU
  and the volume keys, no first run) and `SEGASONIC_CONTROLS_PAGE` (CONTROLS in OPTIONS).
- The game uses the firmware's SDL2, which knows its screen and GPU: the build links it against
  `sdl2_stub.c`, a library of the same name with the functions the port calls, so the executable asks for
  `libSDL2-2.0.so.0` and takes the firmware's when it starts. A function the port starts to call goes
  there too (the link says which).
- The handheld's buttons come as a joystick SDL 2.0.12 does not know, "ANBERNIC-keys": the game gives SDL
  its mapping before SDL starts (`device_mappings` in `ports/sdl/main.c`), as the RG40XX H's controls test
  found them: A 0, B 1, X 3, Y 2, L1 4, L2 5, R1 10, R2 11, SELECT 6, START 7, MENU 8, L3 9, R3 12, the
  d-pad hat 0, the sticks axes 0 to 3.

## Where it stands

- The RG40XX H runs it at 60 frames a second, its sound at its speed, its buttons right.
- The 4:3 picture leaves 16 lines at the top and bottom of the screen: the game drawn 8 lines taller
  at each side (320 x 240) would fill it.
- Other H700 handhelds (RG35XX Plus, RG40XX V, RG CubeXX...) should run it as the RG40XX H does, with the
  same buttons; none has been tried. Other firmwares (muOS, Knulli, ROCKNIX) have other SDLs and paths.

Nothing of the game is in this repository: its folder is made on the player's computer, from the player's
ROM set. SegaSonic The Hedgehog is a trademark of SEGA; this project is not affiliated with or endorsed by
SEGA, nor with Anbernic.

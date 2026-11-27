# SegaSonic on the Nintendo Switch

The console version of the game (`SegaSonic`) built for the Switch, as homebrew installed from an `.nsp`. It
is the same game as on a computer, with what a game console asks for:

- **The picture is WIDE, fixed**: the Switch's screen and a TV are 16:9, and the game's 16:9 picture fills
  them (1279 of 1280 pixels in handheld mode). OPTIONS has no VIDEO page.
- **The controls are fixed**: the d-pad and the left stick move, A jumps and picks, B goes back in the
  menus (it does nothing in a stage), + starts and pauses. OPTIONS has no CONTROLS page.
- **The board's video is drawn on three cores**: the game runs on core 0, the video's rows on cores 1 and
  2 too (`runtime/src/workers.c`, libnx's threads, each on its core).
- **No EXIT** on the title's menu: HOME leaves the game, as it leaves any other.
- **No first run on the Switch**: the game's graphics, sound and data come in the `.nsp`, taken out of
  the player's ROM set on the player's computer. The Switch only installs and plays it.

What the game writes (its settings, the EEPROM, TIME ATTACK's times) goes to `/switch/segasonic/` on the
SD card, with `segasonic.log`, the steps of its start.

## Making the .nsp (for players)

You need three things of your own, which never leave your computer:

- `prod.keys`, your console's keys (the tool uses `header_key` and `key_area_key_application_00`);
- `sonic.zip`, MAME's ROM set of *SegaSonic The Hedgehog* (Japan, rev. C), or a folder of its files;
- the tool, `SegaSonic NSP` (`SegaSonic NSP.exe` on Windows), with the `switch` folder beside it: the
  game's Switch build (below); and `font.bmp`, its window's letters.

Start the tool (a double click). Its window shows what the `.nsp` will be made of, each with its state
(found, not found, or what is wrong with it):

- **Console keys**: `prod.keys` beside the tool, or in `.switch` in your user folder;
- **ROM set**: `sonic.zip` beside the tool;
- **HOME menu icon**: a 256 x 256 JPEG of yours, `icon.jpg` beside the tool (taken by itself, by SegaSonic APK
  too) or dropped on the window; else the game's own (the project's trackball);
- **The game's Switch build**: the `switch` folder beside the tool;
- **Output folder**: where `SegaSonic.nsp` goes, beside the tool unless you choose another.

Drop your files on the window and it takes each by what it is: a `.keys` (or `.txt`) is the keys, a
`.jpg` the icon, anything else the ROM set (`sonic.zip`, or a folder of its files). **Choose the output
folder** opens your system's folder dialog (on Linux zenity's or kdialog's: without either, the button
says so and the folder stays beside the tool). **Make SegaSonic.nsp**, or Enter, makes it: the window
shows the step it is at, then the `.nsp`'s path and size, or what failed.

Files dropped on the tool itself, not on its window, are taken the same way, and it makes the `.nsp` at
once in its console. With options, from a command line, it is a console program and nothing else:

```
SegaSonic NSP --keys prod.keys --rom sonic.zip --icon my_icon.jpg --out SegaSonic.nsp
```

When it cannot show its window (no display, no `font.bmp` beside it, a build without SDL 3), the tool
started with nothing makes the `.nsp` at once with the defaults, in its console, as with files dropped on
it.

Install the `.nsp` with an installer (Tinfoil, DBI, Goldleaf...) on a console that runs Atmosphère with
signature patches: the `.nsp` is not signed by Nintendo, as no homebrew `.nsp` is.

## Building

### The Switch game

With [devkitPro](https://devkitpro.org/wiki/Getting_Started): its installer's **Switch Development**,
then, in devkitPro's MSys2 shell (Windows) or a shell with devkitPro's pacman (Linux, macOS):

```bash
pacman -S --needed switch-sdl2 switch-cmake switch-pkg-config
```

Then, from the repository's folder, in that shell:

```bash
/opt/devkitpro/portlibs/switch/bin/aarch64-none-elf-cmake -S . -B build/switch -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
make -C build/switch -j6 SegaSonic_switch
```

`aarch64-none-elf-cmake` is CMake with devkitPro's Switch toolchain; `CMakeLists.txt` sees it and builds
the game alone (`switch.cmake`), into `build/bin/switch/`:

| File | What it is |
|---|---|
| `main` | the program (an NSO: the ELF `SegaSonic.elf` through `elf2nso`) |
| `main.npdm` | its permissions on the console (`segasonic.json` through `npdmtool`): title `05005E6A50510000` |
| `control.nacp` | its name, author and version (`nacptool`) |
| `icon.jpg` | its icon (`icon.jpg` here, which `tools/make_icon.py` draws) |
| `SegaSonic.nro` | only when `build/bin/segasonic.assets` exists (a computer build with your ROM set): the game with your assets inside, for the Homebrew Menu. It is yours alone: never share it |

The CI builds the same files in devkitPro's image (`.github/workflows/build.yml`, the `switch` job),
without the `.nro`.

### The tool that makes the .nsp

`SegaSonic NSP` is part of the computer build (`ports/switch/nsp/`, the target `segasonic_nsp`):

```bash
cmake --build build/cmake --config Release --target segasonic_nsp
```

Its window is built where the game's is, with SDL 3 (with `font.bmp`, and on Windows `SDL3.dll` unless
SDL 3 is built in); without SDL 3 the tool is its command line alone. It lands in `build/bin/`, beside
`build/bin/switch/`, so from the repository it makes the `.nsp` at once:

```bash
"build/bin/SegaSonic NSP" --keys prod.keys --rom rom
```

### Checking an .nsp

`tools/check_nsp.py` reads an `.nsp` as the console does, with another implementation of AES (Python's
`cryptography`): it deciphers each NCA's header with the keys, checks every hash (the PFS0s' block hash
tables, the RomFS's IVFC levels, the FS headers', the content meta's records) and takes the files out to
compare them with the ones they were made from:

```bash
python tools/check_nsp.py build/bin/SegaSonic.nsp prod.keys --pieces build/bin/switch --assets build/bin/segasonic.assets
```

It shows nothing of the keys.

## How the .nsp is made

`SegaSonic NSP` writes the Switch's formats itself, in C, from their documentation (switchbrew) and from how
hacBrewPack, hactool and Atmosphère's file system write and read them (none of their code):

1. It takes the assets out of the ROM set with the computer's first run (`runtime/src/extract.c`): the
   same `segasonic.assets`, byte for byte.
2. **The program NCA**: section 0 its ExeFS, a PFS0 of `main` and `main.npdm` under a table of SHA-256s
   of its 64 KB blocks; section 1 its RomFS, with `segasonic.assets`, under five IVFC levels of SHA-256s
   of 16 KB blocks. The game reads the pack from `romfs:/`.
3. **The control NCA**: a RomFS with `control.nacp` and the icon, a copy for each of the 16 languages.
   `control.nacp` as an installed game takes it: no user to pick at the start, no save data of the
   system's (the game keeps its files on the SD card), the system's logo handling (there is no logo).
4. **The meta NCA**: a PFS0 with `Application_05005e6a50510000.cnmt`, which lists the other two by their
   SHA-256 and size.
5. Each NCA's sections are ciphered with AES-128-CTR (the counter: the FS header's upper IV, then the
   offset / 16), its key area with AES-128-ECB by `key_area_key_application_00`, its header with
   AES-128-XTS by `header_key` (sectors of 0x200, numbered big-endian from 0). Each NCA is named by its
   SHA-256's first half, and the `.nsp` is a PFS0 of the three.

The tool checks its AES and SHA-256 against FIPS-197's, FIPS-180-4's and IEEE P1619's vectors each time it
starts (`crypto.c`). The same inputs make the same `.nsp`.

| File | |
|---|---|
| `switch.cmake` | the Switch build |
| `segasonic.json` | `main.npdm`'s settings: the title, its range, the kernel's capabilities |
| `icon.jpg` | the HOME menu's icon (`tools/make_icon.py`) |
| `nsp/crypto.c` | AES-128 (ECB, CTR, XTS) and SHA-256 |
| `nsp/formats.c` | PFS0, RomFS, the hashed sections, the NCA, the content meta |
| `nsp/make_nsp.c` | the `.nsp` from its inputs, for the command line and the window: the three NCAs, the `.nsp`; the inputs' defaults |
| `nsp/main.c` | the tool's command line; started with nothing, its window |
| `nsp/window.c` | the window (SDL 3, with Kenney Mini's letters: `ports/sdl/letters.c`): the inputs and their state, files dropped on it, the `.nsp` made on a thread of its own |
| `nsp/folder_dialog.c` | the system's folder dialog: Windows' (COM's `IFileOpenDialog`), macOS's (`osascript`), Linux's (`zenity` or `kdialog`) |

The game's own changes for the Switch are in `ports/sdl/` (`__SWITCH__`: the romfs, the SD card folder,
the screen; `SEGASONIC_HANDHELD`: the fixed picture and controls) and in `console/console.c`
(`SEGASONIC_HANDHELD`: OPTIONS without VIDEO and CONTROLS; `SEGASONIC_SYSTEM_EXIT`: no EXIT).

## Where it stands

- The `.nsp` installs and runs on Eden (an emulator). On a console it has not run yet.
- On one core the game ran at about 50 frames a second on Eden, its sound breaking up: the board's video,
  drawn by the processor, takes about 2.3 ms a frame on a computer and several times that on the Switch's.
  Its rows on three cores (the same frames: the 98 scenarios, `tools/check_scenarios.py --video`, match
  with one thread and with three) took Anbernic's H700 from 45-60 to 60 frames a second; the Switch's
  frames a second are in `segasonic.log`.
- The 16:9 picture still has the original's 4:3 in the opening's scenes, the cage room and the island maps.

Nothing of the game, the keys or the player's own icon is in this repository: the `.nsp` is made on the
player's computer, from the player's files. SegaSonic The Hedgehog is a trademark of SEGA; this project is
not affiliated with or endorsed by SEGA, nor with Nintendo.

# SegaSonic on Android

The console version of the game (`SegaSonic`) built for Android phones and tablets (Android 7 or later), as an
APK. It is the same game as on a computer, with what a phone asks for:

- **The picture as the screen's shape asks**: a phone's screen is 16:9 or wider, and the game's 16:9 picture
  (WIDE) fills its height. On a screen tall enough for the game's own 4:3 picture with a fifth of it left
  under it (an open Galaxy Z Fold's), that picture goes at the top and the touch controls under it; a fold
  opened or closed mid-game changes it at once. Where the picture leaves the screen, OPTIONS' Sonic heads
  stand still. OPTIONS has no VIDEO page.
- **Touch controls**, over the picture or under it (`ports/sdl2/touch.c`): a stick where your left thumb lands
  on the screen's left half (it rolls the trackball as far as it leans, and moves the menus' cursor), **A**
  (jump, pick) and **START** (start, pause), the same size on every phone. They hide when a game controller
  or a keyboard is used, and come back when a finger touches the screen.
- **Game controllers**, Bluetooth or USB: the d-pad and the left stick move, A jumps and picks, B goes back in
  the menus (it does nothing in a stage), Start starts and pauses. OPTIONS has no CONTROLS page.
- **Android's back** (the button or the gesture) goes back in the menus and does nothing in a stage, so a
  thumb at the screen's edge never ends a game.
- **EXIT is in the pause**, under CONTINUE (up and down, then A; or a finger on it): it ends the game, its
  settings and EEPROM written. The title's menu has no EXIT: Android's home leaves the game there, as it
  leaves any other. Going to the background pauses a stage and writes what the game keeps.
- **The board's video is drawn on several cores** (`runtime/src/workers.c`): the game's rows on up to three
  of the phone's cores beside the game's.

What the game writes (its settings, the EEPROM, TIME ATTACK's times) stays in the app's own folder; its log,
`segasonic.log`, is in `Android/data/io.github.cheviman.segasonic/files/` on the phone's storage.

## Making the APK (for players)

Android makes nothing from the ROM set: the APK carries the game's assets, taken out of the player's ROM set
on the player's computer, so each APK is its player's alone: **never share it**. `SegaSonic APK`, a tool in
the computer's package (`apk/`), makes it: drop your `sonic.zip` on its window (and a square PNG or JPEG for
your own launcher icon), **Make SegaSonic.apk**, copy it to the phone and open it there, or
`adb install -r SegaSonic.apk`. It signs the APK with a key of yours, made the first time and kept in your own
folder (`%APPDATA%\SegaSonic` on Windows): keep it, as only an APK with the same key installs over the last one
and keeps what the game wrote. How it works: [apk/README.md](apk/README.md).

## Building

The game's Android build, which SegaSonic APK makes the player's APK of, and a computer's own APK with its
assets pack. You need:

- the Android SDK's command-line tools, with `platforms;android-35`, `build-tools` (35 or newer) and the NDK
  (28 or newer), installed with `sdkmanager`;
- a JDK 17 (`JAVA_HOME`);
- CMake and Ninja.

Then, from the repository's folder:

```bash
cmake -S . -B build/android -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=<sdk>/ndk/<version>/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24
```

```bash
cmake --build build/android --target segasonic_apk --parallel 6
```

`CMakeLists.txt` sees the NDK's toolchain and builds the game alone (`android.cmake`): SDL2 from its source
(2.32.10, `FETCHCONTENT_SOURCE_DIR_SDL2` picks a copy already on the computer), the game as `libmain.so`, and
two APKs around them, put together by the SDK's own tools, without Gradle:

- `build/bin/android/segasonic-base.apk`: the game without an assets pack or a signature, which SegaSonic APK
  makes a player's APK of;
- `build/bin/android/SegaSonic-debug.apk`: this computer's own, with its pack (`build/bin/segasonic.assets`),
  signed with the debug key of its Android tools.

| Step | Tool | What it makes |

|---|---|---|
| SDL's Java (`org.libsdl.app.SDLActivity`, from SDL's source) | `javac`, `d8` | `classes.dex` |
| The manifest, the icon, the assets (the pack stored, read in place) | `aapt2` | the APK's base |
| `libmain.so` and `libSDL2.so` (without their symbols), `classes.dex` | `llvm-strip`, `aapt` | the APK |
| | `zipalign` (and `apksigner`, this computer's) | the APKs, aligned (and signed) |

To give this computer's APK an icon of your own on the launcher, a square JPEG or PNG:
`-DSEGASONIC_ANDROID_ICON=my_icon.jpg` (the launcher shows it whole, its shape cutting the corners:
`res/mipmap-anydpi-v26/ic_launcher_own.xml` in `ic_launcher.xml`'s place, as SegaSonic APK puts it). By default
it has the project's trackball (`res/`, `tools/make_icon.py`: an adaptive icon's two layers).

Install it from the computer with `adb install -r build/bin/SegaSonic.apk`, or copy it to the phone and open it
there (Android asks to allow installing from that app once).

| File | |
|---|---|
| `android.cmake` | the Android build: SDL2, `libmain.so`, the APK |
| `AndroidManifest.xml` | the app: SDL's activity, landscape, full screen |
| `res/` | the launcher's icon (`tools/make_icon.py`), and `ic_launcher_own.xml` and `ic_launcher_art.png` for a player's own picture |
| `apk/` | SegaSonic APK, the tool that makes a player's APK on their computer ([apk/README.md](apk/README.md)) |

The game's own changes for Android are in `ports/sdl2/` (`__ANDROID__`: the pack in the APK, the app's
folders, back and the background; `SEGASONIC_TOUCH`: the touch controls, `touch.c`; `SEGASONIC_PAUSE_EXIT`: the
pause's CONTINUE and EXIT) and in `console/console.c` (`SEGASONIC_HANDHELD`: OPTIONS without VIDEO and CONTROLS;
`SEGASONIC_SYSTEM_EXIT`: no EXIT on the title; `SEGASONIC_PAUSE_EXIT`: the pause's choices).

Nothing of the game is in this repository: the APK is made on the player's computer, from the player's files.
SegaSonic The Hedgehog is a trademark of SEGA; this project is not affiliated with or endorsed by SEGA, nor
with Google.

# SegaSonic APK

`SegaSonic APK` (`SegaSonic APK.exe` on Windows) makes the Android game's APK on the player's computer, as
`SegaSonic NSP` makes the Switch's `.nsp`: the game's Android build with the assets of the player's ROM set,
signed with a key of the player's own. Nothing it reads or writes leaves the computer, and the APK is its
player's alone: **never share it**.

## Making the APK (for players)

You need:

- `sonic.zip`, MAME's ROM set of *SegaSonic The Hedgehog* (Japan, rev. C), or a folder of its files;
- the tool, with the `android` folder beside it, which holds `segasonic-base.apk`: the game's Android build,
  without the assets and without a signature; and `font.bmp`, its window's letters.

Start the tool (a double click). Its window shows what the APK will be made of, each with its state (found,
not found, or what is wrong with it):

- **ROM set**: `sonic.zip` beside the tool;
- **Launcher icon**: a square PNG or JPEG of yours, at most 2048 x 2048, which the launcher shows whole, its
  shape cutting the corners: `icon.jpg` beside the tool (taken by itself, by SegaSonic NSP too) or one dropped
  on the window; else the game's own (the project's trackball);
- **The game's Android build**: `segasonic-base.apk` in the folder `android` beside the tool;
- **Signing key**: yours, kept from before, or made with the first APK (below);
- **Output folder**: where `SegaSonic.apk` goes, beside the tool unless you choose another.

Drop your files on the window and it takes each by what it is: an `.apk` is the game's Android build, a
`.png` or `.jpg` the icon, anything else the ROM set (`sonic.zip`, or a folder of its files). **Choose the
output folder** opens your system's folder dialog; **Make SegaSonic.apk**, or Enter, makes it: the window
shows the step it is at, then the APK's size, or what failed.

Files dropped on the tool itself, not on its window, are taken the same way, and it makes the APK at once in
its console. With options, from a command line, it is a console program and nothing else:

```
SegaSonic APK --rom sonic.zip --icon my_icon.png --base android/segasonic-base.apk --out SegaSonic.apk
```

When it cannot show its window (no display, no `font.bmp` beside it, a build without SDL3), the tool started
with nothing makes the APK at once with the defaults, in its console.

Copy `SegaSonic.apk` to the phone and open it there (Android asks once to allow installing from that app),
or install it from the computer with `adb install -r SegaSonic.apk`.

### The signing key

Android installs an APK over the one installed only when both are signed with the same key. The first time,
the tool makes a key of yours with the system's random generator, and keeps it in your own folder:
`%APPDATA%\SegaSonic` on Windows, `~/Library/Application Support/SegaSonic` on macOS, `$XDG_CONFIG_HOME/SegaSonic`
(`~/.config/SegaSonic`) on Linux. There are two files: `apk_key.pk8`, the private key (PKCS #8), and
`apk_key.x509.der`, its certificate. Every APK the tool makes signs with them, so each installs over the last
and the game keeps what it wrote (its settings, the EEPROM, TIME ATTACK's times). Keep them, and never share
them: with another key, the game has to be uninstalled before an APK installs, and what it wrote goes with
it. The tool never replaces a key it cannot read; it stops and says so.

An APK built with `android.cmake` is signed with the computer's debug key, not with yours: uninstall it
before the first APK of the tool. `apksigner` takes the same key (`--key apk_key.pk8 --cert apk_key.x509.der`).

## How the APK is made

`SegaSonic APK` writes the zip and its signature itself, in C, from their documentation (PKWARE's
APPNOTE.TXT, Android's APK Signature Scheme v2, FIPS-186-4, RFC 6979) and from what `zipalign` and
`apksigner` write and check (none of their code):

1. It takes the assets out of the ROM set with the computer's first run (`runtime/src/extract.c`): the
   same `segasonic.assets`, byte for byte.
2. It reads `segasonic-base.apk` as a zip, by its central directory, and leaves out any signature it has
   (`META-INF`'s `MANIFEST.MF`, `.SF`, `.RSA`, `.DSA` and `.EC`; an APK Signing Block) and any assets pack.
3. With an icon of yours, the build's icon entries take it: `res/mipmap-anydpi-v26/ic_launcher.xml`, the
   launcher's adaptive icon, takes the bytes of `ic_launcher_own.xml` beside it (compiled XML, whole in
   itself: an adaptive icon whose background is `ic_launcher_art` inset by 18 dp), and
   `res/mipmap-xxxhdpi-v4/ic_launcher_art.png` and `ic_launcher.png` (the legacy icon) your picture's
   (Android's decoders tell a PNG from a JPEG by its bytes, whatever the entry's name). A build without
   those entries does not take an icon, and the tool says so.
4. **The APK**: the build's entries, copied packed as they are, then `assets/segasonic.assets`, stored (the
   game reads it in place, through Android's asset manager). Each stored entry's data starts at a multiple
   of 4, a library's (`.so`) at 4096, as `zipalign -p 4` aligns them: apksigner's alignment field (0xD935)
   in its local header.
5. **Its signature**, APK Signature Scheme v2: the SHA-256 of each 1 MB of the entries, of the central
   directory and of the end record, then the SHA-256 of those; signed with ECDSA on P-256 with SHA-256 (its
   nonce RFC 6979's), with the certificate and the public key, in the APK Signing Block before the central
   directory. Android takes v2 from 7.0 (API 24, the game's least), so there is no v1 (JAR) signature.

The tool checks its SHA-256, HMAC-SHA256 and ECDSA against FIPS-180-4's, RFC 4231's and RFC 6979's vectors
each time it starts, and the APK's signature before it writes it. The same inputs and key make the same APK,
byte for byte, on any computer.

## Building

`SegaSonic APK` is part of the computer build (the target `segasonic_apk_maker`):

```bash
cmake --build build/cmake --config Release --target segasonic_apk_maker
```

Its window is built where the game's is, with SDL3 (with `font.bmp`, and on Windows `SDL3.dll` unless SDL3 is
built in); without SDL3 the tool is its command line alone. It lands in `build/bin/`, beside
`build/bin/android/`, so from the repository it makes the APK at once:

```bash
"build/bin/SegaSonic APK" --rom rom
```

An APK is checked with the SDK's own tools: `apksigner verify --verbose --min-sdk-version 24 SegaSonic.apk`
and `zipalign -c -p 4 SegaSonic.apk`.

| File | |
|---|---|
| `main.c` | the tool's command line; started with nothing, its window |
| `make_apk.c` | the APK from its inputs, for the command line and the window: the build's entries, the icon, the assets; the inputs' defaults |
| `zip.c` | the zip: the build's entries read, the APK's written aligned |
| `sign.c` | APK Signature Scheme v2: the digests, the signed data, the APK Signing Block |
| `key.c` | the player's key: made once, kept as PKCS #8 with a self-signed X.509 certificate, read back |
| `ecdsa.c` | ECDSA on P-256 with SHA-256 (RFC 6979's nonce) and HMAC-SHA256; SHA-256 is SegaSonic NSP's (`ports/switch/nsp/crypto.c`) |
| `window.c` | the window (SDL3, with Kenney Mini's letters, `ports/sdl/letters.c`, and SegaSonic NSP's folder dialog, `ports/switch/nsp/folder_dialog.c`) |

Nothing of the game, the ROM set, the key or the player's own icon is in this repository: the APK is made on
the player's computer, from the player's files. SegaSonic The Hedgehog is a trademark of SEGA; this project is
not affiliated with or endorsed by SEGA, nor with Google.

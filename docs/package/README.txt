SegaSonic The Hedgehog - native port
https://github.com/cheviman/segasonic-port

Two games come in this package:

  SegaSonic          The console version: play this one. No coins; a menu (START GAME,
                     TIME ATTACK, MULTIPLAYER, OPTIONS, EXIT), up to three players, WIDE
                     16:9 or 16:10 stages, VSYNC.

  SegaSonic Arcade   The arcade game as the board runs it, coins and all:
                     5 puts a coin in, 1 starts, Escape quits.

THE FIRST RUN
The game needs your own MAME ROM set: sonic.zip (SegaSonic The Hedgehog, Japan, rev. C).
Put it beside the game (beside the .app on macOS) before starting it, or drop it on the
window that asks for it. The game takes out what it uses, once; it does not need the
ROM set again.

PLAYING SEGASONIC (player 1)
  Move                arrows or WASD, or the mouse (after a click in the window)
  Jump, spin          Z or Space, or the left mouse button
  Start, pause        Enter or P
  Back, pause         Escape or Backspace
Players 2 and 3, game controllers and every key: OPTIONS > CONTROLS.
F11 or Alt+Enter: full screen.

MULTIPLAYER
Up to three devices on the same local network, a player each (computers, phones, a Switch,
a handheld, all of the same version). One picks HOST, the others JOIN and its room; NAME is
the name the room shows. The host's START begins the game on every device; the others join
at player select with START. Back (Escape) leaves the game; the others play on. If the
system asks whether SegaSonic may use the network, allow it on private networks.

THE MOUSE, AND CLOSING THE GAME
In SegaSonic the mouse is a pointer until you click in the window: from then on it is
player 1's trackball and the pointer hides. Escape pauses the game and gives the pointer
back (the window's close button); Escape again goes on, and a click takes the mouse again.
Alt+F4 (Windows) or Cmd+Q (macOS) closes the game at any time. On the title, EXIT.
In SegaSonic Arcade the mouse is always the trackball, and Escape quits.

NINTENDO SWITCH AND ANDROID
Two more programs come in this package. They make the game for those on this computer,
from your sonic.zip; what they make carries your ROM's assets: never share it.

  SegaSonic NSP      The Switch's .nsp. Put sonic.zip and your console's prod.keys beside it,
                     open it and press Make SegaSonic.nsp; install the .nsp as any homebrew
                     one (Atmosphere with signature patches). The folder switch stays beside it.

  SegaSonic APK      Android's APK (Android 7 or later). Put sonic.zip beside it, open it and
                     press Make SegaSonic.apk; copy the APK to the phone and open it there.
                     The folder android stays beside it. Keep the signing key it makes the
                     first time (%APPDATA%\SegaSonic on Windows, ~/.config/SegaSonic on Linux,
                     ~/Library/Application Support/SegaSonic on macOS): a new APK installs
                     over the last one only with the same key.

Files somewhere else (the keys, sonic.zip, a picture for your own icon) can be dropped on
their windows. A picture of yours named icon.jpg beside them (a 256 x 256 JPEG) is the
icon of both, the Switch's and the phone's, without dropping it.

MACOS
The apps are not signed by a developer. The first time, right click the app > Open (on
macOS 15 and later: try to open it, then System Settings > Privacy & Security > Open
Anyway). If macOS says the app is damaged, run this in Terminal, in this folder:
  xattr -dr com.apple.quarantine *.app

WHERE THE GAME KEEPS ITS FILES
Windows: beside the game. Linux: ~/.local/share/SegaSonic.
macOS: ~/Library/Application Support/SegaSonic.

No game data is included in this package. SegaSonic The Hedgehog is a trademark of SEGA;
this project is not affiliated with or endorsed by SEGA. Licenses: LICENSE.txt, licenses/.

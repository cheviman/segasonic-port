SegaSonic The Hedgehog for iPhone and iPad
==========================================

Sega's SegaSonic The Hedgehog (1993), the console game for up to three players, as a native game: decompiled
to C and checked frame by frame against MAME, github.com/cheviman/segasonic-port. This is the package for
iOS and iPadOS (13 or later, an iPhone or an iPad of the last years: arm64): the game as an .ipa file,
without its assets and not signed. Nothing of the game comes in this package: the game takes its graphics
and sounds out of your own ROM set.

It is not on the App Store, and never will be: the game is Sega's, and you bring the ROM set. You install
it yourself ("sideloading"), and the tool you install it with signs it for your device.

What you need
-------------
- sonic.zip, MAME's ROM set of SegaSonic The Hedgehog (Japan, rev. C), as MAME has it;
- an iPhone or an iPad, and a computer (Windows or macOS) for the tool that installs the game.

Installing
----------
The .ipa file is in ios/: SegaSonic.ipa. Install it with one of these, which sign an app with your own
Apple ID as they install it:

- AltStore (altstore.io) or SideStore (sidestore.io): install it on the device as its site says, then open
  the .ipa with it (AltStore's My Apps, the + at the top).
- Sideloadly (sideloadly.io), on a computer: the device on USB, drop the .ipa on its window, your Apple ID,
  Start.

With a free Apple ID an app installed so stops opening after 7 days: open AltStore or SideStore to refresh
it (they do it themselves in the background with their companion on the computer), or install it again
with Sideloadly; a refresh keeps the game's folder as it was. A paid developer account signs for a year.
Where the iOS version allows it, TrollStore installs an .ipa for good.

The first time on iOS 16 or later, the device asks for Developer Mode (Settings, Privacy & Security,
Developer Mode) and to trust your Apple ID (Settings, General, VPN & Device Management).

The ROM set
-----------
The game has its own folder, which the Files app shows: open the game once (it says it has no ROM set),
then in the Files app go to On My iPhone (On My iPad), SegaSonic, and put sonic.zip there. From a computer:
Finder (or the Apple Devices app on Windows), the device, Files, the game, and drop sonic.zip on it. Open
the game again: the first start takes the assets out of sonic.zip into segasonic.assets there (some
seconds); from then on the game starts from it, and sonic.zip can go.

What the game makes from your ROM set is yours alone: never share it.

Playing
-------
The touch screen has a stick where your left thumb lands, which rolls the trackball, A (jump, pick) and
START (start, pause) at the right. A game controller (MFi, Xbox, PlayStation, Switch Pro) hides them, and
each controller is a player (player 1 is Sonic, 2 Mighty, 3 Ray): the d-pad and the left stick move, A
jumps and picks, B goes back in the menus (it does nothing in a stage), and Start starts and pauses. The
pause has CONTINUE and EXIT. Going to the home screen pauses the game; swipe up from the bottom (twice: the first swipe only
shows the bar) to leave the game. The game keeps its settings and its log (segasonic.log) in its folder,
beside the assets.

Licenses
--------
LICENSE.txt is the project's (BSD-3-Clause). licenses/ holds MAME's and ymfm's (the board's video and
sound follow their behavior and tables), arcade-lan's (BSD-3-Clause: MULTIPLAYER), SDL's and
SDL_GameControllerDB's (zlib: inside the game) and Kenney's (CC0: the letters, the controls' pictures).

SegaSonic The Hedgehog is a trademark of SEGA; iPhone, iPad and iOS are trademarks of Apple. This project
is not affiliated with or endorsed by them.

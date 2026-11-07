# The console build: what it changes, and what is next

Done:
- No licence screen: power-on goes straight to the title (the SEGA logo, then SegaSonic's).
- The attract mode in the console's order: title, ranking, demo, title, ranking, the other demo.
- No coins: the game's own free play, without its "Free play" line.
- The attract mode with every sound and the music (the arcade's lets some effects through, and only with
  ADVERTISE SOUND on); the title plays the map's music (sound 3; the arcade's title has none).

- The title menu, PRESS START, player select by a touch, pause in grey, actions instead of keys.
- The SEGA logo's jingle: two chords of our own (E flat major, C major) through the sound driver's own MIDI queue on the effects'
  channels B and C (an octave under), instruments from segasonic.ini; it stays (no recorded voice).
- (C) SEGA 1992 on the title and its menu only.

- OPTIONS, a page of its own over the ranking's drifting Sonic faces: DIFFICULTY (VERY EASY, EASY,
  NORMAL, HARD: the test mode's 1-4) and VITALITY (the EEPROM, as the test mode keeps them), CONTINUES,
  VIDEO (a page: WINDOW, FULL SCREEN, PIXELS: sharp, smooth as sharp bilinear, or plain bilinear), CONTROLS, VOLUME
  (segasonic.ini), SOUND TEST (the game's ten songs; the one played becomes the title's music,
  title_music in segasonic.ini, stage 3's at first), STAGE SELECT (the island map, then player select),
  BACK. Direct3D 11 on Windows.
- CONTINUES (segasonic.ini, 3 at first): a game starts with the credits of its start and that many
  continues, shared (a player who joins takes them too), free play off until it ends; FREE is the game's
  own free play. With too few credits the start prompt shows nothing, not "Insert coin": the credit
  count beside it says 0.
- STAGE SELECT's level starts after player select, in the how to play's place (player select runs with
  stage 7, as in a game, so the chosen stage's objects do not run behind it).
- The Japanese captions in English: the game's own English boxes, which its program never shows
  (place_message_box). The how to play: CONTROLS in its box and the line in yellow, their outlines dark
  as the Japanese ones' (a pen of their palettes). Eggman's lines without their red box: white letters
  with a dark outline, made at start on the port's copy of the sprite graphics (console_prepare_sprites).
  "haaa!!" (kind 0x12), which has no English box, is "Huh!" in its frame's place: h, u and ! cut from the
  English lines, an H of their strokes.
- STAGE SELECT on the island map (game mode 8): the map as the map screen fades it in, every level open
  (the opening, Eggman blinking for it, then the 7 markers, each one's number and name blinking); the
  levels cleared in this session's games in the map's own cleared colours, in memory only (no file: it is
  not a continue). A pick starts the level's first stage as the original's stage select does.
- CONTROLS in OPTIONS, for players 1-3 (PLAYER < 1 >): each action's first two keys and its game
  controller button, drawn with Kenney's Input Prompts Pixel (CC0, ports/sdl2/prompts); a line picked waits
  for the new key or button (Escape leaves it), DEFAULTS puts that player's back; kept in controls.ini
  beside the executable, a section a player. Each player rolls and jumps with their own keys and game
  controller (plugged in in their turn); the menus take any player's.
- A player's start prompt up (to join, or a continue's count): their start joins, it does not pause.
  The join chooser in a stage (a continue) turns at a touch, as player select's does.
- The 3-player cabinet: with the 2-player one the original draws player 3's start prompt in the middle
  of the stage (it reads past start_prompt_positions_2p).
- WIDE (VIDEO, segasonic.ini, off at first): a 16:9 window (the video's 416 pixels less 9 at each side).
  The stages and the how to play drawn wider (s32_video_set_wide: 48 more pixels of the tilemaps, the
  bitmap and the sprites at each side, the text layer and so the HUD where they were); their tile sets
  from the blocks of the wide left edge on (set_nbg0_scroll_from_camera); the how to play's panel strip
  copied at the sides; the SEGA logo, the title, the ranking, OPTIONS (CONTROLS' icons moved with it:
  s32_video_side), player select and the attract demos too. The scenes, with 320-pixel pictures, between
  bars. The window takes the largest whole
  scale under WINDOW at which it fits the screen, so a 16:9 one stays 16:9.
- The icon: Sonic's, Mighty's and Ray's heads (the join chooser's) from the game's assets: the window's at
  run time (console_window_icon). The .exe carries nothing of the game's: its icon, and the .apps', is
  ours, a trackball with speed lines (tools/make_icon.py, ports/sdl2/icon/; v1.1.0).

- PIXELS' CRT: scanlines, a soft RGB phosphor mask and a little vignette over smooth pixels (a
  multiplying overlay the size the picture is shown at).
- TIME ATTACK (the title's menu): the map picks what is timed, Eggman's place (ALL GAME, a label made of
  the map labels' own letters in the unused ROBOTNIK'S TOWER label) the whole game, a marker its level.
  The clock on top runs through stages, scenes and continues; a level stops at its stage clear banner, the
  whole game at the ending, with a split at each level's end. BEST TIMES after a level (each level's best
  alone and the best whole game), BEST RUN after the whole game (its splits beside the best run's); kept
  in segasonic.times. Players not in the run when its clock starts cannot join it, nor see its prompt.
- WIDE in every mode of a game or a demo (not the opening's scenes and cage room, nor the island maps).
- A player out of play has no ring count on screen (player select leaves them with the 3-player cabinet).
- WIDE's sides of a stage: the stages are drawn as far as the board's camera looks (past it they end, or
  keep a piece of somewhere else). Each cell of the map no camera place on the stage's path shows (the
  sections' starts, straight from one to the next; the last on and on) is drawn, while the frame is,
  as the repeat of its row (straight or along the isometric slopes, or a wall's rows up and down) that
  also holds for the cells beside and above it; else as the one above it, or the texture beside it
  mirrored where the map is cut straight across. Past the map's width (the pages hold another end of it
  there) a cell takes what the map draws next where it holds the same run of the row's cells (beyond_word:
  a wall's bricks go on as the designer laid them). The video RAM is the game's again once the frame is
  drawn (console_draw_frame, the ports' frame_shown). Stage 0, the cage room's switch scene (the tower's
  platform in the sky, the fall by the waterfall), keeps the map's own sides: its camera is the scene's,
  and the map holds the tower whole (25-10-2026: the sides took the sky there, cutting the tower at the
  board's 320 pixels; found by Sebas on Android).
- Start pauses the stage's events too (the scenes the game plays in it: the seesaw's, ...).
- Debug: F7 and F8 move the camera to the stage's section before or after (the game's unused scroll skip);
  segasonic_console_headless's --actions has them as section- and section+.

Done 20-10-2026 (segasonic-decomp first, merged here):
- No executable carries the ROM: the game loads segasonic.assets, the reference reads rom/ (0883c7c).
- Linux x64 (GCC), macOS universal (Clang, an .app) and Windows x64 build on GitHub Actions with no ROM
  set, SDL2 inside the executables (SEGASONIC_STATIC_SDL2), a package each; what the game writes goes to
  the user's folder outside Windows (SDL_GetPrefPath).
- The first run: with no pack, sonic.zip beside the game or dropped on its window (Kenney Mini letters)
  becomes the pack, checked by CRC32, the same byte for byte as make_pack.py's; the ROM set is never
  opened again.

Done 21-10-2026:
- Linux checked 1:1 in WSL (Ubuntu 24.04, GCC 13 and Clang 18, no warnings), segasonic-decomp and this port:
  the 98 scenarios match the references frame by frame (board, picture, sound; check_scenarios.py --bin
  --out, the references copied); the console build's runs the same as on Windows (the screenshots' runs to
  frame 3600: the menus, STAGE SELECT, three players, WIDE); the first run's pack the same byte for byte, from
  the folder and from a deflated .zip. CMake had taken the Windows SDL2 package on Linux in a tree shared
  with WSL: only on Windows now (segasonic-decomp 8356205).
- The first run on macOS looks beside the .app too (the base is its Resources folder; 3761a47).
- The README for players: download, the first run, the console version, the controls, the files; the
  decompilation by a link to segasonic-decomp. Its screenshots are the console build with WIDE, made by
  tools/make_screenshots.py on segasonic_console_headless, whose --actions presses the console's actions.

Done 07-11-2026 (v1.4.0):
- MULTIPLAYER (Sebas: "el multiplayer lan con el arcade-lan nuestro"): the title menu's line between TIME
  ATTACK and OPTIONS (the menu a row higher, MENU_ROW 22). Its pages in OPTIONS' place over the faces
  (console.c: HOST, JOIN, NAME, BACK; NAME's letters; JOIN's rooms; the room), the game through arcade-lan in
  lockstep (lan.c, lan/ a submodule, as TMNT's): up to three devices, a player each in the room's order. The
  host's START resets every device's board as at power-on (the port leaves the program and runs it again:
  s32_init, the sound board's chips and Z80 as at power-on, sound_cpu_power_on; console_power_on), with the
  host's EEPROM, CONTINUES and WIDE (wide_on: the game's frame; each window shows its own part of it), and
  starts a game as START GAME does. Each VBLANK every device gives its player 1's trackball turn (a byte, the
  rest the next frame), button and START, and the board takes every slot's; START is the board's start where
  the player's prompt is up or out of a stage (no pause: BACK leaves the game, TMNT's). The work RAM's and the
  shared RAM's hash goes to the host. The game over, the host gone, out of step or BACK: the board reset with
  this device's own EEPROM (never written during a game) and the MULTIPLAYER page. While the frames hold,
  WAITING FOR the device's name on the pause's row, drawn on a copy of the frame (the text layer put back).
  Checked with two headless runs (--pace, --lan-name, --lan-hashes): 2495 frames the same, and a guest's BACK.
- VSYNC in OPTIONS > VIDEO (on at first, segasonic.ini; the computers only): SDL_RenderSetVSync, the game at
  its own pace, a game's frame a frame late not shown (frame_behind; the pause's and a held frame's always).
- WIDE 16:10 (the Steam Deck): OFF, 16:10, 16:9 in OPTIONS; segasonic.ini names them (yes is 16:9); the
  window's middle 358 columns of the same 416. Android picks 16:10 on a screen nearer it.
- @cheviman over the title's (C) SEGA 1992 (the menu's font: the copyright's has capitals alone and its '@'
  is the copyright sign), and the version (CMakeLists.txt's project version) at OPTIONS' bottom right.
- TMNT's players' reports, here too: SDL_GameControllerDB built in (the computers and Android), a hint before
  SDL starts with the player's gamecontrollerdb.txt after it; a TV's remote after the game controllers; the
  controllers and the first presses in the log; the touch controls' size with a top in inches and their
  labels inside their buttons; Android's back goes on from the pause.

Next (in this order unless Sebas says otherwise):
1. Done (25 October): macOS played on a Mac, the release's segasonic-macos-universal (unsigned: right click
   → Open, or Open Anyway): Sebas approved it.
2. Quiver Launcher: v1.1.0 is the release it shows (a tag v* makes the release from the CI's packages, its
   notes in docs/releases/; the packages carry their licenses, the C runtime on Windows, the .apps signed ad
   hoc, the icon of ours). Sebas submits it (its account), declared mostly AI-generated, with a description
   and the screenshots in docs/screenshots/.
3. Done in segasonic-decomp (22-23 October): the C reads all of the program's data by name (the 1137 reads
   through ROM addresses are tables and structures over the loaded assets), and its other leftovers are
   counted down to what the original does (segasonic-decomp's README, "What is left, and why").
4. Done (23-25 October, the game devices): the Nintendo Switch (the .nsp made on the player's computer by
   SegaSonic NSP; 16:9, no EXIT, the video on three cores; tried on Eden) and Anbernic's H700 handhelds
   (the RG40XX H's stock firmware, built with Zig; 4:3, 60 frames a second on four cores, its buttons, MENU
   and the volume keys): ports/switch/README.md, ports/anbernic/README.md. Android too (25 October): the
   APK made on the player's computer by SegaSonic APK, signed with the player's own key; 16:9, touch
   controls (a stick, A, START) or a game controller, CONTINUE and EXIT in the pause; played on Sebas's
   phone (ports/android/README.md). Both tools have a window. Released as v1.2.0. Next for them: the Switch
   on a console, the 4:3 picture 8 lines taller at each side (no bars on 640 x 480), the sound on a thread
   of its own. Anbernic's package comes from the CI too (segasonic-anbernic.zip, the player adds the assets
   pack).
5. Achievements: our own, defined on the game's memory as RetroAchievements does (the work RAM is the
   original's, byte for byte); RetroAchievements itself only runs in the emulators it approves.
6. The bitmap layer in the ice and tower stages' dark parts: the original leaves it on, 8 bits a pixel, over
   video RAM's start, where the tilemaps' pages are, and MAME draws them as rows of dots (the arcade build
   does as MAME; the console hides it). If the board does not (footage or photos of a real one), a fix for
   MAME's segas32 video.

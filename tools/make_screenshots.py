"""
Name:        make_screenshots.py
Description: makes the README's screenshots of the console build (docs/screenshots/): each one a run of
             segasonic_console_headless with WIDE on, the menus driven by its actions and the players by a
             control script, and one frame of it as the window shows it, at twice the game's size
Version:     0.1.0
Created:     21-10-2026
Modified:    29-10-2026
Author:      Sebastian Toyos

Usage: python tools/make_screenshots.py [--check] [NAME ...]
Writes docs/screenshots/NAME.png for the shots below (all of them by default), from build/bin's console
headless build and the assets pack beside it. With --check it writes nothing: each shot must match its
picture there pixel for pixel, so a change that alters the console's screens shows (a merge of the
decompilation that left OPTIONS with the title's menu under it, 23-10-2026).

A frame is what the window shows with WIDE on: the video's 416 pixels less 9 at each side, or a 320-pixel
picture between bars (the scenes), 398 x 224 doubled with sharp pixels. The actions are the console's
(--actions: "FRAME ACTION [FRAMES]"), the script the board's controls (tests/controls' format).
"""
import subprocess
import sys
import tempfile
from pathlib import Path

from PIL import Image, ImageChops

ROOT = Path(__file__).resolve().parent.parent
BIN = ROOT / "build" / "bin"
OUT_DIR = ROOT / "docs" / "screenshots"

WIDE_WIDTH = 398        # the window's 16:9 picture (416 less 9 at each side)
HEIGHT = 224
SCALE = 2

# The title's menu, opened with start; OPTIONS is its third line
MENU = ["700 start 4"]
OPTIONS = MENU + ["740 down", "770 down", "800 accept"]
# OPTIONS' STAGE SELECT (its ninth line): the island map, the cursor at the opening (Eggman's console)
STAGE_SELECT = OPTIONS + [f"{840 + 30 * line} down" for line in range(8)] + ["1060 accept"]
# OPTIONS' TRACKBALL (its sixth line): the how to play's floor; accept tries the settings, Sonic moves
TRACKBALL = OPTIONS + [f"{840 + 30 * line} down" for line in range(5)] + ["1000 accept", "1100 accept"]
# Three players (the console's cabinet has three): players 2 and 3 join at player select (the map's pick at
# 1330), everyone picks a character, then they roll on
THREE_PLAYERS = [
    "1340  press start2",
    "1360  press start3",
    "1440  press button1",
    "1460  press button2",
    "1480  press button3",
    "1700  roll 1 8 8",
    "1700  roll 2 6 9",
    "1700  roll 3 9 5",
]

# name: (frame, actions, script, what it shows)
SHOTS = {
    "title_menu": (730, MENU, [], "the title and its menu"),
    "volcanic_vault": (1800, STAGE_SELECT + ["1200 right", "1240 up", "1280 right", "1330 accept"], THREE_PLAYERS,
                       "three players in Volcanic Vault (level 1)"),
    "ice_caption": (2660, STAGE_SELECT + ["1200 up", "1330 accept"], THREE_PLAYERS,
                    "Eggman's line in English in the ice level (level 2)"),
    "trap_tower": (1600, STAGE_SELECT + ["1200 right", "1240 up", "1330 accept"], THREE_PLAYERS,
                   "three players in Trap Tower (level 4)"),
    "options": (900, OPTIONS, [], "OPTIONS"),
    "stage_select": (1290, STAGE_SELECT + ["1200 right", "1240 up"], [], "STAGE SELECT on the island map"),
    "trackball": (1150, TRACKBALL, ["1110  roll 1 -10 -10"], "TRACKBALL: Sonic tried on the how to play's floor"),
}


def executable():
    path = BIN / "segasonic_console_headless.exe"
    return path if path.exists() else BIN / "segasonic_console_headless"


def window_picture(frame):
    """The frame as the window shows it with WIDE on, doubled"""
    if frame.width > WIDE_WIDTH:
        side = (frame.width - WIDE_WIDTH) // 2
        picture = frame.crop((side, 0, side + WIDE_WIDTH, HEIGHT))
    else:
        picture = Image.new("RGB", (WIDE_WIDTH, HEIGHT))
        picture.paste(frame, ((WIDE_WIDTH - frame.width) // 2, 0))
    return picture.resize((WIDE_WIDTH * SCALE, HEIGHT * SCALE), Image.NEAREST)


def shoot(name, work, check=False):
    """The shot into docs/screenshots; with 'check', True when it matches the picture there."""
    frame, actions, script, _ = SHOTS[name]
    actions_path, script_path = work / f"{name}_actions.txt", work / f"{name}_script.txt"
    actions_path.write_text("\n".join(actions) + "\n", encoding="ascii")
    script_path.write_text("\n".join(script + [f"{frame + 1}  end"]) + "\n", encoding="ascii")
    result = subprocess.run([str(executable()), "--wide", "--actions", str(actions_path), "--script", str(script_path),
                             "--video-dump", str(frame), "--video-dir", str(work)], capture_output=True, text=True)
    dumped = work / f"frame_{frame}.bmp"
    if result.returncode != 0 or not dumped.exists():
        raise SystemExit(f"{name}: the run drew no frame {frame}\n{result.stderr[-2000:]}")
    with Image.open(dumped) as image:
        picture = window_picture(image.convert("RGB"))
    dumped.unlink()
    if check:
        with Image.open(OUT_DIR / f"{name}.png") as saved:
            return ImageChops.difference(picture, saved.convert("RGB")).getbbox() is None
    picture.save(OUT_DIR / f"{name}.png", optimize=True)
    return True


def main():
    arguments = sys.argv[1:]
    check = "--check" in arguments
    names = [name for name in arguments if name != "--check"] or list(SHOTS)
    for name in names:
        if name not in SHOTS:
            raise SystemExit(f"{name}: no such shot ({', '.join(SHOTS)})")
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    differ = []
    with tempfile.TemporaryDirectory() as folder:
        for name in names:
            same = shoot(name, Path(folder), check)
            print(f"docs/screenshots/{name}.png  {SHOTS[name][3]}" + ("" if not check else "  same" if same else "  DIFFERENT"))
            if not same:
                differ.append(name)
    if differ:
        raise SystemExit(f"different from docs/screenshots: {', '.join(differ)}")


if __name__ == "__main__":
    main()

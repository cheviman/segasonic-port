"""
Name:        make_font.py
Description: draws the font the first run's window writes with (ports/sdl/font/font.bmp): Kenney Mini
             (Kenney Fonts, CC0), the characters 20h-7Eh in a grid
Version:     0.1.0
Created:     20-10-2026
Modified:    20-10-2026
Author:      Sebastian Toyos

Usage: python tools/make_font.py FONT.ttf

The window the game opens before it has its assets has none of the game's own letters (they are in the
ROM). It writes with Kenney Mini at 16 pixels, its own size (white or black, no grey): a 16 x 16 cell a
character, 16 a row, from 20h (space) in the order of their codes, white on black. The window measures a
character by its lit columns (ports/sdl/first_run.c).
"""
import shutil
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "ports" / "sdl" / "font"
SIZE = 16          # Kenney Mini's pixels: at 16 each of its dots is one
CELL = 16
TOP = 4            # (no character draws above its row 4 at this size)
FIRST, LAST, ROW = 0x20, 0x7E, 16


def main():
    if len(sys.argv) != 2:
        raise SystemExit("Usage: python tools/make_font.py FONT.ttf")
    font = ImageFont.truetype(sys.argv[1], SIZE)
    count = LAST - FIRST + 1
    sheet = Image.new("L", (CELL * ROW, CELL * ((count + ROW - 1) // ROW)), 0)
    draw = ImageDraw.Draw(sheet)
    for index in range(count):
        x, y = (index % ROW) * CELL, (index // ROW) * CELL
        draw.text((x, y - TOP), chr(FIRST + index), font=font, fill=255)
    sheet = sheet.point(lambda value: 255 if value >= 128 else 0).convert("RGB")
    OUT.mkdir(parents=True, exist_ok=True)
    sheet.save(OUT / "font.bmp")
    licence = Path(sys.argv[1]).resolve().parent.parent / "License.txt"
    if licence.exists():
        shutil.copyfile(licence, OUT / "License.txt")
    print(f"  {OUT / 'font.bmp'} ({sheet.size[0]} x {sheet.size[1]})")
    return 0


if __name__ == "__main__":
    sys.exit(main())

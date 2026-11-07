"""
Name:        make_icon.py
Description: draws the executables' icon (ports/sdl2/icon/): a trackball, the cabinet's controller, with
             speed lines on a blue plate; nothing of the game's
Version:     0.1.0
Created:     23-10-2026
Modified:    07-11-2026
Author:      Sebastian Toyos

Usage: python tools/make_icon.py

The icon of SegaSonic.exe and SegaSonic Arcade.exe (segasonic.ico, in the resources CMakeLists.txt writes
from ports/sdl2/icon/version.rc.in), of their .apps on macOS (segasonic.icns) and of the Linux package (segasonic.png, 1024 x 1024, for a launcher of the
desktop's), of the Switch's .nro (ports/switch/icon.jpg, 256 x 256: the plate to the edges, which the
HOME menu rounds itself) and of the Android app (ports/android/res/mipmap-xxxhdpi: an adaptive icon's two
layers, the plate to the edges and the trackball in the middle 66 of its 108, which the launcher shapes and
moves itself; the icon whole for the launchers before Android 8; and the picture the APK shows in a player's
own picture's place until one is put there, ic_launcher_art.png, the plate to the edges). The game's own pictures are in the ROM, which no file of the project carries: SegaSonic's
window, once the game has its assets, shows the characters' heads (console_window_icon); SDL gives
SegaSonic Arcade's the executable's first icon. Drawn at twice the size and scaled down, so its edges are
smooth.
"""
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "ports" / "sdl2" / "icon"
SWITCH = ROOT / "ports" / "switch" / "icon.jpg"
ANDROID = ROOT / "ports" / "android" / "res" / "mipmap-xxxhdpi"
ANDROID_ICON = 192              # (the launcher's icon at xxxhdpi: 48 dp)
ANDROID_LAYER = 432             # (an adaptive icon's layer at xxxhdpi: 108 dp)
ANDROID_SAFE = 66 / 108         # (the part of a layer every launcher's shape shows)
ANDROID_ART = 288               # (a picture in what the launcher shows of a layer at xxxhdpi: 72 dp)
SIZE = 1024
SCALE = 2                       # (drawn at 2048, then scaled down)
ICO_SIZES = [16, 24, 32, 48, 64, 128, 256]

PLATE_TOP = np.array([0x22, 0x4C, 0xC0])     # the plate: blue, darker at the bottom
PLATE_BOTTOM = np.array([0x0A, 0x17, 0x48])
BALL = np.array([0xE2, 0x22, 0x2A])          # the ball: red
STEEL_LIGHT = np.array([0xF2, 0xF4, 0xF8])   # the ring around it
STEEL_DARK = np.array([0x5A, 0x63, 0x78])
LINE = np.array([0xFF, 0xD6, 0x3A])          # the speed lines: yellow
LIGHT = np.array([-0.45, -0.6, 0.66])        # from the top left, towards the viewer


def coverage(distance, pixel):
    """How much of a pixel a shape covers, from its signed distance to the edge (negative inside)."""
    return np.clip(0.5 - distance / pixel, 0.0, 1.0)


def rounded_square(x, y, left, top, right, bottom, radius):
    """Signed distance to a square with rounded corners."""
    cx, cy = (left + right) / 2, (top + bottom) / 2
    qx = np.abs(x - cx) - ((right - left) / 2 - radius)
    qy = np.abs(y - cy) - ((bottom - top) / 2 - radius)
    outside = np.hypot(np.maximum(qx, 0), np.maximum(qy, 0))
    return outside + np.minimum(np.maximum(qx, qy), 0) - radius


def capsule(x, y, x0, x1, cy, radius):
    """Signed distance to a horizontal bar with round ends, from x0 to x1."""
    px = np.clip(x, x0, x1)
    return np.hypot(x - px, y - cy) - radius


def over(image, color, alpha):
    """Paints 'color' over 'image' (RGBA, floats) where 'alpha' says."""
    alpha = alpha[..., None]
    image[..., :3] = image[..., :3] * (1 - alpha) + color * alpha
    image[..., 3:] = image[..., 3:] * (1 - alpha) + alpha


def draw(margin=48, radius=210, plate_shown=True, trackball_shown=True, trackball_scale=1.0):
    """The icon; 'margin' and 'radius' are the plate's room around it and its corners' (0: to the edges).
    Without the plate or without the trackball (its lines, its ring and its ball), the other alone on
    nothing (an adaptive icon's layers), the trackball 'trackball_scale' times its size about the middle."""
    size = SIZE * SCALE
    unit = size / SIZE
    y, x = np.mgrid[0:size, 0:size].astype(np.float64) + 0.5
    x, y = x / unit, y / unit           # (in the 1024 icon's pixels)
    pixel = 1.0 / unit
    image = np.zeros((size, size, 4))

    # the plate
    plate = coverage(rounded_square(x, y, margin, margin, SIZE - margin, SIZE - margin, max(radius, 1)), pixel)
    shade = (y - margin) / (SIZE - 2 * margin)
    color = PLATE_TOP * (1 - shade[..., None]) + PLATE_BOTTOM * shade[..., None]
    sheen = np.clip(1 - np.hypot(x - 300, y - 180) / 700, 0, 1) ** 2 * 0.25    # (a soft light, top left)
    color = color + (255 - color) * sheen[..., None]
    if plate_shown:
        over(image, color, plate)
    else:
        plate = np.ones_like(plate)     # (the lines on nothing: whole)
    if not trackball_shown:
        image = Image.fromarray(np.clip(image * [1, 1, 1, 255], 0, 255).round().astype(np.uint8), "RGBA")
        return image.resize((SIZE, SIZE), Image.LANCZOS)
    x = (x - SIZE / 2) / trackball_scale + SIZE / 2    # (the trackball's own pixels, about the middle)
    y = (y - SIZE / 2) / trackball_scale + SIZE / 2
    pixel = pixel / trackball_scale

    # the speed lines, behind the ball, fading to the left
    cx, cy, ball, ring = 600.0, 560.0, 232.0, 292.0
    for line_y, start, half in ((392, 170, 30), (540, 112, 30), (688, 190, 30)):
        end = cx - 150
        alpha = coverage(capsule(x, y, start, end, line_y, half), pixel)
        fade = np.clip((x - start + half) / (end - start + half), 0, 1) ** 0.55
        over(image, LINE, alpha * fade * plate)

    # the ring: steel, beveled (light at the top left, dark at the bottom right); a dark groove inside it
    distance = np.hypot(x - cx, y - cy)
    toward = ((x - cx) * -0.6 + (y - cy) * -0.8) / np.maximum(distance, 1)      # 1 facing the light
    outer = np.clip((toward + 1) / 2, 0, 1)[..., None]
    over(image, STEEL_DARK * (1 - outer) + STEEL_LIGHT * outer, coverage(distance - ring, pixel))
    inner = np.clip((1 - toward) / 2, 0, 1)[..., None]                         # (the bevel's inside)
    over(image, STEEL_DARK * (1 - inner) + STEEL_LIGHT * inner, coverage(distance - (ring - 22), pixel))
    over(image, np.array([0x0C, 0x0E, 0x16]), coverage(distance - (ball + 12), pixel))

    # the ball: lit from the top left, with a highlight, mottled as a trackball's is, and in the ring's
    # shadow at its top edge (it sits below the ring)
    nx, ny = (x - cx) / ball, (y - cy) / ball
    nz = np.sqrt(np.clip(1 - nx * nx - ny * ny, 0, 1))
    light = LIGHT / np.linalg.norm(LIGHT)
    diffuse = np.clip(nx * light[0] + ny * light[1] + nz * light[2], 0, 1)
    half_way = (light + np.array([0, 0, 1])) / np.linalg.norm(light + np.array([0, 0, 1]))
    facing = np.clip(nx * half_way[0] + ny * half_way[1] + nz * half_way[2], 0, 1)
    mottle = (np.sin(9 * nx + 4 * ny) + np.sin(7 * ny - 5 * nz + 1) + np.sin(8 * nz + 6 * nx + 2)) / 3
    base = BALL * (1 + 0.10 * mottle[..., None])
    color = base * (0.16 + 0.84 * diffuse[..., None])
    color = color + BALL * 0.35 * (np.clip(ny, 0, 1) ** 3 * (1 - nz))[..., None]   # (light from below)
    color = color + (255 - color) * (facing ** 90 * 0.95 + facing ** 12 * 0.18)[..., None]
    edge = np.clip(1 - (ball - distance) / 46, 0, 1) * np.clip(0.3 - ny, 0, 1)
    color = color * (1 - 0.55 * edge)[..., None]
    over(image, np.clip(color, 0, 255), coverage(distance - ball, pixel))

    image = Image.fromarray(np.clip(image * [1, 1, 1, 255], 0, 255).round().astype(np.uint8), "RGBA")
    return image.resize((SIZE, SIZE), Image.LANCZOS)


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    icon = draw()
    icon.save(OUT / "segasonic.png")
    icon.save(OUT / "segasonic.ico", sizes=[(side, side) for side in ICO_SIZES])
    icon.save(OUT / "segasonic.icns")
    print(f"written: {', '.join(name for name in ('segasonic.png', 'segasonic.ico', 'segasonic.icns'))} "
          f"in {OUT.relative_to(ROOT)}")
    SWITCH.parent.mkdir(parents=True, exist_ok=True)
    draw(margin=0, radius=0).convert("RGB").resize((256, 256), Image.LANCZOS).save(SWITCH, quality=95)
    print(f"written: {SWITCH.relative_to(ROOT)}")
    ANDROID.mkdir(parents=True, exist_ok=True)
    icon.resize((ANDROID_ICON, ANDROID_ICON), Image.LANCZOS).save(ANDROID / "ic_launcher.png")
    background = draw(margin=0, radius=0, trackball_shown=False).convert("RGB")
    background.resize((ANDROID_LAYER, ANDROID_LAYER), Image.LANCZOS).save(ANDROID / "ic_launcher_background.png")
    foreground = draw(plate_shown=False, trackball_scale=ANDROID_SAFE)
    foreground.resize((ANDROID_LAYER, ANDROID_LAYER), Image.LANCZOS).save(ANDROID / "ic_launcher_foreground.png")
    art = draw(margin=0, radius=0).convert("RGB")
    art.resize((ANDROID_ART, ANDROID_ART), Image.LANCZOS).save(ANDROID / "ic_launcher_art.png")
    print(f"written: ic_launcher.png, ic_launcher_background.png, ic_launcher_foreground.png, ic_launcher_art.png "
          f"in {ANDROID.relative_to(ROOT)}")


if __name__ == "__main__":
    main()

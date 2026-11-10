#!/usr/bin/env python3
"""
Name:        extract_assets.py
Description: extracts the game's assets from the user's own ROM, driven by assets.json (as in n64decomp/sm64)
Version:     0.1.0
Created:     11-10-2026
Modified:    20-10-2026
Author:      Sebastian Toyos

Usage:
  python extract_assets.py [rom folder or .zip]   extract what is missing (default ROM location: rom/)
  python extract_assets.py --force [rom]          extract everything again
  python extract_assets.py --clean                delete every extracted asset
  python extract_assets.py --check [rom]          build back what can go into the ROM and compare the bytes
"""
import json
import shutil
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / "tools"))
import asset_formats  # noqa: E402
import data_formats  # noqa: E402
import sonic_rom  # noqa: E402

MANIFEST = ROOT / "assets.json"
ASSETS = ROOT / "assets"
STATE = ASSETS / ".extracted"     # manifest version, then the path of every asset extracted

REGION_BUILDERS = {
    "program": sonic_rom.build_program,
    "tiles": sonic_rom.build_tiles,
    "sprites": sonic_rom.build_sprites,
    "sound": sonic_rom.build_sound,
}


def inside_assets(relative_path):
    """Whether the path stays inside assets/: no '..', no absolute path, no link that leads out of it."""
    return ASSETS.resolve() in (ROOT / relative_path).resolve().parents


def load_manifest():
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    for asset in manifest["assets"]:
        if not inside_assets(asset["path"]):
            raise SystemExit(f"assets.json: {asset['path']} is outside assets/")
        asset["offset"] = int(asset["offset"], 0)
        asset["size"] = int(asset["size"], 0)
        if asset["region"] not in REGION_BUILDERS:
            raise SystemExit(f"assets.json: unknown region '{asset['region']}' for {asset['path']}")
    return manifest


def sprite_palette_colors(program, offset, count):
    """'count' palette entries at 'offset' of the program ROM, as the game keeps them (the palette window's
    alternate format: the low bit of blue, green and red in bits 14-12), as (red, green, blue)."""
    colors = []
    for index in range(count):
        value = int.from_bytes(program[offset + index * 2:offset + index * 2 + 2], "little")
        red = ((value >> 12) & 1) | ((value << 1) & 0x1E)
        green = ((value >> 13) & 1) | ((value >> 3) & 0x1E)
        blue = ((value >> 14) & 1) | ((value >> 7) & 0x1E)
        colors.append(asset_formats.rgb555(blue << 10 | green << 5 | red))
    return colors


SHEET_NOTES = ["Each frame goes back into the sprite ROM at 'word' (32-bit words, bank << 20 | address), cut from "
               "the sheet at x, y (width, height in pixels); 'frame' is its number in the game (the offset of its "
               "record in sprite_frame_records). 'rows' are the sheet's animations, by the index of their first frame.",
               "4-bit sheets: up to 8 palettes, 32 entries each ('palettes', by slot): a pixel is slot * 32 + its pen "
               "where the board draws it, slot * 32 + 16 + its pen where it does not, shown see-through: pen 0 and, "
               "unless the frame is 'opaque' (the game draws it so: the end code is a color), the end code 15 at a "
               "word's edges and what follows a word that ends with it. 8-bit sheets: the pixel is the pen; pen 0 "
               "is see-through, and so is pen 255 where the sheet's frames are not opaque (it is their end code, "
               "which the board does not draw).",
               "'palettes' are where the colours are in the program ROM; a frame's 'palette_from' says how its "
               "palette was found: 'drawn' in the scenarios, 'nearby' (a frame no scenario draws, given its "
               "neighbour's) or 'chosen' by hand (sprite_palettes.json)."]


def extract_sprites(asset, sprites, program, path):
    """The frames of the sprite ROM as sheets (sprite_sheets.txt): a PNG per animation family or object, in
    folders by subject, each with a JSON beside it that says where each frame is in it and where it goes back
    in the sprite ROM; what no frame covers as small sheets of their own."""
    frames = asset_formats.sprite_frames(program, int(asset["records"], 0), asset["count"])
    by_number = {frame["number"]: frame for frame in frames}
    known = json.loads((ROOT / asset["palettes"]).read_text(encoding="ascii"))
    palettes = {int(number, 16): (int(offset, 0), source)
                for source in ("drawn", "nearby", "chosen") for number, offset in known.get(source, {}).items()}
    opaque = {int(number, 16) for numbers in known.get("opaque", {}).values() for number in numbers}
    sheets, where = asset_formats.load_sprite_sheets(ROOT / asset["sheets"], list(by_number))
    left_out = [number for number in by_number if number not in where]
    if left_out:
        raise SystemExit(f"{asset['sheets']}: frames in no sheet: "
                         + ", ".join(f"{number:04X}" for number in left_out[:12]) + (" ..." if len(left_out) > 12 else ""))
    files = 0
    covered = bytearray(len(sprites) // 4)
    for sheet_path, sheet in sheets.items():
        members = [by_number[number] for number in sheet["frames"]]
        bits = {frame["bits"] for frame in members}
        if len(bits) != 1:
            raise SystemExit(f"{asset['sheets']}: {sheet_path} mixes 4-bit and 8-bit frames")
        bits = bits.pop()
        slots = list(dict.fromkeys(palettes.get(frame["number"], (None, "none"))[0] for frame in members))
        if len(slots) > (SHEET_SLOTS_4 if bits == 4 else 1):
            raise SystemExit(f"{asset['sheets']}: {sheet_path} has {len(slots)} palettes, more than a "
                             f"{bits}-bit sheet holds")
        pens = {frame["number"]: asset_formats.sprite_pens(sprites, frame) for frame in members}
        sizes = {number: (len(rows[0]) if rows else 0, len(rows)) for number, rows in pens.items()}
        placed, (width, height) = asset_formats.sheet_layout(sheet["rows"], sizes)
        see_through = asset_formats.NOT_DRAWN if bits == 4 else 0
        canvas = [[see_through] * width for _ in range(height)]
        listing = []
        end_code_drawn = set()      # an 8-bit sheet's pen 255: whether the board draws it
        for frame in members:
            number = frame["number"]
            offset, source = palettes.get(number, (None, "none"))
            slot = slots.index(offset)
            x, y = placed[number]
            rows = pens[number]
            drawn = asset_formats.sprite_drawn(rows, bits, number in opaque)
            if bits == 8:
                end_code_drawn |= {shown for row, flags in zip(rows, drawn)
                                   for pen, shown in zip(row, flags) if pen == 255}
            for line, row in enumerate(rows):
                if bits == 4:
                    canvas[y + line][x:x + len(row)] = [slot * 32 + pen + (0 if shown else asset_formats.NOT_DRAWN)
                                                        for pen, shown in zip(row, drawn[line])]
                else:
                    canvas[y + line][x:x + len(row)] = row
            listing.append({"index": len(listing), "frame": f"{number:04X}", "x": x, "y": y,
                            "width": sizes[number][0], "height": sizes[number][1], "word": f"{frame['word']:06X}",
                            "slot": slot, "palette_from": source, "opaque": number in opaque})
            covered[frame["word"]:frame["word"] + frame["words"]] = b"\x01" * frame["words"]
        if bits == 4:
            colors, alphas = [], []
            for offset in slots:
                shades = sprite_palette_colors(program, offset, 16) if offset is not None \
                    else [(level * 17,) * 3 for level in range(16)]
                colors += shades + shades
                alphas += [255] * 16 + [0] * 16
            options = {"alphas": alphas}
        else:
            colors = sprite_palette_colors(program, slots[0], 256) if slots[0] is not None \
                else [(level,) * 3 for level in range(256)]
            if len(end_code_drawn) > 1:
                raise SystemExit(f"{asset['sheets']}: {sheet_path} has pens 255 the board draws and pens 255 it "
                                 "does not, which an 8-bit sheet cannot show apart")
            hidden = (0,) if True in end_code_drawn else (0, 255)
            options = {"alphas": [0 if index in hidden else 255 for index in range(256)]}
        target = path / f"{sheet_path}.png"
        target.parent.mkdir(parents=True, exist_ok=True)
        asset_formats.write_png_indexed(target, canvas, colors, 8, **options)
        firsts, index = [], 0
        for name, numbers in sheet["rows"]:
            firsts.append({"row": name, "first": index, "frames": len(numbers)})
            index += len(numbers)
        target.with_suffix(".json").write_text(json.dumps({
            "sheet": sheet_path, "description": sheet["description"], "notes": SHEET_NOTES, "bits": bits,
            "palettes": [f"0x{offset:06X}" if offset is not None else None for offset in slots],
            "rows": firsts, "frames": listing}, indent=1) + "\n", encoding="ascii")
        files += 2
    # what no frame covers and is not empty (0xFF): sheets of 8-pixel-wide 4-bit strips in unused/
    word = 0
    while word < len(covered):
        if covered[word] or sprites[word * 4:word * 4 + 4] == b"\xff\xff\xff\xff":
            word += 1
            continue
        start = word
        while word < len(covered) and not covered[word] and sprites[word * 4:word * 4 + 4] != b"\xff\xff\xff\xff":
            word += 1
        strip = {"number": None, "width": 8, "height": word - start, "word": start, "bits": 4}
        rows = asset_formats.sprite_pens(sprites, strip)
        empty = all(row == [15, 0, 0, 0, 0, 0, 0, 15] for row in rows)
        name = f"unused/{'blank' if empty else 'graphics'}_strip_{start:06X}"
        target = path / f"{name}.png"
        target.parent.mkdir(parents=True, exist_ok=True)
        asset_formats.write_png_indexed(target, rows, [(level * 17,) * 3 for level in range(16)], 8)
        target.with_suffix(".json").write_text(json.dumps({
            "sheet": name, "description": ("rows of nothing (the end code at both edges, pen 0 between) that no "
                                           "frame record points at" if empty else "graphics no frame record points at"),
            "notes": ["Pens of the sprite ROM no frame covers, 8 a row: they go back at 'word' (32-bit words)."],
            "bits": 4, "palettes": [None], "rows": [{"row": "strip", "first": 0, "frames": 1}],
            "frames": [{"index": 0, "frame": None, "x": 0, "y": 0, "width": 8, "height": word - start,
                        "word": f"{start:06X}", "slot": 0, "palette_from": "none"}]}, indent=1) + "\n", encoding="ascii")
        files += 2
    return files


SHEET_SLOTS_4 = asset_formats.SHEET_SLOTS


TILE_SLOTS = 16    # palettes a tile sheet's PNG holds: 16 of 16 colors


def extract_tile_sheet(asset, data, program, path, columns):
    """A sheet of 16x16 tiles as an 8-bit indexed PNG in color: the 16 palettes its tiles are shown with
    the most (tile_palettes.json), 16 colors each, a tile's pixels at its palette's slot * 16 + pen (pen 0
    see-through), so that index & 15 is the pen; and NAME.json beside it, each tile's palette."""
    first = asset["offset"] // asset_formats.TILE_BYTES
    count = len(data) // asset_formats.TILE_BYTES
    known = json.loads((ROOT / asset["palettes"]).read_text(encoding="ascii"))
    palettes = {int(tile, 16): (int(offset, 0), source)
                for source in ("drawn", "nearby") for tile, offset in known[source].items()}
    used = Counter(palettes[first + tile][0] for tile in range(count) if first + tile in palettes)
    slots = [offset for offset, _ in used.most_common(TILE_SLOTS)]
    colors = []
    for offset in slots:
        colors += sprite_palette_colors(program, offset, 16)
    colors += [(level * 17,) * 3 for level in range(16)] * (TILE_SLOTS - len(slots))
    rows = asset_formats.tile_sheet_pens(data, columns)
    listing = []
    for tile in range(count):
        offset, source = palettes.get(first + tile, (None, "none"))
        slot = slots.index(offset) if offset in slots else None
        top = (tile // columns) * asset_formats.TILE_SIZE
        left = (tile % columns) * asset_formats.TILE_SIZE
        if slot:    # (slot 0: the pens are the indices already)
            for y in range(top, top + asset_formats.TILE_SIZE):
                rows[y][left:left + asset_formats.TILE_SIZE] = [slot * 16 + pen for pen in rows[y][left:left + asset_formats.TILE_SIZE]]
        listing.append({"tile": f"{first + tile:04X}", "palette": f"0x{offset:06X}" if offset is not None else None,
                        "palette_from": source, "slot": slot})
    alphas = [0 if index % 16 == 0 else 255 for index in range(TILE_SLOTS * 16)]
    asset_formats.write_png_indexed(path, rows, colors, 8, alphas=alphas)
    path.with_suffix(".json").write_text(json.dumps({
        "notes": ["Each tile's palette: where its colors are in the program ROM ('palette_from': 'drawn' in the "
                  "scenarios, or 'nearby': a tile no scenario shows, given its neighbour's), and the slot of the "
                  "sheet's 16 palettes it is drawn with (null: not among them, drawn with the first). Index & 15 "
                  "of a pixel is the pen."],
        "tiles": listing}, indent=1) + "\n", encoding="ascii")
    return 2


def extract_samples(asset, data, sound, path):
    """Every sample of the PCM area as an 8-bit WAV (sample_NN.wav, NN its number in pcm_sample_headers),
    and the runs of data no header points at (unused_OFFSET.wav), one byte of the ROM a WAV sample; and
    samples.json: where each goes back."""
    starts = asset_formats.pcm_sample_starts(sound, int(asset["headers"], 0), asset["count"])
    starts = {offset - asset["offset"]: numbers for offset, numbers in starts.items()}
    path.mkdir(parents=True, exist_ok=True)
    index = {"notes": ["Each WAV goes back into the sound region at 'offset', one byte a WAV sample: +m is 128 + m "
                       "and -m is 127 - m (the RF5C68's sign and magnitude), 255 is never used. A sample ends at "
                       "the end byte (FFh) the build puts after it; it can grow up to the next piece's offset.",
                       "'samples' are its numbers in pcm_sample_headers; an unused piece is data no header points at.",
                       "The rate is nominal: the driver sets the pitch of every note."],
             "pieces": []}
    for offset, sample, numbers in asset_formats.pcm_pieces(data, starts):
        name = f"sample_{numbers[0]:02X}.wav" if numbers else f"unused_{asset['offset'] + offset:06X}.wav"
        asset_formats.write_wav(path / name, asset_formats.rf5c68_to_unsigned(sample), asset["rate"], bits=8)
        index["pieces"].append({"file": name, "offset": f"{asset['offset'] + offset:06X}",
                                "samples": [f"{number:02X}" for number in numbers]})
    (path / "samples.json").write_text(json.dumps(index, indent=1) + "\n", encoding="ascii")
    return len(index["pieces"]) + 1


def build_samples(asset):
    """The PCM area back from its WAVs: each at its offset, the end byte everywhere else."""
    folder = ROOT / asset["path"]
    pieces = json.loads((folder / "samples.json").read_text(encoding="ascii"))["pieces"]
    out = bytearray([asset_formats.PCM_END]) * asset["size"]
    for index, piece in enumerate(pieces):
        start = int(piece["offset"], 16) - asset["offset"]
        limit = int(pieces[index + 1]["offset"], 16) - asset["offset"] if index + 1 < len(pieces) else asset["size"]
        samples, _ = asset_formats.read_wav_unsigned(folder / piece["file"])
        data = asset_formats.unsigned_to_rf5c68(samples)
        if start + len(data) > limit:
            raise ValueError(f"{piece['file']}: {len(data)} bytes, more than the {limit - start} up to the next piece")
        out[start:start + len(data)] = data
    return bytes(out)


def read_as_data(asset):
    """[(start, end, name)] of the code the game also reads as data (the asset's 'read_as_data'): pieces of
    their own, so the assets hold them though the C replaces the code."""
    return [(int(read["address"], 0), int(read["address"], 0) + read["size"], read["name"])
            for read in asset.get("read_as_data", [])]


def without(spans, holes):
    """The (start, end) spans less the (start, end, name) holes."""
    for low, high, _ in holes:
        spans = [part for start, end in spans for part in ((start, min(end, low)), (max(start, high), end))
                 if part[0] < part[1]]
    return spans


def extract(asset, data, region_of):
    path = ROOT / asset["path"]
    kind = asset["type"]
    if kind == "sprite_frames":
        return extract_sprites(asset, data, region_of("program"), path)
    if kind == "program_data":
        start, end = asset["offset"], asset["offset"] + asset["size"]
        reads = read_as_data(asset)
        places = data_formats.pieces(ROOT / "symbols.txt", start, end) + [(low, name) for low, _, name in reads]
        code = without(data_formats.v60_code(region_of("program"), ROOT / "symbols.txt", start, end), reads)
        # the chips' mirrors are no data of their own: kept, as the code is
        mirrors = [tuple(int(value, 0) for value in pair) for pair in asset.get("mirrors", [])]
        return data_formats.export(data, start, places, ROOT / asset["formats"], path, sorted(code + mirrors))
    if kind == "sound_data":
        symbols = ROOT / asset["symbols"]
        movable = tuple(int(value, 0) for value in asset["movable"]) if "movable" in asset else None
        return data_formats.export(data, asset["offset"], data_formats.sound_pieces(symbols, asset["size"]),
                                   ROOT / asset["formats"], path, data_formats.z80_code(data, symbols), movable)
    if kind == "pcm_samples":
        return extract_samples(asset, data, region_of("sound"), path)
    if kind == "tiles_16x16x4":
        path.parent.mkdir(parents=True, exist_ok=True)
        columns = asset["columns"]
        if "palettes" not in asset:
            asset_formats.write_png_4bpp(path, columns * asset_formats.TILE_SIZE,
                                         asset_formats.tile_sheet_rows(data, columns))
            return 1
        return extract_tile_sheet(asset, data, region_of("program"), path, columns)
    raise SystemExit(f"assets.json: unknown type '{kind}' for {asset['path']}")


def remove(relative_path):
    if not inside_assets(relative_path):
        print(f"  {relative_path}: outside assets/, left alone")
        return
    path = ROOT / relative_path
    if path.is_dir():
        shutil.rmtree(path)
    elif path.exists():
        path.unlink()
    # Drop the folders the asset leaves empty, up to assets/
    folder = path.parent
    while folder != ASSETS and ASSETS in folder.parents and folder.is_dir() and not any(folder.iterdir()):
        folder.rmdir()
        folder = folder.parent


def read_state():
    """Manifest version and asset paths of the last extraction"""
    if not STATE.exists():
        return None, []
    lines = STATE.read_text(encoding="utf-8").splitlines()
    return (lines[0].strip(), [line for line in lines[1:] if line]) if lines else (None, [])


def write_state(version, paths):
    STATE.parent.mkdir(parents=True, exist_ok=True)
    STATE.write_text("\n".join([version] + paths) + "\n", encoding="utf-8")


# each takes the asset and the bytes the ROM has where it goes (what the asset leaves out, the code, comes
# from there)
REBUILDERS = {
    "sprite_frames": lambda asset, original: asset_formats.build_sprite_rom(ROOT / asset["path"], asset["size"]),
    "program_data": lambda asset, original: data_formats.build(ROOT / asset["path"], asset["size"], original),
    "sound_data": lambda asset, original: data_formats.build(ROOT / asset["path"], asset["size"], original),
    "pcm_samples": lambda asset, original: build_samples(asset),
    "tiles_16x16x4": lambda asset, original: asset_formats.tiles_from_sheet(
        asset_formats.read_png_indexed(ROOT / asset["path"])[0], asset["columns"],
        asset["size"] // asset_formats.TILE_BYTES),
}


def assets_ready(manifest):
    """Whether assets/ holds this manifest's extraction, every asset of it."""
    version, _ = read_state()
    return version == str(manifest["manifest_version"]) and all((ROOT / asset["path"]).exists()
                                                                 for asset in manifest["assets"])


def regions_from_assets(rom, manifest):
    """The board's regions with every asset built from its files over the ROM's bytes (the ROM's stay
    where no asset goes, and for what an asset leaves out): {region name: bytes}."""
    regions = {name: bytearray(builder(rom)) for name, builder in REGION_BUILDERS.items()}
    for asset in manifest["assets"]:
        start, end = asset["offset"], asset["offset"] + asset["size"]
        region = regions[asset["region"]]
        region[start:end] = REBUILDERS[asset["type"]](asset, bytes(region[start:end]))
    return {name: bytes(region) for name, region in regions.items()}


def check(manifest, rom):
    """Builds every asset that can go back into the ROM from its extracted files and compares the bytes;
    then the whole set, chip by chip, as the game's build makes it from the assets."""
    regions = {}
    failed = 0
    for asset in manifest["assets"]:
        rebuild = REBUILDERS.get(asset["type"])
        if rebuild is None:
            continue
        if asset["region"] not in regions:
            regions[asset["region"]] = REGION_BUILDERS[asset["region"]](rom)
        original = regions[asset["region"]][asset["offset"]:asset["offset"] + asset["size"]]
        built = rebuild(asset, original)
        if built == original:
            print(f"  {asset['path']}: identical ({len(built)} bytes)")
            continue
        failed += 1
        first = next(index for index, (a, b) in enumerate(zip(built, original)) if a != b)
        print(f"  {asset['path']}: differs from byte {first:#x} of the region")
    chips = sonic_rom.chips_from_regions(regions_from_assets(rom, manifest))
    differ = sorted(name for name in rom if chips[name] != rom[name])
    print(f"  the ROM set from the assets: {len(rom) - len(differ)} of {len(rom)} chips identical"
          + (f"; differ: {', '.join(differ)}" if differ else ""))
    return 1 if failed or differ else 0


def main():
    arguments = sys.argv[1:]
    manifest = load_manifest()
    listed = [asset["path"] for asset in manifest["assets"]]
    saved_version, saved_paths = read_state()
    if "--clean" in arguments:
        for path in dict.fromkeys(listed + saved_paths):
            remove(path)
        STATE.unlink(missing_ok=True)
        print("Extracted assets removed")
        return 0

    force = "--force" in arguments
    checking = "--check" in arguments
    arguments = [argument for argument in arguments if argument not in ("--force", "--check")]
    source = Path(arguments[0]) if arguments else ROOT / "rom"
    if checking:
        return check(manifest, sonic_rom.load_set(source))
    version = str(manifest["manifest_version"])
    up_to_date = saved_version == version
    # Assets an earlier manifest listed and this one no longer does
    stale = [path for path in saved_paths if path not in listed]
    for path in stale:
        remove(path)
        print(f"  removed {path}")
    todo = [asset for asset in manifest["assets"]
            if force or not up_to_date or not (ROOT / asset["path"]).exists()]
    if not todo:
        if stale:
            write_state(version, listed)
        print("Assets are up to date")
        return 0

    rom = sonic_rom.load_set(source)
    regions = {}

    def region_of(name):
        if name not in regions:
            regions[name] = REGION_BUILDERS[name](rom)
        return regions[name]
    files = 0
    for asset in todo:
        remove(asset["path"])
        data = region_of(asset["region"])[asset["offset"]:asset["offset"] + asset["size"]]
        files += extract(asset, data, region_of)
        print(f"  {asset['path']}")
    write_state(version, listed)
    print(f"{len(todo)} assets extracted ({files} files)")
    return 0


if __name__ == "__main__":
    sys.exit(main())

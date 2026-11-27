"""
Name:        check_sdl.py
Description: the SDL functions a handheld's executable asks its system's SDL 2 for, none newer than that SDL
Version:     0.1.0
Created:     27-11-2026
Modified:    27-11-2026
Author:      Sebastian Toyos

license:BSD-3-Clause
Usage: python ports/anbernic/check_sdl.py --headers SDL2_INCLUDE [--most 2.0.12] EXECUTABLE [EXECUTABLE ...]

The handheld's games run with the SDL 2 its firmware has (Anbernic's 2.0.12): a function newer than it would stop the
game as it starts, on the handheld alone. Each EXECUTABLE (an ELF) names the functions it takes from its libraries
(its dynamic symbols with no section); SDL 2's headers say since which version each one is there ("\\since This
function is available since SDL 2.0.x"). Any SDL function newer than --most, or one the headers do not know, is said
and the check fails.
"""
import re
import struct
import sys
from pathlib import Path

SINCE = re.compile(r"\\since This (?:function|macro) is available since SDL (\d+)\.(\d+)\.(\d+)")
DECLARED = re.compile(r"extern\s+DECLSPEC[^;(]*?\bSDLCALL\s+(SDL_\w+)\s*\(", re.S)


def functions_since(headers):
    """Each function SDL 2's headers declare, with the version its comment says it is there since."""
    since = {}
    for header in sorted(Path(headers).glob("SDL*.h")):
        text = header.read_text(encoding="utf-8", errors="replace")
        for declared in DECLARED.finditer(text):
            comment_start = text.rfind("/**", 0, declared.start())
            comment_end = text.find("*/", comment_start)
            version = None
            if comment_start >= 0 and comment_end < declared.start():
                # (the comment right before the declaration: nothing but spaces between them)
                if not text[comment_end + 2:declared.start()].strip():
                    found = SINCE.search(text, comment_start, comment_end)
                    if found:
                        version = tuple(int(part) for part in found.groups())
            since.setdefault(declared.group(1), version)
    return since


def undefined_symbols(path):
    """The names an ELF's dynamic symbol table has with no section: what it takes from its libraries."""
    data = Path(path).read_bytes()
    if data[:4] != b"\x7fELF":
        raise ValueError(f"{path}: not an ELF")
    wide = data[4] == 2
    order = "<" if data[5] == 1 else ">"
    if wide:
        section_offset, = struct.unpack_from(order + "Q", data, 0x28)
        entry_size, count = struct.unpack_from(order + "HH", data, 0x3A)
    else:
        section_offset, = struct.unpack_from(order + "I", data, 0x20)
        entry_size, count = struct.unpack_from(order + "HH", data, 0x2E)
    sections = []
    for index in range(count):
        at = section_offset + index * entry_size
        if wide:
            _, kind, _, _, offset, size, link, _, _, each = struct.unpack_from(order + "IIQQQQIIQQ", data, at)
        else:
            _, kind, _, _, offset, size, link, _, _, each = struct.unpack_from(order + "IIIIIIIIII", data, at)
        sections.append((kind, offset, size, link, each))
    names = set()
    for kind, offset, size, link, each in sections:
        if kind != 11 or not each:    # SHT_DYNSYM
            continue
        strings = sections[link][1]
        for at in range(offset, offset + size, each):
            if wide:
                name, _, _, section = struct.unpack_from(order + "IBBH", data, at)
            else:
                name, _, _, _, _, section = struct.unpack_from(order + "IIIBBH", data, at)
            if section == 0 and name:
                end = data.index(b"\0", strings + name)
                names.add(data[strings + name:end].decode("ascii", "replace"))
    return names


def main():
    arguments = sys.argv[1:]
    headers, most, executables = None, (2, 0, 12), []
    while arguments:
        argument = arguments.pop(0)
        if argument == "--headers" and arguments:
            headers = arguments.pop(0)
        elif argument == "--most" and arguments:
            most = tuple(int(part) for part in arguments.pop(0).split("."))
        else:
            executables.append(argument)
    if not headers or not executables:
        print(__doc__.strip())
        return 2
    since = functions_since(headers)
    if not since:
        print(f"No SDL 2 functions found in {headers}")
        return 2
    failed = False
    for executable in executables:
        asked = sorted(name for name in undefined_symbols(executable) if name.startswith("SDL_"))
        newer = [(name, since[name]) for name in asked if name in since and since[name] and since[name] > most]
        unknown = [name for name in asked if name not in since]
        newest = max((since[name] for name in asked if since.get(name)), default=None)
        print(f"{executable}: {len(asked)} SDL functions, the newest of SDL "
              f"{'.'.join(str(part) for part in newest) if newest else '?'}")
        for name, version in newer:
            print(f"  {name}: since SDL {'.'.join(str(part) for part in version)}, newer than "
                  f"{'.'.join(str(part) for part in most)}")
        for name in unknown:
            print(f"  {name}: not in SDL 2's headers")
        failed = failed or bool(newer) or bool(unknown)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())

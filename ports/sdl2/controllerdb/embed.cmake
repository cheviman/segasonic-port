# Name:        embed.cmake
# Description: SDL_GameControllerDB's mappings of one platform as a C array, to build into the games
# Version:     0.1.0
# Created:     07-11-2026
# Modified:    07-11-2026
# Author:      Sebastian Toyos
#
# Usage: cmake -DDB=gamecontrollerdb.txt -DPLATFORM=Windows -DOUT=controller_db.c -P embed.cmake
# PLATFORM is SDL's name of the platform, as the mappings' "platform:" field says it (Windows, Mac OS X, Linux,
# Android). The array is the mappings' lines, a line each, and a 0 after them: main.c gives it to SDL as a hint
# before SDL starts. Bytes, not a string: MSVC takes no string literal longer than 64 KB (tmnt-port's, the same).

file(STRINGS "${DB}" lines REGEX "platform:${PLATFORM},")
list(LENGTH lines count)
list(JOIN lines "\n" text)
string(APPEND text "\n")
file(WRITE "${OUT}.txt" "${text}")
file(READ "${OUT}.txt" hex HEX)
file(REMOVE "${OUT}.txt")
string(REPEAT "[0-9a-f]" 48 row)    # (24 bytes a row: CMake's expressions have no {n})
string(REGEX REPLACE "(${row})" "\\1\n" hex "${hex}")
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," bytes "${hex}")
string(REPLACE "0x0d," "" bytes "${bytes}")    # (no carriage returns: SDL splits the mappings at line feeds)
string(REGEX MATCHALL "0x" each "${bytes}")
list(LENGTH each size)
string(REPLACE "\n" "\n\t" bytes "${bytes}")
file(WRITE "${OUT}" "/* SDL_GameControllerDB's ${count} mappings for ${PLATFORM} (ports/sdl2/controllerdb, zlib License),
   written by ports/sdl2/controllerdb/embed.cmake: the mappings' lines and a 0 */
#include <stddef.h>

const unsigned char segasonic_controller_db[] = {
\t${bytes}0x00
};
const size_t segasonic_controller_db_size = ${size};
")

# Name:        switch.cmake
# Description: the Nintendo Switch build (devkitPro's toolchain): the console game alone, as the pieces of its
#              .nsp (the NSO, main.npdm, the NACP and the icon) and, where the assets pack is, as SegaSonic.nro
# Version:     0.1.0
# Created:     23-10-2026
# Modified:    27-11-2026
# Author:      Sebastian Toyos
#
# Included by CMakeLists.txt when the toolchain is the Switch's (aarch64-none-elf-cmake). The game is the same
# console build as on a computer, with SDL 3 for the Switch (below: devkitPro's own branch of it), its picture and
# controls fixed (SEGASONIC_HANDHELD) and no EXIT in its menu (SEGASONIC_SYSTEM_EXIT: HOME leaves it). The Switch
# makes nothing from the ROM set: the game reads its assets pack from its own romfs, which a computer fills with
# the player's (the .nsp, made there with the player's keys; or, where the build has the pack beside the
# executables, build/bin/segasonic.assets, the .nro for the Homebrew Menu, the player's own and never shared).

target_compile_definitions(sonic_game_console PUBLIC SEGASONIC_HANDHELD SEGASONIC_SYSTEM_EXIT SEGASONIC_SCREEN_WIDE)

# SDL 3 for the Switch (Sebas, 19-11-2026: "Probar SDL3 en Switch"). SDL's own Switch port is for those under
# Nintendo's NDA (its docs/README-switch.md), and devkitPro has no package of SDL 3 yet (its portlibs have switch-sdl2
# alone): SDL is built here from devkitPro's own branch of it (github.com/devkitPro/SDL, switch-sdl-3.4: SDL 3.4.0 with
# libnx's video, sound and controllers; not released, its maintainer: "a couple of tests failing"), the commit below
# by its archive's hash (the one in build/downloads/sdl3-switch-archive where it is there), into the games. A package
# of devkitPro's (switch-sdl3) is taken instead once there is one
find_package(SDL3 CONFIG QUIET)
if(TARGET SDL3::SDL3)
	set(SONIC_SWITCH_SDL SDL3::SDL3)
else()
	find_package(PkgConfig REQUIRED)
	set(SONIC_SWITCH_SDL_COMMIT c329016c0e338ab4397ad930f83330c9fe058348)
	set(SONIC_SWITCH_SDL_SHA256 6047e4787f105ab4a0c6293ee3090ce5d1ebad112fcbe2a3e5a8874b0cc6f709)
	set(SONIC_SWITCH_SDL_URL https://github.com/devkitPro/SDL/archive/${SONIC_SWITCH_SDL_COMMIT}.tar.gz)
	if(EXISTS ${CMAKE_SOURCE_DIR}/build/downloads/sdl3-switch-archive/${SONIC_SWITCH_SDL_COMMIT}.tar.gz)
		set(SONIC_SWITCH_SDL_URL
			${CMAKE_SOURCE_DIR}/build/downloads/sdl3-switch-archive/${SONIC_SWITCH_SDL_COMMIT}.tar.gz)
	endif()
	include(FetchContent)
	FetchContent_Declare(SDL3 URL ${SONIC_SWITCH_SDL_URL} URL_HASH SHA256=${SONIC_SWITCH_SDL_SHA256})
	set(SDL_SHARED OFF CACHE BOOL "" FORCE)
	set(SDL_STATIC ON CACHE BOOL "" FORCE)
	set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
	# On Windows devkitPro's CMake is MSYS2's, which writes SDL's precompiled header with a /d/... path in it, and
	# devkitPro's compiler, a Windows program, does not find it. There SDL's sources get SDL_internal.h included
	# first by the compiler's own option instead (the Switch's files of SDL count on it: without it they are empty)
	if(CMAKE_HOST_SYSTEM_NAME MATCHES "MSYS|CYGWIN|MINGW")
		set(CMAKE_DISABLE_PRECOMPILE_HEADERS ON)
	endif()
	FetchContent_MakeAvailable(SDL3)
	if(CMAKE_DISABLE_PRECOMPILE_HEADERS)
		target_compile_options(SDL3-static PRIVATE -include SDL_internal.h)
	endif()
	set(SONIC_SWITCH_SDL SDL3::SDL3-static)
endif()

add_executable(SegaSonic ports/sdl/main.c ports/sdl/settings.c ports/sdl/actions.c)
target_link_libraries(SegaSonic PRIVATE sonic_game_console sonic_data ${SONIC_SWITCH_SDL})
set_target_properties(SegaSonic PROPERTIES SUFFIX ".elf")

# devkitPro's tools: the NACP (the title's name, author and version), the NSO, main.npdm and the .nro
set(SONIC_SWITCH_TOOLS $ENV{DEVKITPRO}/tools/bin)
find_program(SONIC_NACPTOOL nacptool HINTS ${SONIC_SWITCH_TOOLS} REQUIRED)
find_program(SONIC_ELF2NSO elf2nso HINTS ${SONIC_SWITCH_TOOLS} REQUIRED)
find_program(SONIC_NPDMTOOL npdmtool HINTS ${SONIC_SWITCH_TOOLS} REQUIRED)
find_program(SONIC_ELF2NRO elf2nro HINTS ${SONIC_SWITCH_TOOLS} REQUIRED)

set(SONIC_SWITCH_TITLE_ID 05005e6a50510000)    # (homebrew's range; the .nsp's title; nacptool takes it with no 0x)
set(SONIC_SWITCH_VERSION ${PROJECT_VERSION})    # (CMakeLists.txt's project version, the port's release)
set(SONIC_SWITCH_OUT ${SONIC_BIN_DIR}/switch)
set(SONIC_SWITCH_ICON ${CMAKE_SOURCE_DIR}/ports/switch/icon.jpg)

# the .nsp's pieces, which the computer's tool puts together with the player's assets and keys
add_custom_command(OUTPUT ${SONIC_SWITCH_OUT}/main ${SONIC_SWITCH_OUT}/main.npdm ${SONIC_SWITCH_OUT}/control.nacp
		${SONIC_SWITCH_OUT}/icon.jpg
	COMMAND ${CMAKE_COMMAND} -E make_directory ${SONIC_SWITCH_OUT}
	COMMAND ${SONIC_NACPTOOL} --create "SegaSonic" "Sebastian Toyos" ${SONIC_SWITCH_VERSION}
		${SONIC_SWITCH_OUT}/control.nacp --titleid=${SONIC_SWITCH_TITLE_ID}
	COMMAND ${SONIC_ELF2NSO} $<TARGET_FILE:SegaSonic> ${SONIC_SWITCH_OUT}/main
	COMMAND ${SONIC_NPDMTOOL} ${CMAKE_SOURCE_DIR}/ports/switch/segasonic.json ${SONIC_SWITCH_OUT}/main.npdm
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SONIC_SWITCH_ICON} ${SONIC_SWITCH_OUT}/icon.jpg
	DEPENDS SegaSonic ${SONIC_SWITCH_ICON} ${CMAKE_SOURCE_DIR}/ports/switch/segasonic.json
	COMMENT "The .nsp's pieces: main (NSO), main.npdm, control.nacp, icon.jpg"
	VERBATIM)
set(SONIC_SWITCH_OUTPUTS ${SONIC_SWITCH_OUT}/main ${SONIC_SWITCH_OUT}/main.npdm)

# the .nro with the player's assets in its romfs, where this build has them
set(SONIC_SWITCH_PACK ${SONIC_BIN_DIR}/segasonic.assets)
if(EXISTS ${SONIC_SWITCH_PACK})
	set(SONIC_SWITCH_ROMFS ${CMAKE_BINARY_DIR}/switch_romfs)
	add_custom_command(OUTPUT ${SONIC_SWITCH_OUT}/SegaSonic.nro
		COMMAND ${CMAKE_COMMAND} -E make_directory ${SONIC_SWITCH_ROMFS}
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SONIC_SWITCH_PACK} ${SONIC_SWITCH_ROMFS}
		COMMAND ${SONIC_ELF2NRO} $<TARGET_FILE:SegaSonic> ${SONIC_SWITCH_OUT}/SegaSonic.nro --icon=${SONIC_SWITCH_ICON}
			--nacp=${SONIC_SWITCH_OUT}/control.nacp --romfsdir=${SONIC_SWITCH_ROMFS}
		DEPENDS SegaSonic ${SONIC_SWITCH_PACK} ${SONIC_SWITCH_OUT}/control.nacp ${SONIC_SWITCH_ICON}
		COMMENT "SegaSonic.nro with the assets pack in its romfs (the player's own: never shared)"
		VERBATIM)
	list(APPEND SONIC_SWITCH_OUTPUTS ${SONIC_SWITCH_OUT}/SegaSonic.nro)
else()
	message(STATUS "No assets pack at ${SONIC_SWITCH_PACK}: no SegaSonic.nro, the .nsp's pieces alone")
endif()
add_custom_target(SegaSonic_switch ALL DEPENDS ${SONIC_SWITCH_OUTPUTS})

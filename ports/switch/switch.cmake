# Name:        switch.cmake
# Description: the Nintendo Switch build (devkitPro's toolchain): the console game alone, as the pieces of its
#              .nsp (the NSO, main.npdm, the NACP and the icon) and, where the assets pack is, as SegaSonic.nro
# Version:     0.1.0
# Created:     23-10-2026
# Modified:    25-10-2026
# Author:      Sebastian Toyos
#
# Included by CMakeLists.txt when the toolchain is the Switch's (aarch64-none-elf-cmake). The game is the same
# console build as on a computer, with SDL2 for the Switch (switch-sdl2, devkitPro's portlibs), its picture and
# controls fixed (SEGASONIC_HANDHELD) and no EXIT in its menu (SEGASONIC_SYSTEM_EXIT: HOME leaves it). The Switch
# makes nothing from the ROM set: the game reads its assets pack from its own romfs, which a computer fills with
# the player's (the .nsp, made there with the player's keys; or, where the build has the pack beside the
# executables, build/bin/segasonic.assets, the .nro for the Homebrew Menu, the player's own and never shared).

target_compile_definitions(sonic_game_console PUBLIC SEGASONIC_HANDHELD SEGASONIC_SYSTEM_EXIT SEGASONIC_SCREEN_WIDE)

find_package(SDL2 CONFIG QUIET)
if(TARGET SDL2::SDL2)
	set(SONIC_SWITCH_SDL2 SDL2::SDL2)
else()
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(SONIC_SWITCH_SDL2_PKG REQUIRED IMPORTED_TARGET sdl2)
	set(SONIC_SWITCH_SDL2 PkgConfig::SONIC_SWITCH_SDL2_PKG)
endif()

add_executable(SegaSonic ports/sdl2/main.c ports/sdl2/settings.c ports/sdl2/actions.c)
target_link_libraries(SegaSonic PRIVATE sonic_game_console sonic_data ${SONIC_SWITCH_SDL2})
set_target_properties(SegaSonic PROPERTIES SUFFIX ".elf")

# devkitPro's tools: the NACP (the title's name, author and version), the NSO, main.npdm and the .nro
set(SONIC_SWITCH_TOOLS $ENV{DEVKITPRO}/tools/bin)
find_program(SONIC_NACPTOOL nacptool HINTS ${SONIC_SWITCH_TOOLS} REQUIRED)
find_program(SONIC_ELF2NSO elf2nso HINTS ${SONIC_SWITCH_TOOLS} REQUIRED)
find_program(SONIC_NPDMTOOL npdmtool HINTS ${SONIC_SWITCH_TOOLS} REQUIRED)
find_program(SONIC_ELF2NRO elf2nro HINTS ${SONIC_SWITCH_TOOLS} REQUIRED)

set(SONIC_SWITCH_TITLE_ID 05005e6a50510000)    # (homebrew's range; the .nsp's title; nacptool takes it with no 0x)
set(SONIC_SWITCH_VERSION 1.3.0)
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

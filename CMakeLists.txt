# Name:        CMakeLists.txt
# Description: build of the SegaSonic runtime and its development tools
# Version:     0.1.0
# Created:     11-10-2026
# Modified:    07-11-2026
# Author:      Sebastian Toyos
cmake_minimum_required(VERSION 3.20)
project(segasonic VERSION 1.4.1 LANGUAGES C CXX)    # (the port's release: OPTIONS shows it, the executables carry it)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SDL: the window port is written for SDL 3, one version on every platform that has it (Sebas, 19-11-2026: "pasarlo a
# sdl3 o el mas reciente que sirva con todo ... y va como regla para todos los ports"; the same as tmnt-port's). The
# release's archive by its SHA-256, never a moving tag; build/downloads/sdl3-source keeps a copy for builds offline.
# The Switch builds devkitPro's port of it (ports/switch/switch.cmake); the Anbernic stays on its firmware's SDL 2.0.12
# with ports/sdl/sdl2_layer.h
set(SEGASONIC_SDL_VERSION 3.4.20)
set(SEGASONIC_SDL_SHA256 f3bb06f55bed9557227a700af2ea62e49497ebdf58037732a731a5fb3ef8581f)
set(SEGASONIC_SDL_ARCHIVE SDL3-${SEGASONIC_SDL_VERSION}.tar.gz)
set(SEGASONIC_SDL_URL
	https://github.com/libsdl-org/SDL/releases/download/release-${SEGASONIC_SDL_VERSION}/${SEGASONIC_SDL_ARCHIVE})
if(EXISTS ${CMAKE_SOURCE_DIR}/build/downloads/sdl3-source/${SEGASONIC_SDL_ARCHIVE})
	set(SEGASONIC_SDL_URL ${CMAKE_SOURCE_DIR}/build/downloads/sdl3-source/${SEGASONIC_SDL_ARCHIVE})
endif()

# Every executable lands in build/bin (SONIC_BIN_DIR), whatever the generator or configuration
set(SONIC_BIN_DIR ${CMAKE_SOURCE_DIR}/build/bin CACHE PATH "Where the executables go")
foreach(config IN ITEMS "" _DEBUG _RELEASE _RELWITHDEBINFO _MINSIZEREL)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY${config} ${SONIC_BIN_DIR})
endforeach()

# A Windows executable's resources (ports/sdl/icon/version.rc.in, written for each one): the games' icon and
# the version information Windows shows in the file's Properties, with the file's name and what it is
function(segasonic_windows_resource target file_name description with_icon)
	if(NOT WIN32)
		return()
	endif()
	set(SEGASONIC_RC_FILE "${file_name}")
	set(SEGASONIC_RC_DESCRIPTION "${description}")
	if(with_icon)
		set(SEGASONIC_RC_ICON "1 ICON \"${CMAKE_SOURCE_DIR}/ports/sdl/icon/segasonic.ico\"")
	else()
		set(SEGASONIC_RC_ICON "")
	endif()
	configure_file(${CMAKE_SOURCE_DIR}/ports/sdl/icon/version.rc.in ${CMAKE_BINARY_DIR}/resources/${target}.rc @ONLY)
	target_sources(${target} PRIVATE ${CMAKE_BINARY_DIR}/resources/${target}.rc)
endfunction()

if(MSVC)
	add_compile_options(/W3 /utf-8)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
	# The releases (SEGASONIC_STATIC_SDL, below) carry the C runtime in the executables too: they need no
	# Visual C++ redistributable on the player's machine
	if(SEGASONIC_STATIC_SDL)
		set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
		set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)    # (SDL's targets too)
	endif()
endif()

# System 32 board model: part of the game (sound.c is the Z80's side of the board, video.c draws the frames,
# script.c drives the controls in test runs)
add_library(s32runtime STATIC
	runtime/src/machine.c
	runtime/src/io.c
	runtime/src/script.c
	runtime/src/sound.c
	runtime/src/sound_chips.c
	runtime/src/rf5c68.c
	runtime/src/ym3438.c
	runtime/src/video.c
	runtime/src/workers.c
	runtime/src/memory_census.c
)
target_include_directories(s32runtime PUBLIC runtime/include)
# (the video's rows on several threads, workers.c: POSIX threads but on Windows and the Switch)
if(NOT WIN32 AND NOT (NINTENDO_SWITCH OR CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch"))
	find_package(Threads REQUIRED)
	target_link_libraries(s32runtime PUBLIC Threads::Threads)
endif()
if(UNIX AND NOT APPLE)
	target_link_libraries(s32runtime PUBLIC m)    # (the C library's maths are apart there)
endif()

# The game's data: no executable carries the ROM or anything from it. The game loads its assets pack when
# it starts (runtime/src/game_data.c): tools/make_pack.py writes it beside the executables from assets/,
# which extract_assets.py takes out of the player's ROM set (rom/, its files or its .zip; SONIC_ROM picks
# another); a change to an asset goes into the pack. The reference runs the original code, so it reads
# the ROM set itself when it starts (runtime/src/rom_set.c). The ROM never goes into the repository.
# Without a ROM set (a build for another machine) nothing of the game's is made, only the executables
find_package(Python3 COMPONENTS Interpreter)
set(SONIC_ROM ${CMAKE_SOURCE_DIR}/rom CACHE PATH "The ROM set the assets are taken out of")
set(SONIC_ROM_FILES "")
if(IS_DIRECTORY ${SONIC_ROM})
	file(GLOB SONIC_ROM_FILES ${SONIC_ROM}/*)
elseif(EXISTS ${SONIC_ROM})
	set(SONIC_ROM_FILES ${SONIC_ROM})
endif()
add_library(sonic_data STATIC runtime/src/game_data.c runtime/src/rom_set.c runtime/src/inflate.c
	runtime/src/extract.c runtime/src/pack_layout.c)
target_include_directories(sonic_data PUBLIC runtime/include)
if(SONIC_ROM_FILES AND Python3_FOUND)
	file(GLOB_RECURSE SONIC_ASSET_FILES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/assets/*)
	file(GLOB SONIC_FORMAT_TOOLS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/tools/formats/*.py)
	set(SONIC_ASSETS_PACK ${SONIC_BIN_DIR}/segasonic.assets)
	add_custom_command(OUTPUT ${SONIC_ASSETS_PACK}
		COMMAND Python3::Interpreter ${CMAKE_SOURCE_DIR}/extract_assets.py ${SONIC_ROM}
		COMMAND Python3::Interpreter ${CMAKE_SOURCE_DIR}/tools/make_pack.py ${SONIC_ASSETS_PACK}
		DEPENDS ${SONIC_ROM_FILES} ${SONIC_ASSET_FILES} ${SONIC_FORMAT_TOOLS} ${CMAKE_SOURCE_DIR}/assets.json
			${CMAKE_SOURCE_DIR}/extract_assets.py ${CMAKE_SOURCE_DIR}/tools/make_pack.py
			${CMAKE_SOURCE_DIR}/tools/sonic_rom.py ${CMAKE_SOURCE_DIR}/tools/asset_formats.py
			${CMAKE_SOURCE_DIR}/tools/data_formats.py
		COMMENT "Taking the assets out of the ROM set and packing them for the game")
	add_custom_target(sonic_assets ALL DEPENDS ${SONIC_ASSETS_PACK})
	add_dependencies(sonic_data sonic_assets)
else()
	message(STATUS "No ROM set at ${SONIC_ROM} (or no Python): no assets pack; the game needs one beside it")
endif()

# iPhone and iPad (Xcode's toolchain, through CMake's Xcode generator: ports/ios/ios.cmake): the console game alone,
# as Android's, an app of its own; no tools of the computers'
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
	set(SEGASONIC_IOS ON)
endif()

# The Switch's .nsp, made on the player's computer (ports/switch/nsp): the game's Switch build (main,
# main.npdm, control.nacp, icon.jpg), the assets of the player's ROM set and the keys of the player's console.
# A command line; with SDL also a window (below, with the game's window)
if(NOT (NINTENDO_SWITCH OR CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch") AND NOT ANDROID AND NOT SEGASONIC_IOS)
	add_executable(segasonic_nsp ports/switch/nsp/main.c ports/switch/nsp/make_nsp.c ports/switch/nsp/formats.c
		ports/switch/nsp/crypto.c)
	target_link_libraries(segasonic_nsp PRIVATE sonic_data)
	set_target_properties(segasonic_nsp PROPERTIES OUTPUT_NAME "SegaSonic NSP")
	segasonic_windows_resource(segasonic_nsp "SegaSonic NSP.exe" "SegaSonic NSP: the Nintendo Switch's .nsp of SegaSonic" OFF)
endif()

# Android's APK, made on the player's computer (ports/android/apk): the game's Android build
# (android/segasonic-base.apk), the assets of the player's ROM set, signed with the player's own key; its
# SHA-256 is SegaSonic NSP's (crypto.c). A command line; with SDL also a window (below, with the game's
# window)
if(NOT (NINTENDO_SWITCH OR CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch") AND NOT ANDROID AND NOT SEGASONIC_IOS)
	add_executable(segasonic_apk_maker ports/android/apk/main.c ports/android/apk/make_apk.c ports/android/apk/zip.c
		ports/android/apk/sign.c ports/android/apk/key.c ports/android/apk/ecdsa.c ports/switch/nsp/crypto.c)
	target_include_directories(segasonic_apk_maker PRIVATE ports/switch/nsp)
	target_link_libraries(segasonic_apk_maker PRIVATE sonic_data)
	if(WIN32)
		target_link_libraries(segasonic_apk_maker PRIVATE bcrypt)    # (BCryptGenRandom: the key)
	endif()
	set_target_properties(segasonic_apk_maker PROPERTIES OUTPUT_NAME "SegaSonic APK")
	segasonic_windows_resource(segasonic_apk_maker "SegaSonic APK.exe" "SegaSonic APK: Android's APK of SegaSonic" OFF)
endif()

# The translation scaffold the decompiler grew from: the V60 runtime plus the C tools/recompile.py writes
# from the user's ROM into build/generated, instruction by instruction (for comparisons only)
option(SEGASONIC_SCAFFOLD "Build the V60 translation scaffold (sonic_headless)" OFF)
file(GLOB SONIC_GENERATED CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/build/generated/*.c)
if(SEGASONIC_SCAFFOLD AND SONIC_GENERATED)
	add_library(sonic_program STATIC runtime/src/v60.c ${SONIC_GENERATED})
	target_include_directories(sonic_program PUBLIC runtime/include build/generated)
	target_link_libraries(sonic_program PUBLIC s32runtime)
	if(MSVC)
		target_compile_options(sonic_program PRIVATE /bigobj)
	endif()

	add_executable(sonic_headless ports/headless/main.c)
	target_compile_definitions(sonic_headless PRIVATE SEGASONIC_SCAFFOLD)
	target_link_libraries(sonic_headless PRIVATE sonic_program sonic_data)
elseif(SEGASONIC_SCAFFOLD)
	message(STATUS "No generated program yet: run tools/recompile.py to build the scaffold")
endif()

# The game: its runtime plus its C, src/ (tools/decompile.py writes it; SONIC_DECOMPILED_DIR picks another
# copy, such as one written with --trace for debugging)
set(SONIC_DECOMPILED_DIR ${CMAKE_SOURCE_DIR}/src CACHE PATH "The game's C (tools/decompile.py)")
file(GLOB_RECURSE SONIC_DECOMPILED CONFIGURE_DEPENDS ${SONIC_DECOMPILED_DIR}/*.c)
if(SONIC_DECOMPILED)
	add_library(sonic_game STATIC runtime/src/game.c runtime/src/sound_cpu.c ${SONIC_DECOMPILED})
	target_include_directories(sonic_game PUBLIC runtime/include ${SONIC_DECOMPILED_DIR} ${SONIC_DECOMPILED_DIR}/sound)
	target_link_libraries(sonic_game PUBLIC s32runtime)
	if(MSVC)
		target_compile_options(sonic_game PRIVATE /bigobj)
	else()
		# variables are typed lvalues over the work RAM bytes (game/support.h)
		target_compile_options(sonic_game PRIVATE -fno-strict-aliasing)
	endif()

	# a development aid: every mem_read and mem_write of the game's C counted by its line
	# (sonic_game_headless --memory-census FILE; runtime/src/memory_census.c)
	option(SONIC_MEMORY_CENSUS "Count the game's memory accesses by line of C" OFF)
	if(SONIC_MEMORY_CENSUS)
		target_compile_definitions(sonic_game PUBLIC S32_MEMORY_CENSUS)
	endif()

	add_executable(sonic_game_headless ports/headless/main.c)
	target_link_libraries(sonic_game_headless PRIVATE sonic_game sonic_data)

	# The console build: the same game with the console's own versions of the functions console/replaced.txt
	# lists (console/*.c), in SegaSonic.exe
	file(GLOB SONIC_CONSOLE_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/console/*.c)
	add_library(sonic_game_console STATIC runtime/src/game.c runtime/src/sound_cpu.c ${SONIC_DECOMPILED}
		${SONIC_CONSOLE_SOURCES})
	target_include_directories(sonic_game_console PUBLIC runtime/include ${SONIC_DECOMPILED_DIR}
		${SONIC_DECOMPILED_DIR}/sound console)
	target_compile_definitions(sonic_game_console PUBLIC SEGASONIC_CONSOLE SEGASONIC_VERSION="${PROJECT_VERSION}")
	target_link_libraries(sonic_game_console PUBLIC s32runtime)
	# MULTIPLAYER, the LAN game (console/lan.c): arcade-lan, a repository of its own (the submodule lan/, or
	# ARCADE_LAN_DIR where it is), as TMNT's: without it the menu's MULTIPLAYER says NO NETWORK
	set(ARCADE_LAN_DIR ${CMAKE_SOURCE_DIR}/lan CACHE PATH "arcade-lan's sources, for SegaSonic's LAN game")
	if(EXISTS ${ARCADE_LAN_DIR}/CMakeLists.txt)
		# (iOS refuses broadcasts to an app without Apple's multicast entitlement: the rooms there are found by the
		# library's SEEKs and sweeps, ARCADE_LAN_NO_BROADCAST, as tmnt-port's)
		if(SEGASONIC_IOS)
			set(ARCADE_LAN_NO_BROADCAST ON CACHE BOOL "" FORCE)
		endif()
		add_subdirectory(${ARCADE_LAN_DIR} ${CMAKE_BINARY_DIR}/arcade_lan)
		target_link_libraries(sonic_game_console PUBLIC arcade_lan)
		target_compile_definitions(sonic_game_console PUBLIC SEGASONIC_LAN)
	endif()
	add_executable(segasonic_console_headless ports/headless/main.c)
	target_link_libraries(segasonic_console_headless PRIVATE sonic_game_console sonic_data)
	if(MSVC)
		target_compile_options(sonic_game_console PRIVATE /bigobj)
	else()
		target_compile_options(sonic_game_console PRIVATE -fno-strict-aliasing)
	endif()

	# SDL_GameControllerDB (ports/sdl/controllerdb, zlib License): the community's mappings of game controllers,
	# this platform's lines built into the games on the computers and Android (TMNT's players' report: an Nvidia
	# Shield's PS4 and Xbox controllers); the Switch and the Anbernic keep their own controls. main.c gives SDL
	# the mappings before it starts (SEGASONIC_CONTROLLER_DB, to whatever links it)
	if(NOT (NINTENDO_SWITCH OR CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch") AND NOT SEGASONIC_ANBERNIC)
		if(ANDROID)
			set(SEGASONIC_CONTROLLER_PLATFORM "Android")
		elseif(SEGASONIC_IOS)
			set(SEGASONIC_CONTROLLER_PLATFORM "iOS")
		elseif(APPLE)
			set(SEGASONIC_CONTROLLER_PLATFORM "Mac OS X")
		elseif(WIN32)
			set(SEGASONIC_CONTROLLER_PLATFORM "Windows")
		else()
			set(SEGASONIC_CONTROLLER_PLATFORM "Linux")
		endif()
		set(SEGASONIC_CONTROLLER_DB_C ${CMAKE_BINARY_DIR}/controller_db.c)
		add_custom_command(OUTPUT ${SEGASONIC_CONTROLLER_DB_C}
			COMMAND ${CMAKE_COMMAND} -DDB=${CMAKE_SOURCE_DIR}/ports/sdl/controllerdb/gamecontrollerdb.txt
				"-DPLATFORM=${SEGASONIC_CONTROLLER_PLATFORM}" -DOUT=${SEGASONIC_CONTROLLER_DB_C}
				-P ${CMAKE_SOURCE_DIR}/ports/sdl/controllerdb/embed.cmake
			DEPENDS ${CMAKE_SOURCE_DIR}/ports/sdl/controllerdb/gamecontrollerdb.txt
				${CMAKE_SOURCE_DIR}/ports/sdl/controllerdb/embed.cmake
			COMMENT "SDL_GameControllerDB's mappings for ${SEGASONIC_CONTROLLER_PLATFORM}"
			VERBATIM)
		add_library(segasonic_controller_db STATIC ${SEGASONIC_CONTROLLER_DB_C})
		set_target_properties(segasonic_controller_db PROPERTIES POSITION_INDEPENDENT_CODE ON)
		target_compile_definitions(segasonic_controller_db INTERFACE SEGASONIC_CONTROLLER_DB)
	endif()

	# The Nintendo Switch (devkitPro's toolchain): the console game alone, its .nro and its .nsp's pieces
	# (ports/switch/switch.cmake); nothing of what follows
	if(NINTENDO_SWITCH OR CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
		include(ports/switch/switch.cmake)
		return()
	endif()

	# Android (the NDK's toolchain): the console game alone, as libmain.so in its APK
	# (ports/android/android.cmake)
	if(ANDROID)
		include(ports/android/android.cmake)
		return()
	endif()

	# iPhone and iPad (ports/ios/ios.cmake): the console game alone, as an app, and its unsigned .ipa
	if(SEGASONIC_IOS)
		include(ports/ios/ios.cmake)
		return()
	endif()

	# Anbernic's H700 handhelds (toolchain ports/anbernic/zig_aarch64.cmake): the console game alone, as a
	# folder for the stock firmware's Roms/PORTS (ports/anbernic/anbernic.cmake)
	option(SEGASONIC_ANBERNIC "Build the game for Anbernic's H700 handhelds" OFF)
	if(SEGASONIC_ANBERNIC)
		include(ports/anbernic/anbernic.cmake)
		return()
	endif()

	# The game in a window (ports/sdl): SDL 3 from the system, or on Windows the development package
	# unpacked in build/downloads/sdl3 (SDL3_DIR picks another), a library beside the executable; or, with
	# SEGASONIC_STATIC_SDL (the releases), SDL 3 built from its own source into the executable (the version
	# and the archive's SHA-256 at the top, SEGASONIC_SDL_VERSION)
	option(SEGASONIC_STATIC_SDL "Build SDL from its source into the executables" OFF)
	if(SEGASONIC_STATIC_SDL)
		include(FetchContent)
		FetchContent_Declare(SDL3 URL ${SEGASONIC_SDL_URL} URL_HASH SHA256=${SEGASONIC_SDL_SHA256})
		set(SDL_SHARED OFF CACHE BOOL "" FORCE)
		set(SDL_STATIC ON CACHE BOOL "" FORCE)
		set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
		set(SDL_FORCE_STATIC_VCRT ON CACHE BOOL "" FORCE)    # (MSVC: the C runtime inside, as the game's)
		FetchContent_MakeAvailable(SDL3)
		set(SONIC_SDL SDL3::SDL3-static)
		set(SDL3_FOUND TRUE)
	else()
		set(SONIC_SDL_UNPACKED "")
		if(WIN32)    # (the VC package: its libraries are Windows' only, a source tree shared with WSL has it)
			file(GLOB SONIC_SDL_UNPACKED ${CMAKE_SOURCE_DIR}/build/downloads/sdl3/SDL3-*/cmake)
		endif()
		find_package(SDL3 CONFIG QUIET HINTS ${SONIC_SDL_UNPACKED})
		set(SONIC_SDL SDL3::SDL3)
	endif()
	if(SDL3_FOUND)
		# (an .app on macOS: SDL_GetBasePath is its Resources folder)
		add_executable(sonic WIN32 MACOSX_BUNDLE ports/sdl/main.c ports/sdl/settings.c ports/sdl/first_run.c
			ports/sdl/letters.c ports/sdl/mice.c)
		target_link_libraries(sonic PRIVATE sonic_game sonic_data segasonic_controller_db ${SONIC_SDL})
		# (the arcade game beside the console one: "SegaSonic Arcade" its executable, .app and window)
		set_target_properties(sonic PROPERTIES OUTPUT_NAME "SegaSonic Arcade" MACOSX_BUNDLE_BUNDLE_NAME "SegaSonic Arcade"
			MACOSX_BUNDLE_GUI_IDENTIFIER io.github.cheviman.segasonic-port.arcade)
		if(WIN32 AND NOT SEGASONIC_STATIC_SDL)
			add_custom_command(TARGET sonic POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL3::SDL3> $<TARGET_FILE_DIR:sonic>)
		endif()
		# the first run's letters (Kenney Mini, CC0; tools/make_font.py): beside the executable, or in the
		# .app's Resources on macOS
		if(APPLE)
			target_sources(sonic PRIVATE ports/sdl/font/font.bmp)
			set_source_files_properties(ports/sdl/font/font.bmp PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
		else()
			add_custom_command(TARGET sonic POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/ports/sdl/font/font.bmp
					$<TARGET_FILE_DIR:sonic>)
		endif()
		add_executable(SegaSonic WIN32 MACOSX_BUNDLE ports/sdl/main.c ports/sdl/settings.c ports/sdl/actions.c
			ports/sdl/first_run.c ports/sdl/letters.c ports/sdl/mice.c)
		target_link_libraries(SegaSonic PRIVATE sonic_game_console sonic_data segasonic_controller_db ${SONIC_SDL})
		# a development aid: SegaSonic with Android's touch controls, the mouse a finger, to look at them in a
		# window of any shape (as OutRunners')
		option(SEGASONIC_TOUCH_TEST "Build SegaSonic with Android's touch controls, the mouse as a finger" OFF)
		if(SEGASONIC_TOUCH_TEST)
			target_sources(SegaSonic PRIVATE ports/sdl/touch.c)
			target_compile_definitions(SegaSonic PRIVATE SEGASONIC_TOUCH SEGASONIC_TOUCH_TEST)
		endif()
		set_target_properties(SegaSonic PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "SegaSonic"
			MACOSX_BUNDLE_GUI_IDENTIFIER io.github.cheviman.segasonic-port)
		if(WIN32 AND NOT SEGASONIC_STATIC_SDL)
			add_custom_command(TARGET SegaSonic POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL3::SDL3> $<TARGET_FILE_DIR:SegaSonic>)
		endif()
		# the CONTROLS screen's key and button pictures (Kenney's Input Prompts Pixel, CC0) and the first
		# run's letters (Kenney Mini, CC0): beside the executable, or in the .app's Resources on macOS
		if(APPLE)
			target_sources(SegaSonic PRIVATE ports/sdl/prompts/input_prompts.bmp ports/sdl/font/font.bmp)
			set_source_files_properties(ports/sdl/prompts/input_prompts.bmp PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
		else()
			add_custom_command(TARGET SegaSonic POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/ports/sdl/prompts/input_prompts.bmp
					$<TARGET_FILE_DIR:SegaSonic>
				COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/ports/sdl/font/font.bmp
					$<TARGET_FILE_DIR:SegaSonic>)
		endif()
		# the executables' icon (tools/make_icon.py): a resource on Windows, with the version information
		# (segasonic_windows_resource), the .app's on macOS, a picture beside them on Linux (for a launcher of
		# the desktop's)
		segasonic_windows_resource(sonic "SegaSonic Arcade.exe" "SegaSonic Arcade: the arcade game" ON)
		segasonic_windows_resource(SegaSonic "SegaSonic.exe" "SegaSonic The Hedgehog" ON)
		foreach(game sonic SegaSonic)
			if(APPLE)
				target_sources(${game} PRIVATE ports/sdl/icon/segasonic.icns)
				set_target_properties(${game} PROPERTIES MACOSX_BUNDLE_ICON_FILE segasonic.icns)
			elseif(NOT WIN32)
				add_custom_command(TARGET ${game} POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/ports/sdl/icon/segasonic.png
						$<TARGET_FILE_DIR:${game}>)
			endif()
		endforeach()
		if(APPLE)
			set_source_files_properties(ports/sdl/icon/segasonic.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
		endif()

		# SegaSonic NSP's window (ports/switch/nsp/window.c), when it is started with nothing: still a console
		# program, with its own main (SDL_MAIN_HANDLED); the folder dialog is COM's on Windows. Its
		# letters (Kenney Mini, font.bmp) beside it, as SDL3.dll is
		target_sources(segasonic_nsp PRIVATE ports/switch/nsp/window.c ports/switch/nsp/folder_dialog.c
			ports/sdl/letters.c)
		target_include_directories(segasonic_nsp PRIVATE ports/sdl)
		target_compile_definitions(segasonic_nsp PRIVATE SEGASONIC_NSP_WINDOW)
		target_link_libraries(segasonic_nsp PRIVATE ${SONIC_SDL})
		if(WIN32)
			target_link_libraries(segasonic_nsp PRIVATE ole32 shell32 uuid)
			if(NOT SEGASONIC_STATIC_SDL)
				add_custom_command(TARGET segasonic_nsp POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL3::SDL3> $<TARGET_FILE_DIR:segasonic_nsp>)
			endif()
		endif()
		add_custom_command(TARGET segasonic_nsp POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/ports/sdl/font/font.bmp
				$<TARGET_FILE_DIR:segasonic_nsp>)

		# SegaSonic APK's window (ports/android/apk/window.c), as SegaSonic NSP's: its folder dialog (with a
		# title of its own) and its letters, font.bmp and SDL3.dll beside it
		target_sources(segasonic_apk_maker PRIVATE ports/android/apk/window.c ports/switch/nsp/folder_dialog.c
			ports/sdl/letters.c)
		target_include_directories(segasonic_apk_maker PRIVATE ports/sdl)
		target_compile_definitions(segasonic_apk_maker PRIVATE SEGASONIC_APK_WINDOW
			[[FOLDER_TITLE="The folder for SegaSonic.apk"]])
		target_link_libraries(segasonic_apk_maker PRIVATE ${SONIC_SDL})
		if(WIN32)
			target_link_libraries(segasonic_apk_maker PRIVATE ole32 shell32 uuid)
			if(NOT SEGASONIC_STATIC_SDL)
				add_custom_command(TARGET segasonic_apk_maker POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL3::SDL3>
						$<TARGET_FILE_DIR:segasonic_apk_maker>)
			endif()
		endif()
		add_custom_command(TARGET segasonic_apk_maker POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/ports/sdl/font/font.bmp
				$<TARGET_FILE_DIR:segasonic_apk_maker>)
	else()
		message(STATUS "No SDL 3: the window port (sonic) is not built, and SegaSonic NSP and SegaSonic APK are "
			"their command lines alone")
	endif()
else()
	message(STATUS "No game C in ${SONIC_DECOMPILED_DIR}: run tools/decompile.py")
endif()

# Development tools built on MAME's V60 code; never linked into the game
option(SEGASONIC_REFERENCE_TOOLS "Build the MAME-based reference tools" ON)
if(SEGASONIC_REFERENCE_TOOLS)
	add_executable(oracle
		tools/reference/oracle.cpp
		tools/reference/mame/v60d.cpp
	)
	target_include_directories(oracle PRIVATE tools/reference/stub tools/reference/mame)

	# MAME's Z80 disassembler, the reference tools/z80/disassembler.py is compared with
	add_executable(oracle_z80
		tools/reference/oracle_z80.cpp
		tools/reference/mame/z80/z80dasm.cpp
	)
	target_include_directories(oracle_z80 PRIVATE tools/reference/stub tools/reference/mame/z80)

	# MAME's System 32 video drawing the board model's frames: the frames the game's video has to match
	add_library(s32video_reference STATIC tools/reference/video/s32video.cpp)
	target_include_directories(s32video_reference PUBLIC tools/reference/video PRIVATE runtime/include)
	if(MSVC)
		target_compile_options(s32video_reference PRIVATE /wd4244 /wd4267 /wd4100 /wd4146)
	endif()

	# MAME's Z80 and RF5C68 and ymfm's YM3438 on the board model: the sound the game's has to match. The Z80
	# core's steps are written from z80.lst at build time, as MAME does
	set(SONIC_Z80_STEPS ${CMAKE_BINARY_DIR}/generated/cpu/z80/z80.hxx)
	add_custom_command(OUTPUT ${SONIC_Z80_STEPS}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/generated/cpu/z80
		COMMAND Python3::Interpreter ${CMAKE_SOURCE_DIR}/tools/reference/mame/z80/z80make.py z80
			${CMAKE_SOURCE_DIR}/tools/reference/mame/z80/z80.lst ${SONIC_Z80_STEPS}
		DEPENDS tools/reference/mame/z80/z80make.py tools/reference/mame/z80/z80.lst
		VERBATIM)
	add_library(s32sound_reference STATIC
		tools/reference/sound/s32sound.cpp
		tools/reference/ymfm/ymfm_opn.cpp
		tools/reference/ymfm/ymfm_adpcm.cpp
		tools/reference/ymfm/ymfm_ssg.cpp
		${SONIC_Z80_STEPS}
	)
	target_include_directories(s32sound_reference PUBLIC tools/reference/sound
		PRIVATE tools/reference/ymfm runtime/include ${CMAKE_BINARY_DIR}/generated)
	if(MSVC)
		target_compile_options(s32sound_reference PRIVATE /bigobj /wd4244 /wd4267 /wd4100 /wd4146 /wd4127)
	endif()

	add_executable(s32ref
		tools/reference/s32ref.cpp
		tools/reference/mame/v60.cpp
		tools/reference/mame/v60d.cpp
	)
	target_include_directories(s32ref PRIVATE tools/reference/stub tools/reference/mame)
	target_link_libraries(s32ref PRIVATE s32runtime sonic_data s32video_reference s32sound_reference)
	if(MSVC)
		# MAME's core marks error handlers [[noreturn]] even though they return uint32_t
		target_compile_options(s32ref PRIVATE /wd4646 /wd4244)
	endif()

	# The game's YM3438 and RF5C68 against ymfm's and MAME's, sample for sample, over s32ref's chip write
	# logs and synthetic sequences; the game's C is compiled in directly
	add_executable(chipcheck
		tools/reference/sound/chipcheck.cpp
		runtime/src/ym3438.c
		runtime/src/rf5c68.c
		tools/reference/ymfm/ymfm_opn.cpp
		tools/reference/ymfm/ymfm_adpcm.cpp
		tools/reference/ymfm/ymfm_ssg.cpp
	)
	target_include_directories(chipcheck PRIVATE tools/reference/sound tools/reference/ymfm runtime/include)
	if(MSVC)
		target_compile_options(chipcheck PRIVATE /wd4244 /wd4267 /wd4100 /wd4146 /wd4127)
	endif()
endif()

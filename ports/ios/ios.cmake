# Name:        ios.cmake
# Description: the iOS and iPadOS build (Xcode, CMake's Xcode generator): SegaSonic as an app on SDL 3's
#              UIKit backend, for an iPhone or an iPad (arm64) or the Simulator, and an unsigned .ipa of it
# Version:     0.1.0
# Created:     27-11-2026
# Modified:    27-11-2026
# Author:      Sebastian Toyos
#
# Included by CMakeLists.txt when CMAKE_SYSTEM_NAME is iOS, as tmnt-port's (Sebas, 19-11-2026: "y anota que hay que
# agregar una version de ios", every port's; SegaSonic's 27-11-2026). The game is Android's build on Apple's phones and
# tablets: the screen whole (SEGASONIC_HANDHELD: no first run's window, no VSYNC nor window lines), no EXIT in the menu
# (SEGASONIC_SYSTEM_EXIT: the home gesture leaves it), the touch screen's controls (SEGASONIC_TOUCH, ports/sdl/touch.c),
# the game's WIDE by the screen's shape (main.c's follow_screen, between the sides the notch or the Dynamic Island
# takes), the game controllers iOS knows (MFi, Xbox, PlayStation, Switch Pro: SDL's driver for Apple's GameController,
# with SDL_GameControllerDB's iOS lines). The app takes its assets out of the player's sonic.zip in its Documents folder
# at its first start (main.c, into segasonic.assets there), which the Files app shows as the app's own (Info.plist.in:
# UIFileSharingEnabled, LSSupportsOpeningDocumentsInPlace). One app, one folder. SDL from its source, the computers'
# version (CMakeLists.txt's SEGASONIC_SDL_VERSION), static; main.c's SDL_main.h starts UIKit. The icon is the
# project's (Assets.xcassets: the Android app's trackball, its two layers made opaque), the launch screen black
# (LaunchScreen.storyboard), both compiled by Xcode. The app is not signed unless SEGASONIC_IOS_TEAM names a
# development team (a Mac building for its own iPhone): the release's .ipa is unsigned, and the player signs it as it
# is installed (AltStore, SideStore, Sideloadly: docs/package/README-ios.txt). Never the App Store: the game is Sega's,
# and the player brings the ROM set.
#
# Usage (macOS, Xcode; for the Simulator, CMAKE_OSX_SYSROOT iphonesimulator, in a build folder of its own):
#   cmake -S . -B build/ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_ARCHITECTURES=arm64
#       -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0 -DSEGASONIC_REFERENCE_TOOLS=OFF [-DCMAKE_OSX_SYSROOT=iphonesimulator]
#       [-DSONIC_BIN_DIR=build/bin-ios] [-DSEGASONIC_IOS_TEAM=<team ID>]
#   cmake --build build/ios --config Release --target segasonic_ios_games -- CODE_SIGNING_ALLOWED=NO
# The app: SONIC_BIN_DIR/ios/SegaSonic.app; for a device, also SegaSonic.ipa there (the app inside Payload/, zipped).

if(NOT CMAKE_GENERATOR STREQUAL "Xcode")
	message(FATAL_ERROR "The iOS build is the Xcode generator's (-G Xcode): Xcode compiles the apps' launch screen and "
		"icon, and puts the bundles together")
endif()

set(SEGASONIC_IOS_DEFINITIONS SEGASONIC_HANDHELD SEGASONIC_SYSTEM_EXIT SEGASONIC_SCREEN_WIDE SEGASONIC_TOUCH)
target_compile_definitions(sonic_game_console PUBLIC ${SEGASONIC_IOS_DEFINITIONS})

set(SEGASONIC_IOS_TEAM "" CACHE STRING "The development team that signs the app (none: unsigned, as the release's)")
set(SEGASONIC_IOS_OUT ${SONIC_BIN_DIR}/ios)
if(CMAKE_OSX_SYSROOT MATCHES "[Ss]imulator")
	set(SEGASONIC_IOS_SIMULATOR ON)
endif()

# SDL from its source, static (FETCHCONTENT_SOURCE_DIR_SDL3 picks a copy already here). No HIDAPI: iOS's game
# controllers come through Apple's GameController framework, and SDL's HIDAPI there talks Bluetooth itself, which
# needs a permission the apps do not ask for
include(FetchContent)
FetchContent_Declare(SDL3 URL ${SEGASONIC_SDL_URL} URL_HASH SHA256=${SEGASONIC_SDL_SHA256})
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_HIDAPI OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL3)

# the app's resources, which Xcode copies into the bundle (compiling the launch screen and the asset catalog): the
# letters of the touch controls and the menus, the launch screen, the icon
set(SEGASONIC_IOS_RESOURCES ports/sdl/font/font.bmp ports/sdl/prompts/input_prompts.bmp ports/ios/LaunchScreen.storyboard
	ports/ios/Assets.xcassets)
set_source_files_properties(${SEGASONIC_IOS_RESOURCES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

set(SEGASONIC_IOS_OUTPUTS "")

# A game of iOS: its app ('target', named 'name', its bundle 'identifier'), the window port's main.c, settings.c,
# actions.c, letters.c and touch.c and the sources after 'program' besides, on 'program' (sonic_game_console: the
# console game, the assets out of the player's sonic.zip on the device); for a device, its
# .ipa beside it
function(segasonic_ios_game target name identifier program)
	add_executable(${target} MACOSX_BUNDLE ports/sdl/main.c ports/sdl/settings.c ports/sdl/actions.c ports/sdl/letters.c
		ports/sdl/touch.c ${ARGN} ${SEGASONIC_IOS_RESOURCES})
	target_include_directories(${target} PRIVATE ports/sdl)
	target_link_libraries(${target} PRIVATE ${program} sonic_data segasonic_controller_db SDL3::SDL3-static)
	target_compile_definitions(${target} PRIVATE ${SEGASONIC_IOS_DEFINITIONS})
	set_target_properties(${target} PROPERTIES OUTPUT_NAME ${name}
		MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/ports/ios/Info.plist.in
		MACOSX_BUNDLE_BUNDLE_NAME ${name} MACOSX_BUNDLE_GUI_IDENTIFIER ${identifier}
		MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION} MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
		XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER ${identifier}
		XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"    # (iPhone and iPad)
		XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AppIcon)
	foreach(config IN ITEMS "" _DEBUG _RELEASE _RELWITHDEBINFO _MINSIZEREL)
		set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY${config} ${SEGASONIC_IOS_OUT})
	endforeach()
	if(SEGASONIC_IOS_TEAM)
		set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ${SEGASONIC_IOS_TEAM}
			XCODE_ATTRIBUTE_CODE_SIGN_STYLE Automatic XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Development")
	else()
		set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO
			XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
	endif()

	set(outputs "")
	if(NOT SEGASONIC_IOS_SIMULATOR)
		# its .ipa: the app in Payload/, zipped (ditto keeps the bundle's permissions), as a sideloading tool takes it
		set(stage ${CMAKE_BINARY_DIR}/ipa/${target})
		set(ipa ${SEGASONIC_IOS_OUT}/${name}.ipa)
		add_custom_command(OUTPUT ${ipa}
			COMMAND ${CMAKE_COMMAND} -E rm -rf ${stage} ${ipa}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${stage}/Payload
			COMMAND ditto $<TARGET_BUNDLE_DIR:${target}> ${stage}/Payload/${name}.app
			COMMAND ${CMAKE_COMMAND} -E chdir ${stage} ditto -c -k --keepParent Payload ${ipa}
			DEPENDS ${target}
			COMMENT "${name}.ipa (unsigned: the player's sideloading tool signs it)"
			VERBATIM)
		list(APPEND outputs ${ipa})
	endif()
	set(SEGASONIC_IOS_OUTPUTS ${SEGASONIC_IOS_OUTPUTS} ${outputs} PARENT_SCOPE)
endfunction()

segasonic_ios_game(segasonic_ios "SegaSonic" io.github.cheviman.segasonic-port sonic_game_console)
add_custom_target(segasonic_ios_games ALL DEPENDS ${SEGASONIC_IOS_OUTPUTS})
add_dependencies(segasonic_ios_games segasonic_ios)

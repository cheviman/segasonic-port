# Name:        android.cmake
# Description: the Android build (the NDK's toolchain): the console game as libmain.so beside SDL2's, both from
#              their source, and the APKs around them: the game's, for SegaSonic APK, and this computer's own
# Version:     0.1.0
# Created:     25-10-2026
# Modified:    25-10-2026
# Author:      Sebastian Toyos
#
# Included by CMakeLists.txt when the toolchain is the NDK's (build/cmake/android.toolchain.cmake, with
# ANDROID_ABI and ANDROID_PLATFORM). The game is the console build with its picture fixed and WIDE
# (SEGASONIC_HANDHELD, SEGASONIC_SCREEN_WIDE), no EXIT in its menu (SEGASONIC_SYSTEM_EXIT: Android's home
# leaves it) but in the pause (SEGASONIC_PAUSE_EXIT) and the touch screen's controls (SEGASONIC_TOUCH), in
# libmain.so, which SDL's activity starts (its Java, from SDL's source). The APKs are put together without
# Gradle, by the SDK's own tools: javac and d8 (the Java), aapt2 (the manifest, the icon, the assets), aapt (the libraries and the Java in), zipalign
# and apksigner. Android makes nothing from the ROM set: an APK carries the assets pack taken out of the
# player's ROM set on a computer, so it is its player's alone and never shared. Two of them:
#   build/bin/android/segasonic-base.apk   the game without a pack or a signature: SegaSonic APK
#                                           (ports/android/apk) makes the player's APK of it, on their computer
#   build/bin/android/SegaSonic-debug.apk   this computer's own, with its pack (build/bin/segasonic.assets),
#                                           signed with the debug key of its Android tools
#                                           (~/.android/debug.keystore, which keytool makes when there is none;
#                                           the same key each time, so a new APK installs over the last one and
#                                           keeps what the game wrote)

target_compile_definitions(sonic_game_console PUBLIC SEGASONIC_HANDHELD SEGASONIC_SYSTEM_EXIT SEGASONIC_SCREEN_WIDE
	SEGASONIC_PAUSE_EXIT SEGASONIC_TOUCH)

set(SONIC_ANDROID_VERSION 1.3.0)
set(SONIC_ANDROID_VERSION_CODE 10300)    # (major * 10000 + minor * 100 + patch)
set(SONIC_ANDROID_TARGET_SDK 35)
set(SONIC_ANDROID_MIN_SDK 24)    # (Android 7: the APK's signature is v2's alone, as SegaSonic APK signs it)
string(REGEX MATCH "[0-9]+" SONIC_ANDROID_PLATFORM_LEVEL "${ANDROID_PLATFORM}")
if(SONIC_ANDROID_PLATFORM_LEVEL GREATER SONIC_ANDROID_MIN_SDK)
	set(SONIC_ANDROID_MIN_SDK ${SONIC_ANDROID_PLATFORM_LEVEL})
endif()
set(SEGASONIC_ANDROID_ICON "" CACHE FILEPATH
	"The launcher's icon: a square picture of the player's own (JPEG or PNG), instead of the trackball")
set(SEGASONIC_ANDROID_KEYSTORE "" CACHE FILEPATH "The key the APK is signed with (by default the debug one)")

# SDL2 from its source, as the releases' (FETCHCONTENT_SOURCE_DIR_SDL2 picks a copy already here): its
# library, libSDL2.so, and its Java
include(FetchContent)
FetchContent_Declare(SDL2 GIT_REPOSITORY https://github.com/libsdl-org/SDL.git GIT_TAG release-2.32.10
	GIT_SHALLOW TRUE)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL2)

add_library(main SHARED ports/sdl2/main.c ports/sdl2/settings.c ports/sdl2/actions.c ports/sdl2/letters.c
	ports/sdl2/touch.c)
target_link_libraries(main PRIVATE sonic_game_console sonic_data SDL2::SDL2)
target_compile_definitions(main PRIVATE _BSD_SOURCE)    # (bionic's funopen: the pack in the APK, main.c)

# The SDK around the NDK (sdk/ndk/<version>), else ANDROID_HOME: the platform's android.jar, the newest
# build-tools
if(DEFINED ENV{ANDROID_HOME})
	set(SONIC_ANDROID_SDK $ENV{ANDROID_HOME})
else()
	get_filename_component(SONIC_ANDROID_SDK ${ANDROID_NDK}/../.. ABSOLUTE)
endif()
set(SONIC_ANDROID_JAR ${SONIC_ANDROID_SDK}/platforms/android-${SONIC_ANDROID_TARGET_SDK}/android.jar)
if(NOT EXISTS ${SONIC_ANDROID_JAR})
	message(FATAL_ERROR "No android.jar of Android ${SONIC_ANDROID_TARGET_SDK} in the SDK (${SONIC_ANDROID_SDK})")
endif()
file(GLOB SONIC_BUILD_TOOLS LIST_DIRECTORIES true ${SONIC_ANDROID_SDK}/build-tools/*)
list(SORT SONIC_BUILD_TOOLS COMPARE NATURAL)
list(POP_BACK SONIC_BUILD_TOOLS SONIC_BUILD_TOOLS_DIR)
foreach(tool IN ITEMS aapt2 aapt zipalign d8 apksigner)
	string(TOUPPER ${tool} name)
	find_program(SONIC_${name} NAMES ${tool} ${tool}.bat HINTS ${SONIC_BUILD_TOOLS_DIR} NO_DEFAULT_PATH REQUIRED)
endforeach()

# Java 17 (JAVA_HOME): javac and jar for SDL's Java, keytool for the debug key; d8 and apksigner run with it
find_package(Java 17 COMPONENTS Development REQUIRED)
get_filename_component(SONIC_JAVA_BIN ${Java_JAVAC_EXECUTABLE} REALPATH)
get_filename_component(SONIC_JAVA_BIN ${SONIC_JAVA_BIN} DIRECTORY)
get_filename_component(SONIC_JAVA_HOME ${SONIC_JAVA_BIN} DIRECTORY)
find_program(SONIC_KEYTOOL keytool HINTS ${SONIC_JAVA_BIN} REQUIRED)
set(SONIC_WITH_JAVA ${CMAKE_COMMAND} -E env JAVA_HOME=${SONIC_JAVA_HOME})

set(SONIC_APK_WORK ${CMAKE_BINARY_DIR}/apk)
set(SONIC_APK ${SONIC_BIN_DIR}/android/SegaSonic-debug.apk)    # (not SegaSonic APK's SegaSonic.apk)

# SDL's Java, the activity that starts libmain.so: classes.dex
file(GLOB SONIC_SDL_JAVA ${sdl2_SOURCE_DIR}/android-project/app/src/main/java/org/libsdl/app/*.java)
add_custom_command(OUTPUT ${SONIC_APK_WORK}/dex/classes.dex
	COMMAND ${CMAKE_COMMAND} -E rm -rf ${SONIC_APK_WORK}/classes ${SONIC_APK_WORK}/dex
	COMMAND ${CMAKE_COMMAND} -E make_directory ${SONIC_APK_WORK}/classes ${SONIC_APK_WORK}/dex
	COMMAND ${Java_JAVAC_EXECUTABLE} -source 8 -target 8 -Xlint:-options -nowarn -encoding UTF-8
		-bootclasspath ${SONIC_ANDROID_JAR} -classpath ${SONIC_BUILD_TOOLS_DIR}/core-lambda-stubs.jar
		-d ${SONIC_APK_WORK}/classes ${SONIC_SDL_JAVA}    # (the stubs: the lambdas' LambdaMetafactory)
	COMMAND ${Java_JAR_EXECUTABLE} cf ${SONIC_APK_WORK}/classes.jar -C ${SONIC_APK_WORK}/classes .
	COMMAND ${SONIC_WITH_JAVA} ${SONIC_D8} --release --min-api ${SONIC_ANDROID_MIN_SDK} --lib ${SONIC_ANDROID_JAR}
		--output ${SONIC_APK_WORK}/dex ${SONIC_APK_WORK}/classes.jar
	DEPENDS ${SONIC_SDL_JAVA}
	COMMENT "SDL's Java: classes.dex"
	VERBATIM)


# The game's files in every APK: the libraries (without their symbols) and the Java, named as the APK has them
set(SONIC_APK_FILES ${SONIC_APK_WORK}/files)
set(SONIC_APK_LIB lib/${ANDROID_ABI})
add_custom_command(OUTPUT ${SONIC_APK_FILES}/classes.dex ${SONIC_APK_FILES}/${SONIC_APK_LIB}/libmain.so
		${SONIC_APK_FILES}/${SONIC_APK_LIB}/libSDL2.so
	COMMAND ${CMAKE_COMMAND} -E rm -rf ${SONIC_APK_FILES}
	COMMAND ${CMAKE_COMMAND} -E make_directory ${SONIC_APK_FILES}/${SONIC_APK_LIB}
	COMMAND ${CMAKE_STRIP} --strip-unneeded -o ${SONIC_APK_FILES}/${SONIC_APK_LIB}/libmain.so $<TARGET_FILE:main>
	COMMAND ${CMAKE_STRIP} --strip-unneeded -o ${SONIC_APK_FILES}/${SONIC_APK_LIB}/libSDL2.so
		$<TARGET_FILE:SDL2::SDL2>
	COMMAND ${CMAKE_COMMAND} -E copy ${SONIC_APK_WORK}/dex/classes.dex ${SONIC_APK_FILES}
	DEPENDS main SDL2 ${SONIC_APK_WORK}/dex/classes.dex
	COMMENT "The APK's libraries and Java"
	VERBATIM)
set(SONIC_APK_GAME_FILES ${SONIC_APK_FILES}/classes.dex ${SONIC_APK_FILES}/${SONIC_APK_LIB}/libmain.so
	${SONIC_APK_FILES}/${SONIC_APK_LIB}/libSDL2.so)

# An APK's manifest, icon and assets, as aapt2 links them from the folder 'stage' (its res/ and assets/,
# which 'commands' fill), then the game's files in and aligned: 'apk'
function(sonic_apk apk stage commands depends)
	add_custom_command(OUTPUT ${apk}
		COMMAND ${CMAKE_COMMAND} -E rm -rf ${stage}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${stage}/assets
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/ports/android/res ${stage}/res
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/ports/sdl2/font/font.bmp ${stage}/assets
		${commands}
		COMMAND ${SONIC_AAPT2} compile --dir ${stage}/res -o ${stage}/res.zip
		COMMAND ${SONIC_AAPT2} link -o ${stage}/linked.apk -I ${SONIC_ANDROID_JAR}
			--manifest ${CMAKE_SOURCE_DIR}/ports/android/AndroidManifest.xml
			--min-sdk-version ${SONIC_ANDROID_MIN_SDK} --target-sdk-version ${SONIC_ANDROID_TARGET_SDK}
			--version-code ${SONIC_ANDROID_VERSION_CODE} --version-name ${SONIC_ANDROID_VERSION}
			-A ${stage}/assets -0 assets ${stage}/res.zip    # (the pack stored: read in place, never unpacked)
		COMMAND ${CMAKE_COMMAND} -E chdir ${SONIC_APK_FILES} ${SONIC_AAPT} add ${stage}/linked.apk classes.dex
			${SONIC_APK_LIB}/libmain.so ${SONIC_APK_LIB}/libSDL2.so
		COMMAND ${SONIC_ZIPALIGN} -p -f 4 ${stage}/linked.apk ${apk}
		DEPENDS ${CMAKE_SOURCE_DIR}/ports/android/AndroidManifest.xml ${SONIC_ANDROID_RES}
			${CMAKE_SOURCE_DIR}/ports/sdl2/font/font.bmp ${SONIC_APK_GAME_FILES} ${depends}
		COMMENT "The APK: ${apk}"
		VERBATIM)
endfunction()

file(GLOB_RECURSE SONIC_ANDROID_RES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/ports/android/res/*)

# The game's Android build for SegaSonic APK (ports/android/apk), which puts the player's assets pack in it
# (and their icon, ic_launcher_own.xml and the picture in ic_launcher_art's place), and signs it: no pack, no
# signature
set(SONIC_APK_BASE ${SONIC_BIN_DIR}/android/segasonic-base.apk)
sonic_apk(${SONIC_APK_BASE} ${SONIC_APK_WORK}/base "" "")

# This computer's own APK: its assets pack, its icon (SEGASONIC_ANDROID_ICON, as SegaSonic APK puts it) and
# the debug key of its Android tools (made once)
set(SONIC_ANDROID_PACK ${SONIC_BIN_DIR}/segasonic.assets)
set(SONIC_APK_STAGE ${SONIC_APK_WORK}/player)
set(SONIC_APK_EXTRA "")
set(SONIC_APK_EXTRA_FILES "")
if(EXISTS ${SONIC_ANDROID_PACK})
	list(APPEND SONIC_APK_EXTRA COMMAND ${CMAKE_COMMAND} -E copy ${SONIC_ANDROID_PACK} ${SONIC_APK_STAGE}/assets)
	list(APPEND SONIC_APK_EXTRA_FILES ${SONIC_ANDROID_PACK})
else()
	message(STATUS "No assets pack at ${SONIC_ANDROID_PACK}: this computer's APK gets the game alone")
endif()
if(SEGASONIC_ANDROID_ICON)
	get_filename_component(SONIC_ICON_TYPE ${SEGASONIC_ANDROID_ICON} LAST_EXT)
	string(TOLOWER ${SONIC_ICON_TYPE} SONIC_ICON_TYPE)
	set(SONIC_APK_MIPMAP ${SONIC_APK_STAGE}/res/mipmap-xxxhdpi)
	list(APPEND SONIC_APK_EXTRA
		COMMAND ${CMAKE_COMMAND} -E rm -f ${SONIC_APK_MIPMAP}/ic_launcher.png ${SONIC_APK_MIPMAP}/ic_launcher_art.png
		COMMAND ${CMAKE_COMMAND} -E copy ${SEGASONIC_ANDROID_ICON} ${SONIC_APK_MIPMAP}/ic_launcher${SONIC_ICON_TYPE}
		COMMAND ${CMAKE_COMMAND} -E copy ${SEGASONIC_ANDROID_ICON} ${SONIC_APK_MIPMAP}/ic_launcher_art${SONIC_ICON_TYPE}
		COMMAND ${CMAKE_COMMAND} -E copy ${SONIC_APK_STAGE}/res/mipmap-anydpi-v26/ic_launcher_own.xml
			${SONIC_APK_STAGE}/res/mipmap-anydpi-v26/ic_launcher.xml)
	list(APPEND SONIC_APK_EXTRA_FILES ${SEGASONIC_ANDROID_ICON})
endif()
sonic_apk(${SONIC_APK_WORK}/player.apk ${SONIC_APK_STAGE} "${SONIC_APK_EXTRA}" "${SONIC_APK_EXTRA_FILES}")

if(SEGASONIC_ANDROID_KEYSTORE)
	set(SONIC_KEYSTORE ${SEGASONIC_ANDROID_KEYSTORE})
elseif(CMAKE_HOST_WIN32)
	file(TO_CMAKE_PATH "$ENV{USERPROFILE}/.android/debug.keystore" SONIC_KEYSTORE)
else()
	set(SONIC_KEYSTORE $ENV{HOME}/.android/debug.keystore)
endif()
get_filename_component(SONIC_KEYSTORE_DIR ${SONIC_KEYSTORE} DIRECTORY)
add_custom_command(OUTPUT ${SONIC_KEYSTORE}
	COMMAND ${CMAKE_COMMAND} -E make_directory ${SONIC_KEYSTORE_DIR}
	COMMAND ${SONIC_KEYTOOL} -genkeypair -keystore ${SONIC_KEYSTORE} -storepass android -alias androiddebugkey
		-keypass android -keyalg RSA -keysize 2048 -validity 10000 -dname "CN=Android Debug,O=Android,C=US"
	COMMENT "The debug key: ${SONIC_KEYSTORE}"
	VERBATIM)
add_custom_command(OUTPUT ${SONIC_APK}
	COMMAND ${SONIC_WITH_JAVA} ${SONIC_APKSIGNER} sign --ks ${SONIC_KEYSTORE} --ks-pass pass:android
		--ks-key-alias androiddebugkey --key-pass pass:android --out ${SONIC_APK} ${SONIC_APK_WORK}/player.apk
	DEPENDS ${SONIC_APK_WORK}/player.apk ${SONIC_KEYSTORE}
	COMMENT "This computer's APK, signed: ${SONIC_APK}"
	VERBATIM)
add_custom_target(segasonic_apk ALL DEPENDS ${SONIC_APK_BASE} ${SONIC_APK})

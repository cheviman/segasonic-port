/**
 *    Name:        sdl2_layer.h
 *    Description: the SDL 3 names the window port uses, over SDL 2: for a device whose system has SDL 2 alone
 *    Version:     0.1.0
 *    Created:     27-11-2026
 *    Modified:    27-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *
 *    The window port (main.c, actions.c) is written for SDL 3. Anbernic's H700 handhelds run their stock firmware's
 *    own SDL 2.0.12, which draws through its Mali GPU and which no SDL 3 stands in for (sdl2-compat goes the other
 *    way: SDL 2's names over SDL 3). So their build (SEGASONIC_SDL2_LAYER, ports/anbernic/anbernic.cmake) takes SDL 3's
 *    names from here, each one over the SDL 2 function that did the same before the port moved: nothing newer than
 *    SDL 2.0.12 is called (the build's last step, ports/anbernic/check_sdl.py).
 *
 *    Only what a handheld's build compiles is here (SEGASONIC_HANDHELD: no first run's window, no touch screen, no mouse,
 *    no VSYNC): a name the port starts to use there is added, and the compiler says which.
 */
#ifndef SEGASONIC_PORT_SDL2_LAYER_H
#define SEGASONIC_PORT_SDL2_LAYER_H

#include <SDL.h>

#include <stdbool.h>
#include <stdint.h>

/* ---- the start ---- */

#define SDL_INIT_GAMEPAD            SDL_INIT_GAMECONTROLLER
#define SDL_Init(flags)             (SDL_Init(flags) == 0)             /* (SDL 3's say true where it went well) */
#define SDL_InitSubSystem(flags)    (SDL_InitSubSystem(flags) == 0)

/* The SDL the game runs with (the device's own), as SDL 3's one number */
static inline int layer_version(void)
{
	SDL_version version;

	SDL_GetVersion(&version);
	return version.major * 1000000 + version.minor * 1000 + version.patch;
}
#define SDL_GetVersion()                layer_version()
#define SDL_VERSIONNUM_MAJOR(version)   ((version) / 1000000)
#define SDL_VERSIONNUM_MINOR(version)   (((version) / 1000) % 1000)
#define SDL_VERSIONNUM_MICRO(version)   ((version) % 1000)

/* The executable's folder: SDL 3's own text, never freed. SDL 2's is the caller's: one kept for the run */
static inline const char *layer_base_path(void)
{
	static char *base;

	if (!base)
		base = SDL_GetBasePath();
	return base;
}
#define SDL_GetBasePath()           layer_base_path()

/* ---- the window and the renderer ---- */

/* (a window's place is SDL 3's to choose: the screen's corner, as the handheld's whole screen was asked for) */
#define SDL_CreateWindow(title, width, height, flags)  SDL_CreateWindow(title, 0, 0, width, height, flags)

/* SDL 3's full screen with no mode chosen is the desktop's own: SDL 2's SDL_WINDOW_FULLSCREEN_DESKTOP */
#define SDL_WINDOW_FULLSCREEN       SDL_WINDOW_FULLSCREEN_DESKTOP
static inline bool layer_set_full_screen(SDL_Window *window, bool full)
{
	return SDL_SetWindowFullscreen(window, full ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) == 0;
}
#define SDL_SetWindowFullscreen(window, full)  layer_set_full_screen(window, full)

/* The first renderer that starts (SDL 3's, by no name): the GPU's with render targets, else SDL's in software */
static inline SDL_Renderer *layer_create_renderer(SDL_Window *window)
{
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);

	return renderer ? renderer : SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
}
#define SDL_CreateRenderer(window, name)  layer_create_renderer(window)

static inline const char *layer_renderer_name(SDL_Renderer *renderer)
{
	SDL_RendererInfo info;

	return SDL_GetRendererInfo(renderer, &info) == 0 ? info.name : "";
}
#define SDL_GetRendererName(renderer)  layer_renderer_name(renderer)

static inline bool layer_output_size(SDL_Renderer *renderer, int *width, int *height)
{
	return SDL_GetRendererOutputSize(renderer, width, height) == 0;
}
#define SDL_GetRenderOutputSize(renderer, width, height)  layer_output_size(renderer, width, height)

/* ---- drawing: SDL 3 takes its rectangles as floats, and the port's are all whole pixels ---- */

#define SDL_SCALEMODE_NEAREST       SDL_ScaleModeNearest
#define SDL_SCALEMODE_LINEAR        SDL_ScaleModeLinear

static inline void layer_rect_to_frect(const SDL_Rect *rect, SDL_FRect *frect)
{
	frect->x = (float)rect->x;
	frect->y = (float)rect->y;
	frect->w = (float)rect->w;
	frect->h = (float)rect->h;
}
#define SDL_RectToFRect(rect, frect)  layer_rect_to_frect(rect, frect)

static inline SDL_Rect layer_whole_rect(const SDL_FRect *frect)
{
	const SDL_Rect rect = { (int)frect->x, (int)frect->y, (int)frect->w, (int)frect->h };

	return rect;
}

static inline bool layer_render_texture(SDL_Renderer *renderer, SDL_Texture *texture, const SDL_FRect *source,
	const SDL_FRect *destination)
{
	SDL_Rect from, to;

	if (source)
		from = layer_whole_rect(source);
	if (destination)
		to = layer_whole_rect(destination);
	return SDL_RenderCopy(renderer, texture, source ? &from : NULL, destination ? &to : NULL) == 0;
}
#define SDL_RenderTexture(renderer, texture, source, destination) \
	layer_render_texture(renderer, texture, source, destination)

static inline bool layer_fill_rects(SDL_Renderer *renderer, const SDL_FRect *rects, int count)
{
	bool drawn = true;

	for (int index = 0; index < count; index++)
	{
		const SDL_Rect rect = layer_whole_rect(&rects[index]);

		drawn = SDL_RenderFillRect(renderer, &rect) == 0 && drawn;
	}
	return drawn;
}
#define SDL_RenderFillRects(renderer, rects, count)  layer_fill_rects(renderer, rects, count)

#define SDL_LockTexture(texture, rect, pixels, pitch)  (SDL_LockTexture(texture, rect, pixels, pitch) == 0)

/* ---- the screen and the window's icon (segasonic: the window's scale, the console's icon) ---- */

typedef int SDL_DisplayID;    /* (SDL 2's display index: 0 is the first, which is also SDL 3's "none" here) */
#define SDL_GetDisplayForWindow(window)        SDL_GetWindowDisplayIndex(window)
#define SDL_GetPrimaryDisplay()                0
#define SDL_GetDisplayUsableBounds(display, rect)  (SDL_GetDisplayUsableBounds(display, rect) == 0)
#define SDL_CreateSurfaceFrom(width, height, format, pixels, pitch) \
	SDL_CreateRGBSurfaceWithFormatFrom(pixels, width, height, SDL_BITSPERPIXEL(format), pitch, format)
#define SDL_GetNumLogicalCPUCores()            SDL_GetCPUCount()

#define SDL_ConvertSurface(surface, format)  SDL_ConvertSurfaceFormat(surface, format, 0)
#define SDL_DestroySurface(surface)          SDL_FreeSurface(surface)

/* ---- the sound: SDL 3's stream on the default device is SDL 2's device and its queue ---- */

#define SDL_AUDIO_S16                         AUDIO_S16SYS
#define SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK     0
#define SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES   "SDL_AUDIO_DEVICE_SAMPLE_FRAMES"
#define LAYER_AUDIO_SAMPLES                   1024    /* (the device's buffer: what the port asked SDL 2 for) */
#define LAYER_AUDIO_DEVICE(stream)            ((SDL_AudioDeviceID)(uintptr_t)(stream))

/* (the stream is the device's number: SDL 2's own SDL_AudioStream is never made) */
static inline SDL_AudioStream *layer_open_audio(const SDL_AudioSpec *spec)
{
	SDL_AudioSpec wanted = *spec, got;

	wanted.samples = LAYER_AUDIO_SAMPLES;
	return (SDL_AudioStream *)(uintptr_t)SDL_OpenAudioDevice(NULL, 0, &wanted, &got, 0);
}
#define SDL_OpenAudioDeviceStream(device, spec, callback, userdata)  layer_open_audio(spec)
#define SDL_ResumeAudioStreamDevice(stream)   SDL_PauseAudioDevice(LAYER_AUDIO_DEVICE(stream), 0)
#define SDL_PauseAudioStreamDevice(stream)    SDL_PauseAudioDevice(LAYER_AUDIO_DEVICE(stream), 1)
#define SDL_GetAudioStreamQueued(stream)      ((int)SDL_GetQueuedAudioSize(LAYER_AUDIO_DEVICE(stream)))
static inline bool layer_put_audio(SDL_AudioStream *stream, const void *data, int length)
{
	return SDL_QueueAudio(LAYER_AUDIO_DEVICE(stream), data, (Uint32)length) == 0;
}
#define SDL_PutAudioStreamData(stream, data, length)  layer_put_audio(stream, data, length)
#define SDL_ClearAudioStream(stream)          (SDL_ClearQueuedAudio(LAYER_AUDIO_DEVICE(stream)), true)
#define SDL_DestroyAudioStream(stream)        SDL_CloseAudioDevice(LAYER_AUDIO_DEVICE(stream))

/* ---- the keyboard and the game controllers ---- */

#define SDL_KMOD_ALT                KMOD_ALT
#define SDL_GetKeyboardState(count) ((const bool *)SDL_GetKeyboardState(count))    /* (SDL 2's bytes, 0 or 1) */

typedef SDL_GameController          SDL_Gamepad;
typedef SDL_GameControllerButton    SDL_GamepadButton;
typedef SDL_GameControllerAxis      SDL_GamepadAxis;

#define SDL_GAMEPAD_BUTTON_INVALID          SDL_CONTROLLER_BUTTON_INVALID
#define SDL_GAMEPAD_BUTTON_SOUTH            SDL_CONTROLLER_BUTTON_A
#define SDL_GAMEPAD_BUTTON_EAST             SDL_CONTROLLER_BUTTON_B
#define SDL_GAMEPAD_BUTTON_WEST             SDL_CONTROLLER_BUTTON_X
#define SDL_GAMEPAD_BUTTON_NORTH            SDL_CONTROLLER_BUTTON_Y
#define SDL_GAMEPAD_BUTTON_BACK             SDL_CONTROLLER_BUTTON_BACK
#define SDL_GAMEPAD_BUTTON_GUIDE            SDL_CONTROLLER_BUTTON_GUIDE
#define SDL_GAMEPAD_BUTTON_START            SDL_CONTROLLER_BUTTON_START
#define SDL_GAMEPAD_BUTTON_LEFT_STICK       SDL_CONTROLLER_BUTTON_LEFTSTICK
#define SDL_GAMEPAD_BUTTON_RIGHT_STICK      SDL_CONTROLLER_BUTTON_RIGHTSTICK
#define SDL_GAMEPAD_BUTTON_LEFT_SHOULDER    SDL_CONTROLLER_BUTTON_LEFTSHOULDER
#define SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER   SDL_CONTROLLER_BUTTON_RIGHTSHOULDER
#define SDL_GAMEPAD_BUTTON_DPAD_UP          SDL_CONTROLLER_BUTTON_DPAD_UP
#define SDL_GAMEPAD_BUTTON_DPAD_DOWN        SDL_CONTROLLER_BUTTON_DPAD_DOWN
#define SDL_GAMEPAD_BUTTON_DPAD_LEFT        SDL_CONTROLLER_BUTTON_DPAD_LEFT
#define SDL_GAMEPAD_BUTTON_DPAD_RIGHT       SDL_CONTROLLER_BUTTON_DPAD_RIGHT
#define SDL_GAMEPAD_BUTTON_MISC1            SDL_CONTROLLER_BUTTON_MISC1
#define SDL_GAMEPAD_BUTTON_COUNT            SDL_CONTROLLER_BUTTON_MAX
#define SDL_GAMEPAD_AXIS_INVALID            SDL_CONTROLLER_AXIS_INVALID
#define SDL_GAMEPAD_AXIS_LEFTX              SDL_CONTROLLER_AXIS_LEFTX
#define SDL_GAMEPAD_AXIS_LEFTY              SDL_CONTROLLER_AXIS_LEFTY
#define SDL_GAMEPAD_AXIS_LEFT_TRIGGER       SDL_CONTROLLER_AXIS_TRIGGERLEFT
#define SDL_GAMEPAD_AXIS_RIGHT_TRIGGER      SDL_CONTROLLER_AXIS_TRIGGERRIGHT

/* (SDL 3 names a joystick by its instance always. SDL 2's events that say one was plugged in carry its place in the
   list instead, which is what SDL 2's functions for a joystick not yet opened take: the port hands each the event's
   own number) */
#define SDL_OpenGamepad(which)                      SDL_GameControllerOpen(which)
#define SDL_CloseGamepad(pad)                       SDL_GameControllerClose(pad)
#define SDL_IsGamepad(which)                        (SDL_IsGameController(which) != SDL_FALSE)
#define SDL_GetGamepadName(pad)                     SDL_GameControllerName(pad)
#define SDL_GetGamepadJoystick(pad)                 SDL_GameControllerGetJoystick(pad)
#define SDL_GetJoystickID(joystick)                 SDL_JoystickInstanceID(joystick)
#define SDL_GetGamepadAxis(pad, axis)               SDL_GameControllerGetAxis(pad, axis)
#define SDL_GetGamepadButton(pad, button)           (SDL_GameControllerGetButton(pad, button) != 0)
#define SDL_GetGamepadMappingForGUID(guid)          SDL_GameControllerMappingForGUID(guid)
#define SDL_GetGamepadStringForButton(button)       SDL_GameControllerGetStringForButton(button)
#define SDL_GetGamepadButtonFromString(text)        SDL_GameControllerGetButtonFromString(text)
#define SDL_GetGamepadStringForAxis(axis)           SDL_GameControllerGetStringForAxis(axis)
#define SDL_GetGamepadAxisFromString(text)          SDL_GameControllerGetAxisFromString(text)
#define SDL_GetJoystickGUIDForID(which)             SDL_JoystickGetDeviceGUID(which)
#define SDL_GetJoystickNameForID(which)             SDL_JoystickNameForIndex(which)
#define SDL_GUIDToString(guid, text, size)          SDL_JoystickGetGUIDString(guid, text, size)

/* The mappings SDL knows: the port asks for them to count them (SDL 3 hands a list, which it frees) */
static inline char **layer_gamepad_mappings(int *count)
{
	*count = SDL_GameControllerNumMappings();
	return NULL;
}
#define SDL_GetGamepadMappings(count)  layer_gamepad_mappings(count)

/* ---- the events: SDL 3's, with the fields the port reads, from SDL 2's ---- */

#define SDL_EVENT_QUIT                    SDL_QUIT
#define SDL_EVENT_KEY_DOWN                SDL_KEYDOWN
#define SDL_EVENT_KEY_UP                  SDL_KEYUP
#define SDL_EVENT_JOYSTICK_BUTTON_DOWN    SDL_JOYBUTTONDOWN
#define SDL_EVENT_JOYSTICK_ADDED          SDL_JOYDEVICEADDED
#define SDL_EVENT_GAMEPAD_BUTTON_DOWN     SDL_CONTROLLERBUTTONDOWN
#define SDL_EVENT_GAMEPAD_AXIS_MOTION     SDL_CONTROLLERAXISMOTION
#define SDL_EVENT_GAMEPAD_ADDED           SDL_CONTROLLERDEVICEADDED
#define SDL_EVENT_GAMEPAD_REMOVED         SDL_CONTROLLERDEVICEREMOVED
#define SDL_EVENT_MOUSE_MOTION            SDL_MOUSEMOTION         /* (segasonic's trackball: a handheld has no mouse, */
#define SDL_EVENT_MOUSE_BUTTON_DOWN       SDL_MOUSEBUTTONDOWN     /* but the console's code for one is built there) */
#define SDL_EVENT_MOUSE_BUTTON_UP         SDL_MOUSEBUTTONUP
#define SDL_EVENT_WINDOW_FIRST            SDL_WINDOWEVENT         /* (SDL 2's window events are one type) */
#define SDL_EVENT_WINDOW_LAST             SDL_WINDOWEVENT
#define SDL_SetWindowRelativeMouseMode(window, on)  ((void)(window), SDL_SetRelativeMouseMode((on) ? SDL_TRUE : SDL_FALSE) == 0)

typedef union
{
	Uint32 type;
	struct
	{
		Uint32 type;
		SDL_Scancode scancode;
		SDL_Keycode key;
		Uint16 mod;
		bool repeat;
	} key;
	SDL_JoyButtonEvent jbutton;           /* (the fields the port reads are SDL 2's own in these) */
	SDL_JoyDeviceEvent jdevice;
	SDL_ControllerButtonEvent gbutton;
	SDL_ControllerAxisEvent gaxis;
	SDL_ControllerDeviceEvent gdevice;
	struct
	{
		Uint32 type;
		float xrel, yrel;
	} motion;
} layer_event;

static inline bool layer_poll_event(layer_event *event)
{
	SDL_Event from;

	if (!SDL_PollEvent(&from))
		return false;
	SDL_zerop(event);    /* (an event the port does not read: its type alone) */
	switch (from.type)
	{
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			event->key.scancode = from.key.keysym.scancode;
			event->key.key = from.key.keysym.sym;
			event->key.mod = from.key.keysym.mod;
			event->key.repeat = from.key.repeat != 0;
			break;
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			event->jbutton = from.jbutton;
			break;
		case SDL_JOYDEVICEADDED:
		case SDL_JOYDEVICEREMOVED:
			event->jdevice = from.jdevice;
			break;
		case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERBUTTONUP:
			event->gbutton = from.cbutton;
			break;
		case SDL_CONTROLLERAXISMOTION:
			event->gaxis = from.caxis;
			break;
		case SDL_CONTROLLERDEVICEADDED:
		case SDL_CONTROLLERDEVICEREMOVED:
			event->gdevice = from.cdevice;
			break;
		case SDL_MOUSEMOTION:
			event->motion.xrel = (float)from.motion.xrel;
			event->motion.yrel = (float)from.motion.yrel;
			break;
		default:
			break;
	}
	event->type = from.type;
	return true;
}
#define SDL_Event                   layer_event
#define SDL_PollEvent(event)        layer_poll_event(event)

#endif

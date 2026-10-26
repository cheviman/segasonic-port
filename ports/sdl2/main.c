/**
 *    Name:        main.c
 *    Description: the game in a window: SDL2 shows its frames, plays its sound, reads the controls and keeps
 *                 the pace, around the same game and board the headless runs check
 *    Version:     0.1.0
 *    Created:     13-10-2026
 *    Modified:    25-10-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    Usage: sonic
 *
 *    The executable carries nothing of the game's: it loads the game's assets from segasonic.assets beside
 *    it (game/data.h; the pack of the assets taken out of the player's ROM set), with SDL2 beside it too.
 *    What it writes goes beside it on Windows, elsewhere to the user's folder for the game (saves_folder).
 *    With no pack there the first run makes one from the player's ROM set (first_run.c). On the Nintendo
 *    Switch (SegaSonic.nro, SEGASONIC_HANDHELD) what comes with the game is in the .nro's romfs and what it
 *    writes, the pack too, in /switch/segasonic/ on the SD card; the picture is fixed (fix_settings). On
 *    Android (libmain.so in the APK, SEGASONIC_HANDHELD and SEGASONIC_TOUCH) the pack and the letters come
 *    in the APK's assets, what it writes goes to the app's own folder and the touch screen has its
 *    controls (touch.c).
 *
 *    The game (src/) and the board (runtime/) run as they are: this file only wraps them. Where the board
 *    has its hooks, the port does its part: when VBLANK starts the video draws the frame and the port puts
 *    it on the screen; when the board reads the controls the port hands it the keyboard, the mouse and the
 *    game controllers; when the game finishes a frame the port takes the frame's samples, mixes them to
 *    48 kHz and queues them, and waits for the frame's time (60 a second) to pass. The EEPROM (the game's
 *    settings, bookkeeping) is kept in sonic.eeprom; the window's own settings (mouse, keys, picture,
 *    volume) in sonic.ini (settings.c), which the game reads again whenever it is saved.
 *
 *    Controls (player 1; MAME's keys where it has them):
 *      mouse, or the arrow keys       trackball (the mouse's ball rolls on when the mouse stops;
 *                                     the right button holds it)
 *      left button, Z or space        the button
 *      5, 6, 7                        coins 1-3        1, 2, 3   starts 1-3 (Enter: start 1)
 *      9                              service          F2        test (the operator's menu: 9 moves
 *                                                                  its cursor, F2 picks)
 *      Escape                         quit (the console: pause in a game, which frees the mouse to be the
 *                                     pointer; the menu closes on the title)
 *                                     F11 or Alt+Enter   full screen
 *      F3                             smooth or sharp pixels
 *      F4                             the mouse's ball rolls on, or stops with the mouse
 *      F5, F6                         the mouse slower, faster (the window's title says how fast)
 *      F7, F8                         (the console build) debug: the stage's section before, after
 *    Game controllers, one per player: the left stick is the trackball, A the button, Start the start,
 *    Back a coin.
 */
#include "s32/machine.h"
#include "s32/program.h"
#include "s32/video.h"
#include "s32/workers.h"
#include "game/data.h"
#include "first_run.h"
#include "sound/chips.h"

#include "settings.h"
#ifdef SEGASONIC_TOUCH
#include "touch.h"
#endif
#ifdef SEGASONIC_CONSOLE
#include "console.h"
#include "actions.h"
#define FILE_NAME "segasonic"    /* the console build: SegaSonic.exe, segasonic.ini, segasonic.eeprom */
#define WINDOW_TITLE "SegaSonic The Hedgehog"
#else
#define FILE_NAME "sonic"        /* the arcade build: SegaSonic Arcade.exe, sonic.ini, sonic.eeprom */
#define WINDOW_TITLE "SegaSonic Arcade"
#endif
#define ASSETS_FILE "segasonic.assets"    /* the game's assets pack (tools/make_pack.py) */

#ifdef SEGASONIC_CONSOLE
static bool pause_requested;
static bool bound_now;              /* CONTROLS took a key or button this frame */
static void run_pause(void);
/* The mouse is player 1's trackball (held by the window, its pointer hidden) only from a click in the
   window, until a pause: who plays with keys or a game controller keeps the pointer. The click that takes
   it is not the button */
static bool mouse_held;
static bool mouse_click_taken;      /* that click, until its buttons are let go */
#endif

#include <SDL.h>
#ifdef _WIN32
#include "mice.h"
#define SEPARATE_MICE 1    /* (each mouse is a player's trackball: mice.h) */
#endif

#ifdef __SWITCH__
#include <switch.h>        /* romfsInit: the .nro's own files */
#include <sys/stat.h>
#define SWITCH_FOLDER "sdmc:/switch/segasonic"    /* the game's folder on the SD card */
#endif
#if defined(SEGASONIC_HANDHELD) && !defined(_WIN32) && !defined(__SWITCH__)
#include <unistd.h>        /* fsync: the log on the card as it is written */
#endif
#ifdef SEGASONIC_HANDHELD
static void log_written(void);
#ifdef SEGASONIC_HANDHELD_KEYS
#define HANDHELD_MENU_BUTTON 8    /* the joystick button that leaves the game: the RG40XX H's MENU */
static bool settings_unsaved;    /* the volume keys turned the volume */
static void handheld_event(const SDL_Event *event);
static void save_handheld_settings(void);
#endif
/* What pauses a stage and goes on from the pause: start; a handheld's B (back) does nothing there */
#define PAUSE_ACTIONS (ACTION_BIT(ACTION_START))
#else
#define PAUSE_ACTIONS (ACTION_BIT(ACTION_START) | ACTION_BIT(ACTION_BACK))
#endif

#include <math.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PLAYERS            3
#define OUTPUT_RATE        48000
#define QUEUE_FRAMES_MAX   6          /* audio queued beyond this many frames is dropped */
#define ASSETS_FILE        "segasonic.assets"    /* the game's assets pack (tools/make_pack.py) */

static const double FM_RATE = S32_SOUND_CLOCK / 144.0;
static const double PCM_RATE = S32_PCM_CLOCK / 384.0;

static jmp_buf finished;
static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static SDL_Texture *scaled;           /* smooth pixels: the picture a whole number of times its size */
static int scaled_times;
static SDL_AudioDeviceID audio;
static SDL_GameController *controllers[PLAYERS];
static double mouse_rest[PLAYERS][2];    /* the fractions of a count the mouse has moved and not handed on */
static double hand[PLAYERS][2];          /* counts the mouse has moved this frame */
static bool hand_moved[PLAYERS];
static double ball[PLAYERS][2];          /* counts a frame the ball turns */
#ifdef SEPARATE_MICE
static bool mice_listening;              /* Raw Input tells the mice apart (mice_open) */
#endif
static uint64_t next_frame;           /* performance counter value the next frame waits for */
static double shown_scale;            /* the window's scale as it was last set */
static int shown_width;               /* ... and its picture's width (picture_width) */
static char eeprom_path[1024];

/* What the controls say, gathered between frames */
static struct
{
	int roll_x[PLAYERS];
	int roll_y[PLAYERS];
	bool button[PLAYERS];
	bool coin[PLAYERS];
	bool start[PLAYERS];
	bool service;
	bool test;
} controls;

/* A chip's samples on their way to the output rate: what is left of the last frame and where the next
   output sample falls between two of them */
typedef struct resampler
{
	double step;            /* source samples an output sample */
	double position;        /* of the next output sample, from the first sample kept */
	int32_t *samples;       /* left and right */
	size_t count;           /* samples (pairs) kept */
	size_t capacity;
} resampler;

static resampler chips[3];

/* ---- files ---- */

static uint8_t *load_file(const char *path, size_t *size)
{
	FILE *input = fopen(path, "rb");
	uint8_t *data;
	long length;

	if (!input)
		return NULL;
	fseek(input, 0, SEEK_END);
	length = ftell(input);
	fseek(input, 0, SEEK_SET);
	data = length > 0 ? (uint8_t *)malloc((size_t)length) : NULL;
	if (data && fread(data, 1, (size_t)length, input) != (size_t)length)
	{
		free(data);
		data = NULL;
	}
	fclose(input);
	if (size)
		*size = data ? (size_t)length : 0;
	return data;
}

static void save_eeprom(void)
{
	FILE *output;

	if (!s32_board.eeprom.dirty)
		return;
	output = fopen(eeprom_path, "wb");
	if (!output)
		return;
	fwrite(s32_board.eeprom.data, 1, sizeof(s32_board.eeprom.data), output);
	fclose(output);
	s32_board.eeprom.dirty = false;
}

static void load_eeprom(void)
{
	size_t size;
	uint8_t *data = load_file(eeprom_path, &size);

	if (data && size == sizeof(s32_board.eeprom.data))
		memcpy(s32_board.eeprom.data, data, size);
	free(data);
}

/* ---- the picture ---- */

/* The frame in the window, as large as it fits with the game's proportions. Sharp pixels: the picture
   scaled straight up, each pixel a block. Smooth ones ("sharp bilinear"): first a whole number of times with
   sharp pixels into 'scaled', then that softened the rest of the way, so only the pixels' edges blend */
#define WIDE_WIDTH  398    /* 16:9 at 224 rows: the video's 416 less 9 at each side */

/* The window's picture, in the game's pixels: 320, or 16:9 with WIDE */
static int picture_width(void)
{
#ifdef SEGASONIC_CONSOLE
	if (settings.wide)
		return WIDE_WIDTH;
#endif
	return 320;
}

#ifndef __SWITCH__    /* (the Switch's window is its screen) */
#define WINDOW_EDGES 64    /* room left on the screen for the window's title bar and borders */

/* The window's scale: settings.window_scale, or the largest whole one under it at which the picture fits
   the screen (a 16:9 picture at a scale that does not fit would be cut down to 4:3 by the system) */
static double window_scale_that_fits(void)
{
	SDL_Rect usable;
	double scale = settings.window_scale;
	const int display = window ? SDL_GetWindowDisplayIndex(window) : 0;

	if (SDL_GetDisplayUsableBounds(display < 0 ? 0 : display, &usable) != 0)
		return scale;
	while (scale > 1.0 && (picture_width() * scale > usable.w - WINDOW_EDGES
		|| S32_VIDEO_HEIGHT * scale > usable.h - WINDOW_EDGES))
		scale -= 1.0;
	return scale;
}
#endif

/* ---- CRT: a multiplying overlay on smooth pixels, the size the picture is shown at ---- */

#define CRT_SCANLINE_DARK  0.45    /* how much the scanlines' edges darken (their middles keep it all) */
#define CRT_MASK           0.80    /* the other two channels in a phosphor column */
#define CRT_VIGNETTE       0.18    /* how much the corners darken */

static SDL_Texture *crt_overlay;
static int crt_width, crt_height;

/* For each pixel of the shown picture: its game row's scanline (bright in the middle, dark at the edges),
   the column's phosphor (red, green, blue in turn), and the vignette toward the edges */
static void make_crt_overlay(int width, int height)
{
	uint32_t *pixels;
	int pitch = 0;

	if (crt_overlay && crt_width == width && crt_height == height)
		return;
	if (crt_overlay)
		SDL_DestroyTexture(crt_overlay);
	crt_overlay = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
	crt_width = width;
	crt_height = height;
	if (!crt_overlay || SDL_LockTexture(crt_overlay, NULL, (void **)&pixels, &pitch) != 0)
		return;
	for (int y = 0; y < height; y++)
	{
		const double row = (y + 0.5) * S32_VIDEO_HEIGHT / height;
		const double scan = 1.0 - CRT_SCANLINE_DARK * (1.0 - sin(3.14159265358979 * (row - floor(row))));
		const double dy = (2.0 * y / height - 1.0);

		for (int x = 0; x < width; x++)
		{
			const double dx = (2.0 * x / width - 1.0);
			const double light = scan * (1.0 - CRT_VIGNETTE * (dx * dx * dy * dy));
			const int phosphor = x % 3;
			const int red = (int)(255.0 * light * (phosphor == 0 ? 1.0 : CRT_MASK));
			const int green = (int)(255.0 * light * (phosphor == 1 ? 1.0 : CRT_MASK));
			const int blue = (int)(255.0 * light * (phosphor == 2 ? 1.0 : CRT_MASK));

			pixels[y * (pitch / 4) + x] = 0xFF000000u | (uint32_t)red << 16 | (uint32_t)green << 8 | (uint32_t)blue;
		}
	}
	SDL_UnlockTexture(crt_overlay);
	SDL_SetTextureBlendMode(crt_overlay, SDL_BLENDMODE_MOD);
}

#ifdef SEGASONIC_PAUSE_EXIT
static SDL_Rect picture_shown;    /* where the last frame was on the screen */
#endif

static void present(const uint32_t *pixels, int width)
{
	SDL_Rect source = { 0, 0, width, S32_VIDEO_HEIGHT };
	SDL_Rect shown;
	int out_width = 0, out_height = 0;
	double scale;

	SDL_UpdateTexture(texture, NULL, pixels, S32_VIDEO_MAX_WIDTH * (int)sizeof(uint32_t));
	if (width > picture_width())
	{
		/* a 416-pixel frame in a narrower picture: its middle (16:9: 9 pixels off each side) */
		source.x = (width - picture_width()) / 2;
		source.w = width = picture_width();
	}
	SDL_GetRendererOutputSize(renderer, &out_width, &out_height);
	scale = SDL_min((double)out_width / width, (double)out_height / S32_VIDEO_HEIGHT);
	shown.w = (int)(width * scale);
	shown.h = (int)(S32_VIDEO_HEIGHT * scale);
	shown.x = (out_width - shown.w) / 2;
	shown.y = (out_height - shown.h) / 2;
	/* plain bilinear: the picture straight to the window, softened all the way */
	SDL_SetTextureScaleMode(texture, settings.bilinear && !settings.crt ? SDL_ScaleModeLinear : SDL_ScaleModeNearest);
	if (settings.bilinear && !settings.crt)
	{
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, &source, &shown);
	}
	else if (settings.smooth || settings.crt)
	{
		const int times = SDL_max(1, (int)scale);
		SDL_Rect whole = { 0, 0, width * times, S32_VIDEO_HEIGHT * times };

		if (!scaled || scaled_times != times)
		{
			if (scaled)
				SDL_DestroyTexture(scaled);
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
			scaled = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET,
				S32_VIDEO_MAX_WIDTH * times, S32_VIDEO_HEIGHT * times);
			SDL_SetTextureScaleMode(scaled, SDL_ScaleModeLinear);
			scaled_times = times;
		}
		SDL_SetRenderTarget(renderer, scaled);
		SDL_RenderCopy(renderer, texture, &source, &whole);
		SDL_SetRenderTarget(renderer, NULL);
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, scaled, &whole, &shown);
		if (settings.crt)
		{
			make_crt_overlay(shown.w, shown.h);
			if (crt_overlay)
				SDL_RenderCopy(renderer, crt_overlay, NULL, &shown);
		}
	}
	else
	{
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, &source, &shown);
	}
#ifdef SEGASONIC_TOUCH
	touch_draw(renderer);    /* (over the picture: the touch screen's controls) */
#endif
#ifdef SEGASONIC_PAUSE_EXIT
	picture_shown = shown;    /* (the pause's choices under a finger: pause_item_at) */
#endif
	SDL_RenderPresent(renderer);
}

#ifdef SEGASONIC_CONSOLE
/* ---- the CONTROLS screen's keys and buttons: Kenney's Input Prompts Pixel (CC0), 16 x 16 tiles ---- */

typedef struct
{
	int what;      /* an SDL_Scancode or an SDL_GameControllerButton */
	uint8_t row, column, tiles;    /* where in the sheet, and how many tiles wide */
} prompt;

static const prompt key_prompts[] = {
	{ SDL_SCANCODE_ESCAPE, 0, 17, 1 }, { SDL_SCANCODE_F1, 0, 18, 1 }, { SDL_SCANCODE_F2, 0, 19, 1 },
	{ SDL_SCANCODE_F3, 0, 20, 1 }, { SDL_SCANCODE_F4, 0, 21, 1 }, { SDL_SCANCODE_F5, 0, 22, 1 },
	{ SDL_SCANCODE_F6, 0, 23, 1 }, { SDL_SCANCODE_F7, 0, 24, 1 }, { SDL_SCANCODE_F8, 0, 25, 1 },
	{ SDL_SCANCODE_F9, 0, 26, 1 }, { SDL_SCANCODE_F10, 0, 27, 1 }, { SDL_SCANCODE_F11, 0, 28, 1 },
	{ SDL_SCANCODE_F12, 0, 29, 1 },
	{ SDL_SCANCODE_1, 1, 17, 1 }, { SDL_SCANCODE_2, 1, 18, 1 }, { SDL_SCANCODE_3, 1, 19, 1 },
	{ SDL_SCANCODE_4, 1, 20, 1 }, { SDL_SCANCODE_5, 1, 21, 1 }, { SDL_SCANCODE_6, 1, 22, 1 },
	{ SDL_SCANCODE_7, 1, 23, 1 }, { SDL_SCANCODE_8, 1, 24, 1 }, { SDL_SCANCODE_9, 1, 25, 1 },
	{ SDL_SCANCODE_0, 1, 26, 1 }, { SDL_SCANCODE_MINUS, 1, 27, 1 }, { SDL_SCANCODE_EQUALS, 1, 29, 1 },
	{ SDL_SCANCODE_BACKSPACE, 1, 32, 2 },
	{ SDL_SCANCODE_Q, 2, 17, 1 }, { SDL_SCANCODE_W, 2, 18, 1 }, { SDL_SCANCODE_E, 2, 19, 1 },
	{ SDL_SCANCODE_R, 2, 20, 1 }, { SDL_SCANCODE_T, 2, 21, 1 }, { SDL_SCANCODE_Y, 2, 22, 1 },
	{ SDL_SCANCODE_U, 2, 23, 1 }, { SDL_SCANCODE_I, 2, 24, 1 }, { SDL_SCANCODE_O, 2, 25, 1 },
	{ SDL_SCANCODE_P, 2, 26, 1 }, { SDL_SCANCODE_LEFTBRACKET, 2, 27, 1 }, { SDL_SCANCODE_RIGHTBRACKET, 2, 28, 1 },
	{ SDL_SCANCODE_BACKSLASH, 2, 31, 1 },
	{ SDL_SCANCODE_A, 3, 18, 1 }, { SDL_SCANCODE_S, 3, 19, 1 }, { SDL_SCANCODE_D, 3, 20, 1 },
	{ SDL_SCANCODE_F, 3, 21, 1 }, { SDL_SCANCODE_G, 3, 22, 1 }, { SDL_SCANCODE_H, 3, 23, 1 },
	{ SDL_SCANCODE_J, 3, 24, 1 }, { SDL_SCANCODE_K, 3, 25, 1 }, { SDL_SCANCODE_L, 3, 26, 1 },
	{ SDL_SCANCODE_APOSTROPHE, 3, 27, 1 }, { SDL_SCANCODE_SEMICOLON, 3, 30, 1 },
	{ SDL_SCANCODE_RETURN, 3, 32, 2 }, { SDL_SCANCODE_KP_ENTER, 3, 32, 2 },
	{ SDL_SCANCODE_Z, 4, 19, 1 }, { SDL_SCANCODE_X, 4, 20, 1 }, { SDL_SCANCODE_C, 4, 21, 1 },
	{ SDL_SCANCODE_V, 4, 22, 1 }, { SDL_SCANCODE_B, 4, 23, 1 }, { SDL_SCANCODE_N, 4, 24, 1 },
	{ SDL_SCANCODE_M, 4, 25, 1 }, { SDL_SCANCODE_COMMA, 4, 26, 1 }, { SDL_SCANCODE_PERIOD, 4, 27, 1 },
	{ SDL_SCANCODE_SLASH, 4, 29, 1 },
	{ SDL_SCANCODE_UP, 4, 30, 1 }, { SDL_SCANCODE_RIGHT, 4, 31, 1 }, { SDL_SCANCODE_DOWN, 4, 32, 1 },
	{ SDL_SCANCODE_LEFT, 4, 33, 1 },
	{ SDL_SCANCODE_LALT, 5, 17, 2 }, { SDL_SCANCODE_RALT, 5, 17, 2 }, { SDL_SCANCODE_TAB, 5, 19, 2 },
	{ SDL_SCANCODE_DELETE, 5, 21, 2 }, { SDL_SCANCODE_END, 5, 23, 2 },
	{ SDL_SCANCODE_LCTRL, 6, 17, 2 }, { SDL_SCANCODE_RCTRL, 6, 17, 2 }, { SDL_SCANCODE_CAPSLOCK, 6, 19, 2 },
	{ SDL_SCANCODE_HOME, 6, 21, 2 }, { SDL_SCANCODE_PAGEUP, 6, 23, 2 }, { SDL_SCANCODE_PAGEDOWN, 6, 25, 2 },
	{ SDL_SCANCODE_SPACE, 6, 32, 2 },
	{ SDL_SCANCODE_LSHIFT, 7, 17, 2 }, { SDL_SCANCODE_RSHIFT, 7, 17, 2 }, { SDL_SCANCODE_INSERT, 7, 19, 2 },
	{ SDL_SCANCODE_PAUSE, 7, 25, 2 },
};

static const prompt button_prompts[] = {
	{ SDL_CONTROLLER_BUTTON_A, 0, 4, 1 }, { SDL_CONTROLLER_BUTTON_B, 0, 5, 1 },
	{ SDL_CONTROLLER_BUTTON_X, 0, 6, 1 }, { SDL_CONTROLLER_BUTTON_Y, 0, 7, 1 },
	{ SDL_CONTROLLER_BUTTON_DPAD_UP, 1, 1, 1 }, { SDL_CONTROLLER_BUTTON_DPAD_RIGHT, 1, 2, 1 },
	{ SDL_CONTROLLER_BUTTON_DPAD_DOWN, 1, 3, 1 }, { SDL_CONTROLLER_BUTTON_DPAD_LEFT, 1, 4, 1 },
	{ SDL_CONTROLLER_BUTTON_BACK, 18, 4, 1 }, { SDL_CONTROLLER_BUTTON_START, 18, 5, 1 },
	{ SDL_CONTROLLER_BUTTON_LEFTSHOULDER, 18, 7, 1 }, { SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, 18, 8, 1 },
};

#define PROMPT_SIZE  16
#define PROMPT_CLEAR 0xFFFF00FFu    /* magenta: the sheet's see-through */

static SDL_Surface *prompt_sheet;    /* input_prompts.bmp beside the executable, as ARGB */
static uint32_t controls_picture[S32_VIDEO_MAX_WIDTH * S32_VIDEO_HEIGHT];

static void load_prompts(const char *base)
{
	char path[1024];
	SDL_Surface *loaded;

	snprintf(path, sizeof(path), "%sinput_prompts.bmp", base);
	loaded = SDL_LoadBMP(path);
	if (!loaded)
		return;    /* (CONTROLS then shows the names alone) */
	prompt_sheet = SDL_ConvertSurfaceFormat(loaded, SDL_PIXELFORMAT_ARGB8888, 0);
	SDL_FreeSurface(loaded);
}

static const prompt *find_prompt(const prompt *list, size_t count, int what)
{
	for (size_t i = 0; i < count; i++)
		if (list[i].what == what)
			return &list[i];
	return NULL;
}

/* One prompt into the picture, its top left at (x, y) */
static void draw_prompt(const prompt *icon, int x, int y)
{
	const uint32_t *sheet = (const uint32_t *)prompt_sheet->pixels;
	const int pitch = prompt_sheet->pitch / 4;

	for (int row = 0; row < PROMPT_SIZE; row++)
		for (int column = 0; column < PROMPT_SIZE * icon->tiles; column++)
		{
			const uint32_t color = sheet[(icon->row * PROMPT_SIZE + row) * pitch + icon->column * PROMPT_SIZE + column];

			if ((color | 0xFF000000u) == PROMPT_CLEAR || y + row < 0 || y + row >= S32_VIDEO_HEIGHT
				|| x + column >= S32_VIDEO_MAX_WIDTH)
				continue;
			controls_picture[(y + row) * S32_VIDEO_MAX_WIDTH + x + column] = color | 0xFF000000u;
		}
}

/* The frame with each of the shown player's actions' first two keys and first button on its line */
static const uint32_t *with_controls(const uint32_t *pixels)
{
	const action_binding *bindings = action_bindings[console_controls_player()];

	memcpy(controls_picture, pixels, sizeof(controls_picture));
	for (int action = 0; action < ACTIONS; action++)
	{
		const int y = console_controls_row(action) * 8 - 4;
		const prompt *icon, *first = NULL;
		int shown = 0;

		for (int key = 0; key < ACTION_KEYS && shown < 2; key++)
		{
			icon = find_prompt(key_prompts, SDL_arraysize(key_prompts), bindings[action].keys[key]);
			if (!icon || (first && first->row == icon->row && first->column == icon->column))
				continue;    /* (no picture, or the first key's: Enter and the keypad's) */
			draw_prompt(icon, s32_video_side() + (shown++ ? CONSOLE_CONTROLS_SECOND_X : CONSOLE_CONTROLS_KEYS_X), y);
			first = first ? first : icon;
		}
		if ((icon = find_prompt(button_prompts, SDL_arraysize(button_prompts), bindings[action].buttons[0])) != NULL)
			draw_prompt(icon, s32_video_side() + CONSOLE_CONTROLS_BUTTON_X, y);
	}
	return controls_picture;
}
#endif

/* The board's frame drawn (the console's draws WIDE's sides of a stage's map too) */
static void draw_frame(void)
{
#ifdef SEGASONIC_CONSOLE
	console_draw_frame();
#else
	s32_video_draw_frame();
#endif
}

static void on_frame_shown(void)
{
	int width;
	const uint32_t *pixels;

	draw_frame();
	pixels = s32_video_frame(&width);
#ifdef SEGASONIC_CONSOLE
	if (prompt_sheet && console_controls_row(0) >= 0)
		pixels = with_controls(pixels);
#endif
	present(pixels, width);
}

static void toggle_full_screen(void)
{
	settings.full_screen = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP) == 0;
	SDL_SetWindowFullscreen(window, settings.full_screen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
	settings_save();
}

/* Smooth or sharp pixels (present) */
static void set_smooth(bool smooth)
{
	(void)smooth;    /* (present picks the way each frame: the game's texture stays sharp) */
}

static void toggle_smooth(void)
{
	settings.smooth = !settings.smooth;
	set_smooth(settings.smooth);
	settings_save();
}

/* ---- the controls ---- */

static void show_mouse_speed(void)
{
	char title[96];

	snprintf(title, sizeof(title), WINDOW_TITLE " - mouse %d%% (F5/F6), ball %s (F4)",
		(int)lround(settings.mouse_speed * 100), settings.ball_rolls_on ? "rolls on" : "stops with the mouse");
	SDL_SetWindowTitle(window, title);
}

static void toggle_ball_rolls_on(void)
{
	settings.ball_rolls_on = !settings.ball_rolls_on;
	show_mouse_speed();
	settings_save();
}

/* The mouse's frame on a player's trackball. While the hand moves, the ball turns with it; when the hand stops
   (or is lifted to start again), the ball rolls on and slows down, as the arcade's does. The right button
   (of the window's one mouse) is the hand resting on the ball: the ball only turns as the mouse moves. */
static void roll_ball(int player, bool hand_on_ball)
{
	if (hand_moved[player] || hand_on_ball || !settings.ball_rolls_on)
	{
		ball[player][0] = hand[player][0];
		ball[player][1] = hand[player][1];
	}
	else
	{
		const double speed = hypot(ball[player][0], ball[player][1]);
		const double slower = speed * settings.ball_drag - settings.ball_friction;
		const double keep = slower > 0.0 ? slower / speed : 0.0;

		ball[player][0] *= keep;
		ball[player][1] *= keep;
	}
	for (int axis = 0; axis < 2; axis++)
	{
		int whole;

		mouse_rest[player][axis] += ball[player][axis];
		whole = (int)mouse_rest[player][axis];
		*(axis ? &controls.roll_y[player] : &controls.roll_x[player]) += whole;
		mouse_rest[player][axis] -= whole;
		hand[player][axis] = 0.0;
	}
	hand_moved[player] = false;
}

#ifdef SEPARATE_MICE
/* Whether each mouse is its own trackball: Raw Input has told a mouse apart, and sonic.ini does not say no.
   Until then the window's mouse events are the mouse (a remote desktop, say, sends no raw ones) */
static bool separate_mice_now(void)
{
	return mice_listening && settings.separate_mice && mice_seen();
}
#endif

/* The mice's frame: each player's mouse on their trackball, and whether a button of it is held. The window's
   one mouse is player 1's, with its right button holding the ball, where the mice are not told apart. */
static void roll_mice(Uint32 mouse, bool held[PLAYERS])
{
	for (int player = 0; player < PLAYERS; player++)
		held[player] = false;
#ifdef SEPARATE_MICE
	if (separate_mice_now())
	{
		for (int player = 0; player < PLAYERS; player++)
		{
			int dx, dy;
			unsigned down;

			mice_take(player, &dx, &dy, &down);
#ifdef SEGASONIC_CONSOLE
			if (!mouse_held)
				dx = dy = 0, down = 0;    /* (the pointer's, until a click takes the mouse) */
			if (mouse_click_taken)
				down = 0;                 /* (the click that took it is not the button) */
#endif
			/* the board's X counts the other way (MAME reverses it) */
			hand[player][0] -= dx * settings.mouse_speed;
			hand[player][1] += dy * settings.mouse_speed;
			hand_moved[player] |= dx != 0 || dy != 0;
			roll_ball(player, (down & 2u) != 0);    /* (the right button holds the ball, the left is the button) */
			held[player] = (down & 1u) != 0;
		}
		return;
	}
#endif
	roll_ball(0, (mouse & SDL_BUTTON_RMASK) != 0);
	held[0] = (mouse & SDL_BUTTON_LMASK) != 0;
}

static void change_mouse_speed(bool faster)
{
	settings.mouse_speed *= faster ? 1.25 : 0.8;
	if (settings.mouse_speed < 0.05)
		settings.mouse_speed = 0.05;
	if (settings.mouse_speed > 4.0)
		settings.mouse_speed = 4.0;
	show_mouse_speed();
	settings_save();
}

/* the settings the picture shows, as the file has them after it was read again */
#ifdef SEGASONIC_CONSOLE
static char times_path[1024];    /* segasonic.times: TIME ATTACK's best times, written by the game */

static void load_times(void)
{
	FILE *input = fopen(times_path, "r");
	int level = 0;
	unsigned long frames = 0;
	char line[128];

	if (!input)
		return;
	while (fgets(line, sizeof(line), input))
		if (sscanf(line, " level %d = %lu", &level, &frames) == 2 && level >= 0 && level < CONSOLE_LEVELS)
			console_best_times.levels[level] = (uint32_t)frames;
		else if (sscanf(line, " run %d = %lu", &level, &frames) == 2 && level >= 0 && level < CONSOLE_LEVELS)
			console_best_times.run[level] = (uint32_t)frames;
	fclose(input);
}

static void save_times(void)
{
	FILE *output = fopen(times_path, "w");

	console_best_times.changed = false;
	if (!output)
		return;
	fputs("; TIME ATTACK's best times, in the board's frames (60 a second): the game writes this file.\n"
		"; level: each level alone; run: the best whole game, its time at each level's end\n", output);
	for (int level = 0; level < CONSOLE_LEVELS; level++)
		fprintf(output, "level %d = %lu\n", level, (unsigned long)console_best_times.levels[level]);
	for (int level = 0; level < CONSOLE_LEVELS; level++)
		fprintf(output, "run %d = %lu\n", level, (unsigned long)console_best_times.run[level]);
	fclose(output);
}

/* The console's OPTIONS show the window's settings as they are */
static void tell_console_settings(void)
{
	console_window_settings.window_scale = (int)lround(settings.window_scale);
	console_window_settings.full_screen = settings.full_screen;
	console_window_settings.smooth = settings.smooth;
	console_window_settings.bilinear = settings.bilinear;
	console_window_settings.crt = settings.crt;
	console_window_settings.wide = settings.wide;
	console_window_settings.volume = (int)lround(settings.volume * 5.0);
	console_window_settings.title_music = (int)settings.title_music;
	console_window_settings.continues = (int)settings.continues;
	console_window_settings.changed = false;
}
#endif

#ifdef SEGASONIC_HANDHELD
/* A game device's build (the Switch's, a handheld's, Android's): one screen and the device's own controls, so
   the picture is fixed, the whole screen and smooth pixels, whatever segasonic.ini says; OPTIONS has no
   VIDEO page (console.c) */
static void fix_settings(void)
{
#ifdef SEGASONIC_SCREEN_WIDE
	settings.wide = true;     /* (a 16:9 screen: the Switch's; a phone's, wider) */
#else
	settings.wide = false;    /* (a 4:3 one: the game's own picture, twice its size on a handheld's 640 x 480) */
#endif
	settings.full_screen = true;
	settings.smooth = true;
	settings.bilinear = false;
	settings.crt = false;
}
#endif

static void apply_settings(void)
{
#ifdef SEGASONIC_HANDHELD
	fix_settings();
	set_smooth(settings.smooth);
#else
	const bool full = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;

	set_smooth(settings.smooth);
	if (full != settings.full_screen)
		SDL_SetWindowFullscreen(window, settings.full_screen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
	if (settings.window_scale != shown_scale || picture_width() != shown_width)
	{
		shown_scale = settings.window_scale;
		shown_width = picture_width();
		SDL_SetWindowSize(window, (int)(picture_width() * window_scale_that_fits()),
			(int)(S32_VIDEO_HEIGHT * window_scale_that_fits()));
	}
#endif
	show_mouse_speed();
#ifdef SEGASONIC_CONSOLE
	console_jingle_program((int)settings.sega_jingle_program, (int)settings.sega_jingle_layer);
	tell_console_settings();
#endif
}

#ifdef __ANDROID__
/* Android's own: the fingers on the touch controls; the game going to the background, where Android may
   end it, so what it keeps is written then */
static void android_event(const SDL_Event *event)
{
	touch_event(event);
	if (event->type == SDL_APP_WILLENTERBACKGROUND)
		save_eeprom();
}
#endif

static void read_events(void)
{
	SDL_Event event;

	while (SDL_PollEvent(&event))
	{
#ifdef SEGASONIC_HANDHELD_KEYS
		handheld_event(&event);
#endif
#ifdef __ANDROID__
		android_event(&event);
#endif
		switch (event.type)
		{
			case SDL_QUIT:
				longjmp(finished, 1);
#if defined(__ANDROID__) && defined(SEGASONIC_CONSOLE)
			case SDL_APP_WILLENTERBACKGROUND:
				if (console_in_game())
					pause_requested = true;    /* (back in the game: it waits, paused) */
				break;
#endif
			case SDL_MOUSEMOTION:
#ifdef SEGASONIC_CONSOLE
				if (!mouse_held)
					break;
#endif
#ifdef SEPARATE_MICE
				if (separate_mice_now())
					break;    /* (each mouse's own movement comes from Raw Input: roll_mice) */
#endif
				/* the board's X counts the other way (MAME reverses it) */
				hand[0][0] -= event.motion.xrel * settings.mouse_speed;
				hand[0][1] += event.motion.yrel * settings.mouse_speed;
				hand_moved[0] = true;
				break;
			case SDL_KEYDOWN:
				if (event.key.repeat)
					break;
#ifdef SEGASONIC_CONSOLE
				if (console_controls_waiting() >= 0)
				{
					/* CONTROLS waits for a key: this one (Escape leaves the action as it was) */
					if (event.key.keysym.sym != SDLK_ESCAPE)
						actions_bind_key(console_controls_player(), console_controls_waiting(), event.key.keysym.scancode);
					console_controls_bound();
					bound_now = true;
					break;
				}
#endif
#ifndef SEGASONIC_CONSOLE
				if (event.key.keysym.sym == SDLK_ESCAPE)
					longjmp(finished, 1);    /* (the console's back and pause are actions: actions.c) */
#endif
				if (event.key.keysym.sym == SDLK_F11
					|| (event.key.keysym.sym == SDLK_RETURN && (event.key.keysym.mod & KMOD_ALT)))
					toggle_full_screen();
				if (event.key.keysym.sym == SDLK_F3)
					toggle_smooth();
				if (event.key.keysym.sym == SDLK_F4)
					toggle_ball_rolls_on();
#ifdef SEGASONIC_CONSOLE
				if (event.key.keysym.sym == SDLK_F7 || event.key.keysym.sym == SDLK_F8)
					console_debug_section(event.key.keysym.sym == SDLK_F8 ? 1 : -1);
#endif
				if (event.key.keysym.sym == SDLK_F5 || event.key.keysym.sym == SDLK_F6)
					change_mouse_speed(event.key.keysym.sym == SDLK_F6);
				break;
#ifdef SEGASONIC_CONSOLE
			case SDL_MOUSEBUTTONDOWN:
				if (!mouse_held)
				{
					mouse_held = true;
					mouse_click_taken = true;
					SDL_SetRelativeMouseMode(SDL_TRUE);
				}
				break;
			case SDL_MOUSEBUTTONUP:
				if (mouse_click_taken && SDL_GetMouseState(NULL, NULL) == 0)
					mouse_click_taken = false;
				break;
			case SDL_CONTROLLERBUTTONDOWN:
				if (console_controls_waiting() >= 0)
				{
					actions_bind_button(console_controls_player(), console_controls_waiting(),
						(SDL_GameControllerButton)event.cbutton.button);
#ifdef SEGASONIC_HANDHELD
					fprintf(stderr, "CONTROLS: player %d, action %d: %s\n", console_controls_player() + 1,
						console_controls_waiting(),
						SDL_GameControllerGetStringForButton((SDL_GameControllerButton)event.cbutton.button));
					log_written();
#endif
					console_controls_bound();
					bound_now = true;
				}
				break;
#endif
			case SDL_CONTROLLERDEVICEADDED:
				for (int player = 0; player < PLAYERS; player++)
					if (!controllers[player])
					{
						controllers[player] = SDL_GameControllerOpen(event.cdevice.which);
						break;
					}
				break;
			case SDL_CONTROLLERDEVICEREMOVED:
				for (int player = 0; player < PLAYERS; player++)
					if (controllers[player]
						&& SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controllers[player])) == event.cdevice.which)
					{
						SDL_GameControllerClose(controllers[player]);
						controllers[player] = NULL;
					}
				break;
			default:
				break;
		}
	}
}

/* the state of every control once a frame, where the board reads them */
static void on_controls_update(void)
{
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	Uint32 mouse = SDL_GetMouseState(NULL, NULL);
	bool mouse_down[PLAYERS];
	s32_inputs *input = &s32_board.input;

	read_events();
#ifdef SEGASONIC_CONSOLE
	if (!mouse_held || mouse_click_taken)
		mouse = 0;    /* (the pointer's, or the click that took the mouse) */
#endif
#ifdef SEPARATE_MICE
	if (mice_listening)
		mice_update();
#endif
	roll_mice(mouse, mouse_down);
#ifndef SEGASONIC_CONSOLE
	controls.roll_x[0] += (keys[SDL_SCANCODE_LEFT] - keys[SDL_SCANCODE_RIGHT]) * (int)lround(settings.key_roll);
	controls.roll_y[0] += (keys[SDL_SCANCODE_DOWN] - keys[SDL_SCANCODE_UP]) * (int)lround(settings.key_roll);
	controls.button[0] = keys[SDL_SCANCODE_Z] || keys[SDL_SCANCODE_SPACE];
	controls.button[1] = controls.button[2] = false;
	for (int player = 0; player < PLAYERS; player++)
		controls.button[player] |= mouse_down[player];
#endif
	for (int player = 0; player < PLAYERS; player++)
	{
		controls.coin[player] = keys[SDL_SCANCODE_5 + player] != 0;
		controls.start[player] = keys[SDL_SCANCODE_1 + player] != 0;
	}
#ifdef SEGASONIC_CONSOLE
	/* the console: each player's keys and game controller (controls.ini) roll their trackball and press
	   their button. Start is their start, but in a stage start and back pause it, unless the player's start
	   prompt is up (to join, or a continue's count); back elsewhere goes back (the title's menu, OPTIONS:
	   Escape never closes the console's window) */
#ifdef SEGASONIC_TOUCH
	actions_touch(touch_held());
#endif
	actions_read(controllers);
	if (console_controls_defaults())
		actions_defaults(console_controls_player());
	if (bound_now)
	{
		bound_now = false;
		actions_forget_pressed();    /* (the key the CONTROLS screen took acts no more this frame) */
	}
	for (int player = 0; player < PLAYERS; player++)
	{
		SDL_GameController *pad = controllers[player];

		controls.roll_x[player] += (actions_holds(player, pad, ACTION_LEFT) - actions_holds(player, pad, ACTION_RIGHT))
			* (int)lround(settings.key_roll);
		controls.roll_y[player] += (actions_holds(player, pad, ACTION_DOWN) - actions_holds(player, pad, ACTION_UP))
			* (int)lround(settings.key_roll);
		controls.button[player] = actions_holds(player, pad, ACTION_JUMP);
		if (!console_in_game() || console_start_prompted(player))
			controls.start[player] |= (actions_player_held(player) & ACTION_BIT(ACTION_START)) != 0;
		else if (actions_player_pressed(player) & PAUSE_ACTIONS)
			pause_requested = true;
	}
#ifdef SEGASONIC_TOUCH
	{
		/* the touch screen's stick rolls player 1's trackball as a game controller's left stick, its A jumps */
		int lean_x, lean_y;

		touch_stick(&lean_x, &lean_y);
		controls.roll_x[0] -= (int)lround((double)lean_x / TOUCH_STICK_FULL * settings.stick_roll);
		controls.roll_y[0] += (int)lround((double)lean_y / TOUCH_STICK_FULL * settings.stick_roll);
		controls.button[0] |= (touch_held() & ACTION_BIT(ACTION_JUMP)) != 0;
	}
#endif
	for (int player = 0; player < PLAYERS; player++)
		controls.button[player] |= mouse_down[player];
	if (!console_in_game() && (actions_pressed() & ACTION_BIT(ACTION_BACK)))
		console_back();
	console_actions(actions_pressed(), actions_held());
	s32_video_set_wide(console_wide_now());    /* (this frame's sprites and picture) */
#endif
	controls.service = keys[SDL_SCANCODE_9] != 0;
	controls.test = keys[SDL_SCANCODE_F2] != 0;
	for (int player = 0; player < PLAYERS; player++)
	{
		SDL_GameController *pad = controllers[player];

		if (!pad)
			continue;
		controls.roll_x[player] -= (int)lround(SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTX) / 32767.0 * settings.stick_roll);
		controls.roll_y[player] += (int)lround(SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTY) / 32767.0 * settings.stick_roll);
#ifndef SEGASONIC_CONSOLE
		controls.button[player] |= SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_A) != 0;
		controls.start[player] |= SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_START) != 0;
		controls.coin[player] |= SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_BACK) != 0;
#endif
	}

	/* the board's ports are active low: a cleared bit is a pressed button */
	input->port_a = (uint8_t)(0xff & ~(controls.button[0] ? 0x01 : 0) & ~(controls.button[2] ? 0x04 : 0));
	input->port_b = (uint8_t)(0xff & ~(controls.button[1] ? 0x01 : 0));
	input->port_e = (uint8_t)(0xff
		& ~(controls.service ? 0x01 : 0) & ~(controls.test ? 0x02 : 0)
		& ~(controls.coin[0] ? 0x04 : 0) & ~(controls.coin[1] ? 0x08 : 0) & ~(controls.coin[2] ? 0x40 : 0)
		& ~(controls.start[0] ? 0x10 : 0) & ~(controls.start[1] ? 0x20 : 0) & ~(controls.start[2] ? 0x80 : 0));
	for (int player = 0; player < PLAYERS; player++)
	{
		input->trackball_x[player] = (uint16_t)(input->trackball_x[player] + controls.roll_x[player]);
		input->trackball_y[player] = (uint16_t)(input->trackball_y[player] + controls.roll_y[player]);
		controls.roll_x[player] = controls.roll_y[player] = 0;
	}
}

/* ---- the sound ---- */

static void keep(resampler *chip, const int32_t *samples, size_t count)
{
	if (chip->count + count > chip->capacity)
	{
		chip->capacity = (chip->count + count) * 2;
		chip->samples = (int32_t *)realloc(chip->samples, chip->capacity * 2 * sizeof(int32_t));
	}
	memcpy(chip->samples + chip->count * 2, samples, count * 2 * sizeof(int32_t));
	chip->count += count;
}

/* the chip's output at the next output sample, between the two samples around it */
static void sample_at(const resampler *chip, double position, double out[2])
{
	const size_t index = (size_t)position;
	const double fraction = position - (double)index;

	for (int side = 0; side < 2; side++)
	{
		const double here = chip->samples[index * 2 + side];
		const double next = chip->samples[(index + 1) * 2 + side];
		out[side] = here + (next - here) * fraction;
	}
}

static void drop_used(resampler *chip)
{
	const size_t used = (size_t)chip->position;

	if (!used)
		return;
	memmove(chip->samples, chip->samples + used * 2, (chip->count - used) * 2 * sizeof(int32_t));
	chip->count -= used;
	chip->position -= (double)used;
}

/* the frame's samples of the three chips, mixed as MAME's routes do (each YM3438 at 0.30, the RF5C68
   at 0.40) at 48 kHz, at the settings' volume */

static void play_frame(void)
{
	static int16_t mix[OUTPUT_RATE / 20 * 2];
	size_t made = 0;

	sound_chips_end_frame(s32_board.cycles);
	for (int chip = 0; chip < 3; chip++)
	{
		size_t count;
		const int32_t *samples = sound_chips_frame_samples(chip, &count);
		keep(&chips[chip], samples, count);
	}
	while (made < OUTPUT_RATE / 20)
	{
		double fm0[2], fm1[2], pcm[2];

		if (chips[0].position + 1 >= (double)chips[0].count || chips[1].position + 1 >= (double)chips[1].count
			|| chips[2].position + 1 >= (double)chips[2].count)
			break;
		sample_at(&chips[0], chips[0].position, fm0);
		sample_at(&chips[1], chips[1].position, fm1);
		sample_at(&chips[2], chips[2].position, pcm);
		for (int side = 0; side < 2; side++)
		{
			double value = settings.volume * (0.30 * (fm0[side] + fm1[side]) + 0.40 * pcm[side]);
			mix[made * 2 + side] = (int16_t)(value > 32767 ? 32767 : (value < -32768 ? -32768 : value));
		}
		for (int chip = 0; chip < 3; chip++)
			chips[chip].position += chips[chip].step;
		made++;
	}
	for (int chip = 0; chip < 3; chip++)
		drop_used(&chips[chip]);
	if (audio && SDL_GetQueuedAudioSize(audio) < (Uint32)(OUTPUT_RATE / 60 * 4 * QUEUE_FRAMES_MAX))
		SDL_QueueAudio(audio, mix, (Uint32)(made * 2 * sizeof(int16_t)));
}

#ifdef SEGASONIC_CONSOLE
/* ---- the console's pause ---- */

static uint32_t pause_picture[S32_VIDEO_MAX_WIDTH * S32_VIDEO_HEIGHT];

/* The frame as it stands in grey, with "PAUSE" in the game's font in colour on top: the frame is drawn
   again with the word on the text layer, and the pixels that change are the word's */
static void draw_pause_picture(int *width)
{
	const uint32_t *pixels;

	draw_frame();
	pixels = s32_video_frame(width);
	memcpy(pause_picture, pixels, sizeof(pause_picture));
	console_pause_text(true);
	draw_frame();
	pixels = s32_video_frame(width);
	for (size_t i = 0; i < sizeof(pause_picture) / sizeof(pause_picture[0]); i++)
	{
		const uint32_t color = pause_picture[i];
		uint32_t grey;

		if (pixels[i] != color)
		{
			pause_picture[i] = pixels[i];
			continue;
		}
		grey = ((((color >> 16) & 0xFF) * 77 + ((color >> 8) & 0xFF) * 150 + (color & 0xFF) * 29) >> 8) * 3 / 4;
		pause_picture[i] = 0xFF000000u | (grey << 16) | (grey << 8) | grey;
	}
	console_pause_text(false);
	draw_frame();
}

#ifdef SEGASONIC_PAUSE_EXIT
/* The pause's choice under a finger at (x, y) (SDL's, 0 to 1 across the screen), or -1: a choice's row and
   half a row over and under it, across the picture's middle half */
static int pause_item_at(float x, float y)
{
	int out_width = 0, out_height = 0;
	double column, row;

	SDL_GetRendererOutputSize(renderer, &out_width, &out_height);
	if (picture_shown.w <= 0 || picture_shown.h <= 0)
		return -1;
	column = (x * out_width - picture_shown.x) / picture_shown.w;
	row = (y * out_height - picture_shown.y) / picture_shown.h * S32_VIDEO_HEIGHT / 8.0;
	if (column < 0.25 || column > 0.75)
		return -1;
	for (int item = 0; item < CONSOLE_PAUSE_ITEMS; item++)
		if (fabs(row - (CONSOLE_PAUSE_MENU_ROW + 2.0 * item + 0.5)) <= 1.0)
			return item;
	return -1;
}
#endif

/* The actions start or back in a stage: the game stops, its sound too, until one of them is pressed
   again. The mouse is the pointer again (the window's close button, another window), until a click in
   the window takes it back. Android's (SEGASONIC_PAUSE_EXIT) has its choices: CONTINUE, or EXIT, which
   ends the game as the title's EXIT does elsewhere */
static void run_pause(void)
{
	int width;
	SDL_Event event;
	bool paused = true;
#ifdef SEGASONIC_PAUSE_EXIT
	int choice = CONSOLE_PAUSE_CONTINUE, picked = -1;
#endif

	pause_requested = false;
#ifdef SEGASONIC_HANDHELD_KEYS
	save_handheld_settings();
#endif
	mouse_held = false;    /* (a click takes it again) */
	SDL_SetRelativeMouseMode(SDL_FALSE);
	if (audio)
		SDL_PauseAudioDevice(audio, 1);
#ifdef SEGASONIC_PAUSE_EXIT
	console_pause_choice(choice);
#endif
	draw_pause_picture(&width);
	present(pause_picture, width);
	while (paused)
	{
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
				longjmp(finished, 1);
#ifdef SEGASONIC_HANDHELD_KEYS
			handheld_event(&event);
#endif
#ifdef __ANDROID__
			android_event(&event);
#endif
			if (event.type == SDL_KEYDOWN && !event.key.repeat && (event.key.keysym.sym == SDLK_F11
				|| (event.key.keysym.sym == SDLK_RETURN && (event.key.keysym.mod & KMOD_ALT))))
			{
				toggle_full_screen();
				present(pause_picture, width);
			}
			if (event.type == SDL_WINDOWEVENT)
				present(pause_picture, width);
#ifdef SEGASONIC_TOUCH
			if (event.type == SDL_FINGERDOWN || event.type == SDL_FINGERUP)
				present(pause_picture, width);    /* (START lit while a finger is on it) */
#endif
#ifdef SEGASONIC_PAUSE_EXIT
			if (event.type == SDL_FINGERDOWN && pause_item_at(event.tfinger.x, event.tfinger.y) >= 0)
				picked = pause_item_at(event.tfinger.x, event.tfinger.y);    /* (a choice touched: picked) */
#endif
		}
#ifdef SEGASONIC_TOUCH
		actions_touch(touch_held());
#endif
		actions_read(controllers);
		if (actions_pressed() & PAUSE_ACTIONS)
			paused = false;
#ifdef SEGASONIC_PAUSE_EXIT
		/* up and down move to a choice, accept picks it; CONTINUE goes on once the button that picked it is let
		   go (held into the stage, it would jump) */
		if (actions_pressed() & (ACTION_BIT(ACTION_UP) | ACTION_BIT(ACTION_DOWN)))
		{
			choice = (actions_pressed() & ACTION_BIT(ACTION_UP)) ? CONSOLE_PAUSE_CONTINUE : CONSOLE_PAUSE_EXIT;
			console_pause_choice(choice);
			draw_pause_picture(&width);
			present(pause_picture, width);
		}
		if (picked < 0 && (actions_pressed() & ACTION_BIT(ACTION_ACCEPT)))
			picked = choice;
		if (picked == CONSOLE_PAUSE_EXIT)
			longjmp(finished, 1);
		if (picked == CONSOLE_PAUSE_CONTINUE && !(actions_held() & ACTION_BIT(ACTION_ACCEPT)))
			paused = false;
#endif
		SDL_Delay(16);
	}
	if (audio)
	{
		SDL_ClearQueuedAudio(audio);
		SDL_PauseAudioDevice(audio, 0);
	}
	next_frame = SDL_GetPerformanceCounter();    /* (the time paused is not owed) */
}
#endif

/* ---- the frame ---- */

static void wait_for_frame(void)
{
	const uint64_t frequency = SDL_GetPerformanceFrequency();
	uint64_t now = SDL_GetPerformanceCounter();

	next_frame += frequency / S32_FRAME_RATE;
	if (now > next_frame + frequency / 4)
		next_frame = now;    /* far behind (the window was moved, the machine was busy): start over */
	while ((now = SDL_GetPerformanceCounter()) < next_frame)
	{
		const uint64_t left = (next_frame - now) * 1000 / frequency;
		if (left > 2)
			SDL_Delay((Uint32)(left - 1));
	}
}

#ifdef SEGASONIC_HANDHELD
/* ---- a game device's log (stderr: segasonic.log): what SDL found, the frames' pace, the buttons' numbers ---- */

/* Handhelds' own buttons as SDL's game controller has them, where SDL does not know them. Anbernic's H700
   consoles show theirs as a joystick, "ANBERNIC-keys" (gpio-keys: bus 0x19, vendor and product 1, version
   0x100), numbered as Anbernic's SDL 2.0.12 numbers them: the RG40XX H's as its controls test found them,
   button by button (the d-pad a hat, the sticks four axes; MENU is guide, button 8) */
static const char *const device_mappings[] = {
	"19000000010000000100000000010000,ANBERNIC-keys,a:b0,b:b1,x:b3,y:b2,leftshoulder:b4,rightshoulder:b10,"
	"lefttrigger:b5,righttrigger:b11,back:b6,start:b7,guide:b8,dpup:h0.1,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,"
	"leftx:a0,lefty:a1,rightx:a2,righty:a3,leftstick:b9,rightstick:b12,platform:Linux,",
};

static uint64_t pace_start, pace_work_ticks, pace_work_most, pace_frame_start;
static int pace_frames;

/* The log on the card as it is written: a handheld is often left with its reset button */
static void log_written(void)
{
	fflush(stderr);
#if !defined(_WIN32) && !defined(__SWITCH__)
	fsync(fileno(stderr));
#endif
}

/* The mappings as a hint before SDL starts: the game controllers it finds then are opened as such (added
   after, a joystick already there stays a joystick until it is plugged in again, as SDL 2.0.12 does) */
static void add_device_mappings(void)
{
	static char mappings[2048];

	mappings[0] = '\0';
	for (size_t mapping = 0; mapping < SDL_arraysize(device_mappings); mapping++)
	{
		SDL_strlcat(mappings, device_mappings[mapping], sizeof(mappings));
		SDL_strlcat(mappings, "\n", sizeof(mappings));
	}
	SDL_SetHint(SDL_HINT_GAMECONTROLLERCONFIG, mappings);
}

#ifdef SEGASONIC_HANDHELD_KEYS
/* A handheld's own keys, as its firmware's games have them: MENU leaves the game (the joystick's own button,
   whatever the mapping makes of the rest; the Switch's HOME leaves it there), the volume keys turn the
   game's volume (OPTIONS' VOLUME), a step of ten a press. They come as keys or as joystick buttons the
   mapping has not: the log says which */
static void handheld_event(const SDL_Event *event)
{
	int step = 0;

	if (event->type == SDL_JOYBUTTONDOWN && event->jbutton.button == HANDHELD_MENU_BUTTON)
		longjmp(finished, 1);
	if (event->type == SDL_KEYDOWN && !event->key.repeat)
	{
		fprintf(stderr, "key: %s\n", SDL_GetKeyName(event->key.keysym.sym));
		step = event->key.keysym.sym == SDLK_VOLUMEUP ? 1 : (event->key.keysym.sym == SDLK_VOLUMEDOWN ? -1 : 0);
	}
	if (event->type == SDL_JOYBUTTONDOWN && event->jbutton.button > 12)
		fprintf(stderr, "joystick button: %d\n", event->jbutton.button);    /* (one the mapping has not) */
	if (step)
	{
		const long volume = lround(settings.volume * 5.0) + step;

		settings.volume = (volume < 0 ? 0 : (volume > 10 ? 10 : volume)) / 5.0;
		settings_unsaved = true;    /* (written at the pause or at the end: a write to the card holds frames) */
		fprintf(stderr, "volume: %ld\n", lround(settings.volume * 5.0));
	}
}

/* The volume the keys turned, into segasonic.ini, where a moment's wait does not show */
static void save_handheld_settings(void)
{
	if (!settings_unsaved)
		return;
	settings_save();
	settings_unsaved = false;
}
#endif

static void log_device(void)
{
	SDL_version linked;
	SDL_RendererInfo info;
	int width = 0, height = 0;

	SDL_GetVersion(&linked);
	fprintf(stderr, "SDL %d.%d.%d, video %s, audio %s\n", linked.major, linked.minor, linked.patch,
		SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "none",
		SDL_GetCurrentAudioDriver() ? SDL_GetCurrentAudioDriver() : "none");
	if (renderer && SDL_GetRendererInfo(renderer, &info) == 0)
	{
		SDL_GetRendererOutputSize(renderer, &width, &height);
		fprintf(stderr, "renderer %s, screen %d x %d\n", info.name, width, height);
	}
	for (int index = 0; index < SDL_NumJoysticks(); index++)
	{
		char guid[64];

		SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(index), guid, sizeof(guid));
		fprintf(stderr, "joystick %d: %s, %s, %s\n", index, SDL_JoystickNameForIndex(index), guid,
			SDL_IsGameController(index) ? "a game controller" : "no game controller mapping");
	}
#ifdef SEGASONIC_TOUCH
	fprintf(stderr, "touch screens: %d\n", SDL_GetNumTouchDevices());
#endif
	log_written();
}


/* Every 5 seconds: the frames a second, and the time a frame takes to make before it waits (the average
   and the most) */
static void log_pace(bool frame_made)
{
	const uint64_t now = SDL_GetPerformanceCounter(), frequency = SDL_GetPerformanceFrequency();

	if (!frame_made)
	{
		pace_frame_start = now;    /* (after the wait: the next frame starts) */
		return;
	}
	if (!pace_start)
		pace_start = now;
	if (pace_frame_start)
	{
		const uint64_t work = now - pace_frame_start;

		pace_work_ticks += work;
		if (work > pace_work_most)
			pace_work_most = work;
	}
	if (++pace_frames == 300)
	{
		fprintf(stderr, "%.1f frames a second; a frame takes %.1f ms, at most %.1f\n",
			pace_frames * (double)frequency / (double)(now - pace_start),
			1000.0 * (double)pace_work_ticks / (double)frequency / pace_frames,
			1000.0 * (double)pace_work_most / (double)frequency);
		fflush(stderr);    /* (not to the card at once: a write to it would hold a frame) */
		pace_start = now;
		pace_frames = 0;
		pace_work_ticks = pace_work_most = 0;
	}
}
#endif

static void on_frame_done(void)
{
#ifdef SEGASONIC_HANDHELD
	log_pace(true);
#endif
	play_frame();
#ifdef SEGASONIC_CONSOLE
	if (console_quit_requested())
		longjmp(finished, 1);
	if (console_window_settings.changed)
	{
		settings.window_scale = console_window_settings.window_scale;
		settings.full_screen = console_window_settings.full_screen;
		settings.smooth = console_window_settings.smooth;
		settings.bilinear = console_window_settings.bilinear;
		settings.crt = console_window_settings.crt;
		settings.wide = console_window_settings.wide;
		settings.volume = console_window_settings.volume / 5.0;
		settings.title_music = console_window_settings.title_music;
		settings.continues = console_window_settings.continues;
		apply_settings();
		settings_save();
	}
	tell_console_settings();    /* (F11 and F3 change them too) */
	if (console_best_times.changed)
		save_times();
	if (pause_requested)
		run_pause();
#endif
	if (program_frames() % 600 == 0)
		save_eeprom();
	if (program_frames() % 30 == 0 && settings_reread())
		apply_settings();
	wait_for_frame();
#ifdef SEGASONIC_HANDHELD
	log_pace(false);
#endif
}

static void on_irq_changed(bool asserted)
{
	program_irq_line(asserted);
}

static void on_log(const char *message)
{
	(void)message;
}

/* A window with what went wrong: the port has no console */
static int fail(const char *what, const char *detail)
{
	char message[1200];

	snprintf(message, sizeof(message), "%s\n%s", what, detail);
	fprintf(stderr, "%s\n", message);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, WINDOW_TITLE, message, NULL);
	return 1;
}

/* Where the game keeps what it writes (its settings, the EEPROM): beside the executable on Windows, as it
   always was; elsewhere the user's own folder for the game (SDL_GetPrefPath: ~/.local/share/SegaSonic,
   ~/Library/Application Support/SegaSonic), as the executable's may not be written (an .app, /usr/bin).
   The caller frees it (SDL_free) */
static char *saves_folder(void)
{
#ifdef _WIN32
	return SDL_GetBasePath();
#elif defined(__SWITCH__)
	mkdir(SWITCH_FOLDER, 0777);
	return SDL_strdup(SWITCH_FOLDER "/");
#elif defined(__ANDROID__)
	return SDL_GetPrefPath("", "SegaSonic");    /* (the app's own folder, its internal storage) */
#elif defined(SEGASONIC_HANDHELD)
	return SDL_GetBasePath();    /* (a handheld's: the game's folder on its card) */
#else
	return SDL_GetPrefPath("", "SegaSonic");
#endif
}

#ifdef __ANDROID__
/* The pack in the APK's assets (a relative path to SDL's files there) as a FILE (funopen's), as the pack
   on a computer is read */
static int read_packed(void *packed, char *buffer, int size)
{
	return (int)SDL_RWread((SDL_RWops *)packed, buffer, 1, (size_t)size);
}

static fpos_t seek_packed(void *packed, fpos_t offset, int whence)
{
	return (fpos_t)SDL_RWseek((SDL_RWops *)packed, (Sint64)offset, whence);    /* (RW_SEEK_* are SEEK_*) */
}

static int close_packed(void *packed)
{
	return SDL_RWclose((SDL_RWops *)packed);
}
#endif

/* The assets pack: beside the executable, else in the folder of what the game keeps (Android's: in the
   APK) */
static game_data_result load_assets(const char *base, const char *saves, char *path, size_t size)
{
	game_data_result result;
#ifdef __ANDROID__
	SDL_RWops *packed = SDL_RWFromFile(ASSETS_FILE, "rb");
	FILE *input = packed ? funopen(packed, read_packed, NULL, seek_packed, close_packed) : NULL;

	(void)base;
	(void)saves;
	snprintf(path, size, "assets/" ASSETS_FILE);
	if (!input)
	{
		if (packed)
			SDL_RWclose(packed);
		return GAME_DATA_MISSING;
	}
	result = game_data_read(input);
	fclose(input);    /* (the asset too: close_packed) */
	return result;
#else

	snprintf(path, size, "%s" ASSETS_FILE, base ? base : "");
	result = game_data_load(path);
	if (result != GAME_DATA_MISSING || !saves)
		return result;
	snprintf(path, size, "%s" ASSETS_FILE, saves);
	return game_data_load(path);
#endif
}

int main(int argc, char **argv)
{
	SDL_AudioSpec wanted;
	char settings_path[1024], assets_path[1024];
	char *base, *saves;
	game_data_result assets;

	(void)argc;
	(void)argv;
#ifdef __SWITCH__
	romfsInit();
	/* (what the game says, failures included, goes to segasonic.log in its folder: the Switch has no console
	   and SDL no message boxes there) */
	mkdir(SWITCH_FOLDER, 0777);
	freopen(SWITCH_FOLDER "/segasonic.log", "w", stderr);
	fprintf(stderr, "SegaSonic: starting\n");
#endif
#ifdef __ANDROID__
	/* (what the game says goes to segasonic.log in the app's folder on the storage, Android/data/<the
	   app>/files: stderr goes nowhere there) */
	if (SDL_AndroidGetExternalStoragePath())
	{
		char log_path[1024];

		snprintf(log_path, sizeof(log_path), "%s/segasonic.log", SDL_AndroidGetExternalStoragePath());
		freopen(log_path, "w", stderr);
	}
	fprintf(stderr, "SegaSonic: starting\n");
	SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");    /* (back is the menus' back, actions.c, never the end) */
	SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");          /* (fingers are the touch controls, not the mouse) */
	SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");
	SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
#endif
#ifdef SEGASONIC_HANDHELD
	add_device_mappings();
#endif
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0)
		return fail("SDL could not start.", SDL_GetError());
#ifdef __SWITCH__
	base = SDL_strdup("romfs:/");
#else
	base = SDL_GetBasePath();
#endif
	saves = saves_folder();
	snprintf(settings_path, sizeof(settings_path), "%s" FILE_NAME ".ini", saves ? saves : "");
	snprintf(eeprom_path, sizeof(eeprom_path), "%s" FILE_NAME ".eeprom", saves ? saves : "");
#ifdef SEGASONIC_CONSOLE
	{
		char controls_path[1024];

		snprintf(controls_path, sizeof(controls_path), "%scontrols.ini", saves ? saves : "");
		actions_open(controls_path);
		load_prompts(base ? base : "");    /* (the pictures come with the game, beside it) */
		snprintf(times_path, sizeof(times_path), "%s" FILE_NAME ".times", saves ? saves : "");
		load_times();
	}
#endif
	assets = load_assets(base, saves, assets_path, sizeof(assets_path));
	fprintf(stderr, "assets: %s (%d)\n", assets_path, (int)assets);
#if !defined(__SWITCH__) && !defined(SEGASONIC_HANDHELD)
	/* (a game device makes nothing from the ROM set: its assets come with the game, made on a computer, in
	   the Switch's .nsp or beside a handheld's executable) */
	if (assets == GAME_DATA_MISSING)
	{
		/* (the first run: the pack from the player's ROM set, where the game keeps what it writes) */
		snprintf(assets_path, sizeof(assets_path), "%s" ASSETS_FILE, saves ? saves : "");
		if (!first_run(base, saves, assets_path))
		{
			fprintf(stderr, "first run: no assets made\n");
			SDL_Quit();
			return 0;
		}
		assets = game_data_load(assets_path);
	}
#endif
	switch (assets)
	{
	case GAME_DATA_LOADED:
		break;
	case GAME_DATA_MISSING:
		return fail("The game's assets are missing.", assets_path);
	default:
		return fail("The game's assets cannot be read.", assets_path);
	}
	settings_open(settings_path);
#ifdef SEGASONIC_HANDHELD
	fix_settings();
#endif
#ifdef SEGASONIC_CONSOLE
	console_jingle_program((int)settings.sega_jingle_program, (int)settings.sega_jingle_layer);
#endif
#ifdef SEGASONIC_CONSOLE
	tell_console_settings();
#endif
	shown_scale = settings.window_scale;
	shown_width = picture_width();
#ifdef __SWITCH__
	/* (the Switch's screen, 1280 x 720; docked, the system shows it at the TV's size) */
	window = SDL_CreateWindow(WINDOW_TITLE, 0, 0, 1280, 720, 0);
#else
	window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		(int)(picture_width() * window_scale_that_fits()), (int)(S32_VIDEO_HEIGHT * window_scale_that_fits()),
		SDL_WINDOW_RESIZABLE | (settings.full_screen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0));
#endif
#ifdef SEGASONIC_CONSOLE
	if (window)
	{
		/* the window's icon: the three characters' heads, from the game's assets (console_window_icon) */
		static uint32_t icon_pixels[CONSOLE_ICON_SIZE * CONSOLE_ICON_SIZE];
		SDL_Surface *icon = NULL;

		if (console_window_icon(icon_pixels, game_sprites, GAME_SPRITES_SIZE, game_program))
			icon = SDL_CreateRGBSurfaceWithFormatFrom(icon_pixels, CONSOLE_ICON_SIZE, CONSOLE_ICON_SIZE, 32,
				CONSOLE_ICON_SIZE * 4, SDL_PIXELFORMAT_ARGB8888);
		if (icon)
		{
			SDL_SetWindowIcon(window, icon);
			SDL_FreeSurface(icon);
		}
	}
#endif
#ifdef _WIN32
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d11");    /* (SDL tries Direct3D 9 first; the others if 11 fails) */
#endif
	renderer = window ? SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED) : NULL;
	if (window && !renderer)
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	texture = renderer ? SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
		S32_VIDEO_MAX_WIDTH, S32_VIDEO_HEIGHT) : NULL;
	if (!texture)
		return fail("The window could not be opened.", SDL_GetError());
	fprintf(stderr, "window: %d x %d\n", picture_width(), S32_VIDEO_HEIGHT);
#ifdef SEGASONIC_TOUCH
	touch_open(renderer, base ? base : "");    /* (its letters come with the game: Android's in the APK) */
#endif
	SDL_free(base);
	SDL_free(saves);
#ifdef SEGASONIC_HANDHELD
	log_device();
#endif
	SDL_RenderSetIntegerScale(renderer, SDL_FALSE);
	SDL_SetTextureScaleMode(texture, SDL_ScaleModeNearest);
#ifndef SEGASONIC_CONSOLE
	SDL_SetRelativeMouseMode(SDL_TRUE);    /* (the arcade's trackball; the console's waits for a click) */
#endif
#ifdef SEPARATE_MICE
	mice_listening = settings.separate_mice && mice_open(window);
#endif
	show_mouse_speed();

	SDL_zero(wanted);
	wanted.freq = OUTPUT_RATE;
	wanted.format = AUDIO_S16SYS;
	wanted.channels = 2;
	wanted.samples = 1024;
	audio = SDL_OpenAudioDevice(NULL, 0, &wanted, NULL, 0);
	if (audio)
		SDL_PauseAudioDevice(audio, 0);
	chips[0].step = chips[1].step = FM_RATE / OUTPUT_RATE;
	chips[2].step = PCM_RATE / OUTPUT_RATE;

	/* the board, the video, the sound board and the game, as the headless runs set them up */
	s32_board.irq_changed = on_irq_changed;
	s32_board.controls_update = on_controls_update;
	s32_board.log = on_log;
	s32_board.frame_timing = true;
	s32_board.sound_rom = game_sound;
#ifdef SEGASONIC_CONSOLE
	console_prepare_sprites(game_sprites, GAME_SPRITES_SIZE, game_program);    /* (the console's changes) */
#endif
	s32_video_init(game_tiles, GAME_TILES_SIZE, game_sprites, GAME_SPRITES_SIZE);
	s32_board.frame_shown = on_frame_shown;
	/* the video's rows on the cores beside the game's (runtime/src/workers.c): the Switch's 1 and 2, up to
	   three elsewhere (a handheld's four cores) */
#ifdef __SWITCH__
	s32_workers_start(2);
#else
	s32_workers_start(SDL_min(SDL_GetCPUCount() - 1, 3));
#endif
	fprintf(stderr, "video: %d threads beside the game's\n", s32_workers_count());
	sound_chips_attach();
	sound_cpu_attach();
	s32_init(game_program);
	load_eeprom();
	program_reset();
	program_frame_done = on_frame_done;
	next_frame = SDL_GetPerformanceCounter();

	if (!setjmp(finished))
		program_run();

	save_eeprom();
#ifdef SEGASONIC_HANDHELD_KEYS
	save_handheld_settings();
	fflush(stderr);
#endif
	if (audio)
		SDL_CloseAudioDevice(audio);
	SDL_Quit();
#ifdef __SWITCH__
	romfsExit();
#endif
#ifdef __ANDROID__
	exit(0);    /* (the app's process ends with the game: Android would start the next one in it, as this one left it) */
#endif
	return 0;
}

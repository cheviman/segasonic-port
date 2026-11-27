/**
 *    Name:        window.c
 *    Description: SegaSonic NSP's window: the .nsp's four inputs and their state (the console's keys, the ROM
 *                 set, the icon, the output folder), files dropped on it taken by what they are, a button
 *                 for the output folder and one (or Enter) that makes the .nsp
 *    Version:     0.1.0
 *    Created:     25-10-2026
 *    Modified:    27-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    Started with nothing (a double click), the tool opens this window. Each input starts as the command
 *    line's default (make_nsp.c) and is what the player drops on the window or chooses for it; the game's
 *    Switch build, the folder switch beside the tool, is shown too. Under each its place: the path the
 *    player gave, or for a default where the tool looks ("beside this tool"), never a folder of the
 *    computer's own, which a picture of the window would show. Each state is looked at again when the
 *    window comes back to the front (a file put beside the tool meanwhile). The .nsp is made by make_nsp, as
 *    the command line makes it, on a thread of its own: the window keeps drawing while it works, and when
 *    it is closed then, it waits out of sight for the .nsp to be written. Its letters are Kenney Mini's
 *    (ports/sdl/letters.c, font.bmp beside the tool). SDL gives paths in UTF-8; on Windows they go to the
 *    C library in the system's code page. On Windows the console the tool is started with is closed when
 *    it is the tool's own (a double click), and kept when it is a command line's.
 */
#define SDL_MAIN_HANDLED    /* (the tool's main is main.c's, a console program's) */

#include "window.h"
#include "folder_dialog.h"
#include "make_nsp.h"
#include "letters.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include "port_sdl.h"
#define SDL_MAIN_HANDLED    /* (the tool's own main, main.c's: a console program) */
#include <SDL3/SDL_main.h>

#include <stdio.h>
#include <string.h>

#define WINDOW_TITLE     "SegaSonic NSP"
#define WINDOW_WIDTH     800
#define WINDOW_HEIGHT    472
#define MARGIN           24
#define TITLE_TOP        24
#define ROWS             5
#define ROWS_TOP         116
#define ROW_HEIGHT       44
#define LINE_GAP         4
#define BUTTONS_TOP      344
#define BUTTON_WIDTH     300
#define BUTTON_HEIGHT    36
#define BUTTON_GAP       24
#define STATUS_TOP       400
#define BAR_WIDTH        480
#define BAR_HEIGHT       6
#define BAR_EVERY        40      /* milliseconds between two of the bar's frames while it works */
#define BAR_ROUND        1600    /* milliseconds for its block to go across */
#define LETTER_SPACING   2       /* letters.c's pixels between two characters */
#define BESIDE_TOOL      "beside this tool"

#ifdef _WIN32
#define SEPARATOR "\\"
#else
#define SEPARATOR "/"
#endif

enum
{
	WAITING,
	MAKING,
	MADE,
	FAILED
};

enum
{
	NO_BUTTON = -1,
	FOLDER_BUTTON,
	MAKE_BUTTON,
	BUTTONS
};

typedef struct
{
	char line[128];               /* its name and its state: "Console keys (prod.keys): found" */
	char place[NSP_PATH_SIZE + 16];    /* where it is: the player's path, or where a default is looked for */
	int good;
} input_row;

typedef struct
{
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Cursor *arrow, *hand;
	letters font;
	char here[NSP_PATH_SIZE];     /* the tool's folder, with its separator: the defaults' */
	nsp_inputs given;             /* what the player gave: files dropped, the folder chosen */
	nsp_inputs inputs;            /* those and the defaults: what the next .nsp is made of */
	input_row rows[ROWS];
	int phase, hover, pressed, no_dialog;
	char note[96];                /* what the folder's button had to say */
	Uint32 started;               /* SDL_GetTicks when it started making the .nsp */
	/* the thread that makes it: its own copy of the inputs, what it says as it goes and at the end */
	SDL_Thread *thread;
	nsp_inputs making;
	nsp_outcome outcome;
	char made_place[NSP_PATH_SIZE + 32];    /* where the .nsp goes, as its row says it */
	int made;
	void *step;                   /* the step it is at (a const char *), SDL_SetAtomicPointer's */
	SDL_AtomicInt done;
} tool_window;

static tool_window tool;

/* A path from SDL (UTF-8) as fopen takes it */
static void native_path(const char *utf8, char *path, size_t size)
{
#ifdef _WIN32
	WCHAR wide[NSP_PATH_SIZE];

	if (MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wide, NSP_PATH_SIZE) > 0
		&& WideCharToMultiByte(CP_ACP, 0, wide, -1, path, (int)size, NULL, NULL) > 0)
		return;
#endif
	snprintf(path, size, "%s", utf8);
}

/* The window's own handle, the folder dialog's owner (Windows'; NULL elsewhere) */
static void *window_handle(SDL_Window *window)
{
#ifdef _WIN32
	void *handle = SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);

	if (handle)
		return handle;
#else
	(void)window;
#endif
	return NULL;
}

/* ---- the inputs' state ---- */

static void set_row(input_row *row, const char *name, const char *place, const char *problem, const char *good)
{
	snprintf(row->line, sizeof(row->line), "%s: %s", name, problem ? problem : good);
	snprintf(row->place, sizeof(row->place), "%s", place);
	row->good = problem == NULL;
}

/* Where the keys are looked for: beside the tool, then in .switch in the player's folder */
static const char *keys_place(const nsp_inputs *inputs)
{
	if (tool.given.keys[0])
		return inputs->keys;
	if (strncmp(inputs->keys, tool.here, strlen(tool.here)) == 0)
		return BESIDE_TOOL;
	return nsp_file_exists(inputs->keys) ? "in .switch in your user folder"
		: BESIDE_TOOL ", or in .switch in your user folder";
}

/* The inputs again (the player's, the defaults for the rest) and what each is now */
static void refresh(void)
{
	nsp_inputs *const inputs = &tool.inputs;
	char path[NSP_PATH_SIZE + 16];

	*inputs = tool.given;
	nsp_defaults(inputs, tool.here);
	set_row(&tool.rows[0], "Console keys (prod.keys)", keys_place(inputs), nsp_keys_problem(inputs->keys), "found");
	set_row(&tool.rows[1], "ROM set (sonic.zip: MAME's, Japan, rev. C)", tool.given.rom[0] ? inputs->rom : BESIDE_TOOL,
		nsp_file_exists(inputs->rom) || nsp_folder_exists(inputs->rom) ? NULL : "not found", "found");
	if (inputs->icon[0])
		set_row(&tool.rows[2], "HOME menu icon (a 256 x 256 JPEG)",
			tool.given.icon[0] ? inputs->icon : NSP_OWN_ICON ", " BESIDE_TOOL, nsp_icon_problem(inputs->icon), "yours");
	else
	{
		snprintf(path, sizeof(path), "%s" SEPARATOR "icon.jpg", inputs->pieces);
		set_row(&tool.rows[2], "HOME menu icon (a 256 x 256 JPEG)", "in the game's Switch build",
			nsp_icon_problem(path), "the game's own");
	}
	snprintf(path, sizeof(path), "%s" SEPARATOR "main", inputs->pieces);
	set_row(&tool.rows[3], "The game's Switch build",
		tool.given.pieces[0] ? inputs->pieces : "the folder switch, " BESIDE_TOOL,
		nsp_file_exists(path) ? NULL : "not found", "found");
	nsp_folder_of(inputs->out, path, sizeof(path));
	set_row(&tool.rows[4], "Output folder", tool.given.out[0] ? path : BESIDE_TOOL,
		nsp_folder_exists(path) ? NULL : "not found", "SegaSonic.nsp goes here");
}

/* ---- drawing ---- */

/* A line's width at scale 1, as letters_draw lays it out */
static int line_width(const char *text)
{
	int width = 0;

	for (; *text; text++)
		if (*text >= 0x20 && *text <= 0x7E)
			width += tool.font.widths[*text - 0x20] + LETTER_SPACING;
	return width > 0 ? width - LETTER_SPACING : 0;
}

/* 'text' in 'width' pixels: when it is wider, its end after "..." (a path's file is at its end) */
static const char *fitted(const char *text, int width, char *out, size_t size)
{
	const int dots = line_width("...") + LETTER_SPACING;
	int left = line_width(text);

	if (left <= width)
		return text;
	for (; *text && dots + left > width; text++)
		if (*text >= 0x20 && *text <= 0x7E)
			left -= tool.font.widths[*text - 0x20] + LETTER_SPACING;
	snprintf(out, size, "...%s", text);
	return out;
}

static void fill(int r, int g, int b, const SDL_Rect *box)
{
	SDL_FRect frect;

	SDL_RectToFRect(box, &frect);
	SDL_SetRenderDrawColor(tool.renderer, (Uint8)r, (Uint8)g, (Uint8)b, 255);
	SDL_RenderFillRect(tool.renderer, &frect);
}

static SDL_Rect button_box(int button)
{
	SDL_Rect box;

	box.w = BUTTON_WIDTH;
	box.h = BUTTON_HEIGHT;
	box.x = (WINDOW_WIDTH - BUTTONS * BUTTON_WIDTH - (BUTTONS - 1) * BUTTON_GAP) / 2
		+ button * (BUTTON_WIDTH + BUTTON_GAP);
	box.y = BUTTONS_TOP;
	return box;
}

static int button_at(float x, float y)
{
	for (int button = 0; button < BUTTONS; button++)
	{
		const SDL_Rect box = button_box(button);
		const SDL_Point point = { (int)x, (int)y };

		if (SDL_PointInRect(&point, &box))
			return button;
	}
	return NO_BUTTON;
}

/* Nothing works while the .nsp is made; the folder's button, when the system has no folder dialog */
static int button_works(int button)
{
	return button != NO_BUTTON && tool.phase != MAKING && !(button == FOLDER_BUTTON && tool.no_dialog);
}

static void draw_button(int button, const char *label)
{
	const SDL_Rect box = button_box(button);
	const int works = button_works(button), lit = works && tool.hover == button;
	SDL_Color text = { 255, 255, 255, 255 };

	if (!works)
	{
		fill(52, 62, 110, &box);
		text.r = 150;
		text.g = 160;
		text.b = 200;
	}
	else if (button == MAKE_BUTTON)
	{
		/* (the one Enter presses: yellow) */
		if (lit)
			fill(255, 236, 140, &box);
		else
			fill(255, 210, 60, &box);
		text.r = 16;
		text.g = 24;
		text.b = 64;
	}
	else if (lit)
		fill(84, 116, 224, &box);
	else
		fill(48, 76, 170, &box);
	letters_draw(tool.renderer, &tool.font, label, box.y + (box.h - LETTERS_CELL) / 2, 1, text, 2 * box.x + box.w);
}

/* While it works: a block going across a bar, below the step it is at */
static void draw_bar(int y)
{
	const Uint32 into = (Uint32)(SDL_GetTicks() - tool.started) % BAR_ROUND;
	SDL_Rect bar, block;

	bar.w = BAR_WIDTH;
	bar.h = BAR_HEIGHT;
	bar.x = (WINDOW_WIDTH - BAR_WIDTH) / 2;
	bar.y = y;
	fill(40, 52, 110, &bar);
	block = bar;
	block.w = BAR_WIDTH / 5;
	block.x = bar.x + (int)(into * (Uint32)(BAR_WIDTH - block.w) / BAR_ROUND);
	fill(255, 210, 60, &block);
}

static void draw(void)
{
	const SDL_Color white = { 255, 255, 255, 255 }, yellow = { 255, 220, 64, 255 }, red = { 255, 110, 90, 255 };
	const int width = WINDOW_WIDTH - 2 * MARGIN;
	char line[160], shorter[NSP_PATH_SIZE + 160];
	const char *first, *second = NULL;
	SDL_Color colour = white;
	int y = ROWS_TOP;

	/* (a hand over a button that works) */
	if (tool.hand && tool.arrow)
		SDL_SetCursor(button_works(tool.hover) ? tool.hand : tool.arrow);
	SDL_SetRenderDrawColor(tool.renderer, 16, 24, 64, 255);
	SDL_RenderClear(tool.renderer);
	letters_draw(tool.renderer, &tool.font, "SegaSonic NSP", TITLE_TOP, 3, yellow, WINDOW_WIDTH);
	letters_draw(tool.renderer, &tool.font, "The Switch game as an .nsp to install, made of your own files",
		TITLE_TOP + LETTERS_CELL * 3 + 12, 1, white, WINDOW_WIDTH);
	for (int row = 0; row < ROWS; row++)
	{
		letters_draw(tool.renderer, &tool.font, tool.rows[row].line, y, 1, tool.rows[row].good ? yellow : red,
			WINDOW_WIDTH);
		letters_draw(tool.renderer, &tool.font, fitted(tool.rows[row].place, width, shorter, sizeof(shorter)),
			y + LETTERS_CELL + LINE_GAP, 1, white, WINDOW_WIDTH);
		y += ROW_HEIGHT;
	}
	draw_button(FOLDER_BUTTON, tool.no_dialog ? "No folder dialog on this system" : "Choose the output folder");
	draw_button(MAKE_BUTTON, tool.phase == MAKING ? "Making SegaSonic.nsp..." : "Make SegaSonic.nsp");

	/* below the buttons: what it is doing, what it made or what failed */
	if (tool.phase == MAKING)
	{
		const char *const step = (const char *)SDL_GetAtomicPointer(&tool.step);

		snprintf(line, sizeof(line), "Making SegaSonic.nsp: %s", step ? step : "starting");
		first = line;
		colour = yellow;
	}
	else if (tool.phase == MADE)
	{
		snprintf(line, sizeof(line), "SegaSonic.nsp is written: %.1f MB", (double)tool.outcome.size / (1024.0 * 1024.0));
		first = line;
		second = tool.made_place;
		colour = yellow;
	}
	else if (tool.phase == FAILED)
	{
		first = tool.outcome.what;
		second = tool.outcome.detail;
		colour = red;
	}
	else if (tool.note[0])
	{
		first = tool.note;
		colour = red;
	}
	else
	{
		first = "Drop prod.keys, sonic.zip or a 256 x 256 JPEG on this window";
		second = "Enter makes SegaSonic.nsp";
	}
	letters_draw(tool.renderer, &tool.font, first, STATUS_TOP, 1, colour, WINDOW_WIDTH);
	if (tool.phase == MAKING)
		draw_bar(STATUS_TOP + LETTERS_CELL + LINE_GAP * 3);
	else if (second && second[0])
		letters_draw(tool.renderer, &tool.font, fitted(second, width, shorter, sizeof(shorter)),
			STATUS_TOP + LETTERS_CELL + LINE_GAP, 1, white, WINDOW_WIDTH);
	SDL_RenderPresent(tool.renderer);
}

/* ---- what the buttons do ---- */

static void told_step(void *context, const char *step)
{
	(void)context;
	SDL_SetAtomicPointer(&tool.step, (void *)step);
}

static int making_thread(void *context)
{
	(void)context;
	tool.made = make_nsp(&tool.making, NULL, told_step, NULL, &tool.outcome);
	SDL_SetAtomicInt(&tool.done, 1);
	return 0;
}

static void start_making(void)
{
	if (tool.phase == MAKING)
		return;
	refresh();
	tool.making = tool.inputs;
	snprintf(tool.made_place, sizeof(tool.made_place), "%s",
		tool.given.out[0] ? tool.making.out : NSP_FILE ", " BESIDE_TOOL);
	memset(&tool.outcome, 0, sizeof(tool.outcome));
	SDL_SetAtomicPointer(&tool.step, NULL);
	SDL_SetAtomicInt(&tool.done, 0);
	tool.note[0] = '\0';
	tool.started = (Uint32)SDL_GetTicks();
	tool.phase = MAKING;
	if (!(tool.thread = SDL_CreateThread(making_thread, "SegaSonic NSP", NULL)))
	{
		snprintf(tool.outcome.what, sizeof(tool.outcome.what), "Making SegaSonic.nsp cannot start");
		tool.phase = FAILED;
	}
}

static void end_making(void)
{
	SDL_WaitThread(tool.thread, NULL);
	tool.thread = NULL;
	tool.phase = tool.made ? MADE : FAILED;
	refresh();
}

static void choose_folder(void)
{
	char start[NSP_PATH_SIZE], chosen[NSP_PATH_SIZE - sizeof(NSP_FILE) - 1];    /* (room for SegaSonic.nsp after it) */
	int got;

	nsp_folder_of(tool.inputs.out, start, sizeof(start));
	got = folder_dialog(window_handle(tool.window), start, chosen, sizeof(chosen));
	SDL_PumpEvents();
	SDL_FlushEvents(SDL_EVENT_MOUSE_MOTION, SDL_EVENT_MOUSE_WHEEL);    /* (what the mouse did over the dialog) */
	tool.pressed = NO_BUTTON;
	tool.note[0] = '\0';
	if (got == FOLDER_NO_DIALOG)
		tool.no_dialog = 1;    /* (the button says so; the folder stays as it was) */
	else if (got == FOLDER_UNUSABLE)
		snprintf(tool.note, sizeof(tool.note), "The folder chosen cannot be used");
	else if (got == FOLDER_CHOSEN)
	{
		const size_t length = strlen(chosen);
		const int ends = length > 0 && (chosen[length - 1] == '/' || chosen[length - 1] == '\\');

		snprintf(tool.given.out, sizeof(tool.given.out), "%s%s" NSP_FILE, chosen, ends ? "" : SEPARATOR);
		tool.phase = WAITING;
	}
	refresh();
}

static void press(int button)
{
	if (!button_works(button))
		return;
	if (button == FOLDER_BUTTON)
		choose_folder();
	else
		start_making();
}

/* Started from Explorer the console is the tool's own: it closes, and the window stays. From a command line
   it is the command line's, and stays */
static void close_own_console(void)
{
#ifdef _WIN32
	DWORD processes[2];

	if (GetConsoleProcessList(processes, 2) == 1)
		FreeConsole();
#endif
}

static void close_window(void)
{
	if (tool.hand)
		SDL_DestroyCursor(tool.hand);
	if (tool.arrow)
		SDL_DestroyCursor(tool.arrow);
	letters_free(&tool.font);
	if (tool.renderer)
		SDL_DestroyRenderer(tool.renderer);
	if (tool.window)
		SDL_DestroyWindow(tool.window);
	SDL_Quit();
}

int nsp_window(const char *here)
{
	const char *base;
	int running = 1;

	SDL_SetMainReady();
	if (!SDL_Init(SDL_INIT_VIDEO))
		return -1;
	memset(&tool, 0, sizeof(tool));
	tool.hover = tool.pressed = NO_BUTTON;
	base = SDL_GetBasePath();
	if (base)
		native_path(base, tool.here, sizeof(tool.here));
	else
		snprintf(tool.here, sizeof(tool.here), "%s", here);
	tool.window = SDL_CreateWindow(WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_HIDDEN);
	tool.renderer = tool.window ? SDL_CreateRenderer(tool.window, NULL) : NULL;
	if (!tool.renderer || !letters_load(tool.renderer, base, &tool.font))
	{
		close_window();
		return -1;
	}
	tool.arrow = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
	tool.hand = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
	SDL_SetEventEnabled(SDL_EVENT_DROP_FILE, true);
	refresh();
	SDL_ShowWindow(tool.window);
	close_own_console();
	draw();
	while (running)
	{
		SDL_Event event;
		const int making = tool.phase == MAKING;
		const int got = making ? SDL_WaitEventTimeout(&event, BAR_EVERY) : SDL_WaitEvent(&event);

		if (making && SDL_GetAtomicInt(&tool.done))
		{
			end_making();
			draw();
		}
		if (!got)
		{
			if (!making)
				break;     /* (SDL's events failed) */
			if (tool.phase == MAKING)
				draw();    /* (the bar's next frame) */
			continue;
		}
		if (event.type == SDL_EVENT_QUIT)
			running = 0;
		else if (event.type == SDL_EVENT_DROP_FILE)
		{
			if (tool.phase != MAKING)
			{
				char path[NSP_PATH_SIZE];

				native_path(event.drop.data, path, sizeof(path));
				nsp_take_dropped(&tool.given, path);
				tool.phase = WAITING;
				tool.note[0] = '\0';
				refresh();
				draw();
			}
		}
		else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat
			&& (event.key.key == SDLK_RETURN || event.key.key == SDLK_KP_ENTER))
		{
			press(MAKE_BUTTON);
			draw();
		}
		else if (event.type == SDL_EVENT_MOUSE_MOTION)
		{
			const int hover = button_at(event.motion.x, event.motion.y);

			if (hover != tool.hover)
			{
				tool.hover = hover;
				draw();
			}
		}
		else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT)
			tool.pressed = button_at(event.button.x, event.button.y);
		else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT)
		{
			const int button = button_at(event.button.x, event.button.y);

			if (button != NO_BUTTON && button == tool.pressed)
				press(button);
			tool.pressed = NO_BUTTON;
			draw();
		}
		else if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST)
		{
			/* (any event of the window's: SDL 3 has each as an event of its own) */
			if (event.type == SDL_EVENT_WINDOW_FOCUS_GAINED && tool.phase != MAKING)
				refresh();
			else if (event.type == SDL_EVENT_WINDOW_MOUSE_LEAVE)
				tool.hover = NO_BUTTON;
			draw();
		}
	}
	if (tool.thread)
	{
		/* (closed while it works: the .nsp is finished out of sight) */
		SDL_HideWindow(tool.window);
		SDL_WaitThread(tool.thread, NULL);
	}
	close_window();
	return 0;
}

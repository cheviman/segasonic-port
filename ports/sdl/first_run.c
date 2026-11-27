/**
 *    Name:        first_run.c
 *    Description: the first run: the game's assets pack from the player's ROM set, found beside the game (on
 *                 macOS beside its .app) or where it keeps its files, or dropped on a window that asks for it
 *    Version:     0.1.0
 *    Created:     20-10-2026
 *    Modified:    27-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    The window has none of the game's letters (they are in the ROM): it writes with Kenney Mini (letters.c;
 *    font.bmp beside the executable). The ROM set is read once (runtime/src/extract.c): the pack is the
 *    assets alone, and the game never opens the ROM set again.
 */
#include "first_run.h"
#include "letters.h"

#include "game/extract.h"

#include "port_sdl.h"

#include <stdio.h>
#include <string.h>

#define ROM_SET_FILE    "sonic.zip"
#define WINDOW_WIDTH    640
#define WINDOW_HEIGHT   448
#define LINE_GAP        8

static void draw_window(SDL_Renderer *renderer, const letters *font, const char *problem, int reading)
{
	const SDL_Color white = { 255, 255, 255, 255 }, blue = { 120, 170, 255, 255 }, red = { 255, 110, 90, 255 };
	int y = 120;

	SDL_SetRenderDrawColor(renderer, 16, 24, 64, 255);
	SDL_RenderClear(renderer);
	letters_draw(renderer, font, "SegaSonic The Hedgehog", y, 3, blue, WINDOW_WIDTH);
	y += LETTERS_CELL * 3 + LINE_GAP * 4;
	if (reading)
		letters_draw(renderer, font, "Reading the ROM set...", y, 2, white, WINDOW_WIDTH);
	else
	{
		letters_draw(renderer, font, "Drop the arcade ROM set here", y, 2, white, WINDOW_WIDTH);
		y += LETTERS_CELL * 2 + LINE_GAP;
		letters_draw(renderer, font, "sonic.zip: MAME's set, Japan, rev. C", y, 1, white, WINDOW_WIDTH);
		y += LETTERS_CELL + LINE_GAP;
		letters_draw(renderer, font, "The game takes its graphics and sounds from it, once.", y, 1, white, WINDOW_WIDTH);
	}
	if (problem && !reading)
		letters_draw(renderer, font, problem, y + LETTERS_CELL + LINE_GAP * 3, 1, red, WINDOW_WIDTH);
	SDL_RenderPresent(renderer);
}

/* On macOS, the folder the .app is in (the game's base is the .app's Resources folder: where the player sees
   the game is beside the .app); 0 elsewhere */
static int app_folder(const char *base, char *folder, size_t size)
{
#ifdef SDL_PLATFORM_APPLE
	static const char inside[] = ".app/Contents/Resources/";
	const size_t length = base ? strlen(base) : 0, tail = sizeof(inside) - 1;
	size_t end;

	if (length < tail || length >= size || strcmp(base + length - tail, inside) != 0)
		return 0;
	for (end = length - tail; end > 0 && base[end - 1] != '/'; end--)
		;    /* (back over the .app's name) */
	memcpy(folder, base, end);
	folder[end] = '\0';
	return end > 0;
#else
	(void)base;
	(void)folder;
	(void)size;
	return 0;
#endif
}

/* sonic.zip in 'folder', when it is there and is the set */
static int found_beside(const char *folder, const char *pack)
{
	char path[1024];
	SDL_IOStream *probe;

	if (!folder)
		return 0;
	snprintf(path, sizeof(path), "%s" ROM_SET_FILE, folder);
	if (!(probe = SDL_IOFromFile(path, "rb")))
		return 0;
	SDL_CloseIO(probe);
	return game_data_extract(path, pack) == NULL;
}

int first_run(const char *base, const char *saves, const char *pack)
{
	SDL_Window *window;
	SDL_Renderer *renderer;
	letters font;
	char problem[192] = "", beside_app[1024];
	int done = 0, running = 1;

	if (found_beside(base, pack) || (app_folder(base, beside_app, sizeof(beside_app)) && found_beside(beside_app, pack))
		|| found_beside(saves, pack))
		return 1;
	window = SDL_CreateWindow("SegaSonic The Hedgehog", WINDOW_WIDTH, WINDOW_HEIGHT, 0);
	renderer = window ? SDL_CreateRenderer(window, NULL) : NULL;
	if (!renderer || !letters_load(renderer, base, &font))
	{
		/* (no window or no letters: the message alone) */
		if (renderer)
			SDL_DestroyRenderer(renderer);
		if (window)
			SDL_DestroyWindow(window);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "SegaSonic The Hedgehog",
			"The game takes its graphics and sounds from the arcade ROM set (sonic.zip: MAME's set, Japan, "
			"rev. C). Put sonic.zip beside the game and start it again.", NULL);
		return 0;
	}
	SDL_SetEventEnabled(SDL_EVENT_DROP_FILE, true);
	draw_window(renderer, &font, NULL, 0);
	while (running && !done)
	{
		SDL_Event event;

		if (!SDL_WaitEvent(&event))
			break;
		if (event.type == SDL_EVENT_QUIT)
			running = 0;
		else if (event.type == SDL_EVENT_DROP_FILE)
		{
			const char *failed;

			draw_window(renderer, &font, NULL, 1);
			failed = game_data_extract(event.drop.data, pack);
			if (!failed)
				done = 1;
			else
			{
				/* (a file of the set missing: what the set is not; else what failed) */
				snprintf(problem, sizeof(problem), "%s%s", strncmp(failed, "no ", 3) == 0 ? "Not the set: " : "", failed);
				if (problem[0] >= 'a' && problem[0] <= 'z')
					problem[0] = (char)(problem[0] - 'a' + 'A');
				draw_window(renderer, &font, problem, 0);
			}
		}
		else if (event.type == SDL_EVENT_WINDOW_EXPOSED)
			draw_window(renderer, &font, problem[0] ? problem : NULL, 0);
	}
	letters_free(&font);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	return done;
}

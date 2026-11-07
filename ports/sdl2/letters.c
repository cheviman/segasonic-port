/**
 *    Name:        letters.c
 *    Description: the window's own letters: Kenney Mini (font.bmp) loaded and drawn with SDL
 *    Version:     0.1.0
 *    Created:     23-10-2026
 *    Modified:    07-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    The window has none of the game's letters before the game runs (they are in the ROM): it writes with
 *    Kenney Mini at 16 pixels, white on black, 16 characters a row from 20h; a character is as wide as its
 *    last lit column, and black is see-through.
 */
#include "letters.h"

#include <stdio.h>

#define FONT_FILE     "font.bmp"
#define FONT_FIRST    0x20
#define FONT_LAST     0x7E
#define FONT_ROW      16
#define FONT_SPACING  2        /* pixels between two characters, past the lit columns */
#define SPACE_WIDTH   4

int letters_load(SDL_Renderer *renderer, const char *base, letters *font)
{
	char path[1024];
	SDL_Surface *loaded, *sheet;

	snprintf(path, sizeof(path), "%s" FONT_FILE, base ? base : "");
	if (!(loaded = SDL_LoadBMP(path)))
		return 0;
	sheet = SDL_ConvertSurfaceFormat(loaded, SDL_PIXELFORMAT_ARGB8888, 0);
	SDL_FreeSurface(loaded);
	if (!sheet)
		return 0;
	for (int index = 0; index <= FONT_LAST - FONT_FIRST; index++)
	{
		const int left = (index % FONT_ROW) * LETTERS_CELL, top = (index / FONT_ROW) * LETTERS_CELL;

		font->widths[index] = 0;
		for (int y = top; y < top + LETTERS_CELL && y < sheet->h; y++)
			for (int x = left; x < left + LETTERS_CELL && x < sheet->w; x++)
				if (((const Uint32 *)((const Uint8 *)sheet->pixels + y * sheet->pitch))[x] & 0x00FFFFFFu)
					if (x - left + 1 > font->widths[index])
						font->widths[index] = x - left + 1;
	}
	font->widths[0] = SPACE_WIDTH;
	SDL_SetColorKey(sheet, SDL_TRUE, SDL_MapRGB(sheet->format, 0, 0, 0));
	font->sheet = SDL_CreateTextureFromSurface(renderer, sheet);
	SDL_FreeSurface(sheet);
	return font->sheet != NULL;
}

void letters_free(letters *font)
{
	if (font->sheet)
		SDL_DestroyTexture(font->sheet);
	font->sheet = NULL;
}

int letters_width(const letters *font, const char *text, int scale)
{
	int width = 0;

	for (; *text; text++)
		if (*text >= FONT_FIRST && *text <= FONT_LAST)
			width += (font->widths[*text - FONT_FIRST] + FONT_SPACING) * scale;
	return width > 0 ? width - FONT_SPACING * scale : 0;
}

/* The line with its left at x */
static void draw_from(SDL_Renderer *renderer, const letters *font, const char *text, int x, int y, int scale,
	SDL_Color colour)
{
	SDL_SetTextureColorMod(font->sheet, colour.r, colour.g, colour.b);
	for (; *text; text++)
	{
		SDL_Rect from, to;
		int index;

		if (*text < FONT_FIRST || *text > FONT_LAST)
			continue;
		index = *text - FONT_FIRST;
		from.x = (index % FONT_ROW) * LETTERS_CELL;
		from.y = (index / FONT_ROW) * LETTERS_CELL;
		from.w = font->widths[index] > 0 ? font->widths[index] : 1;
		from.h = LETTERS_CELL;
		to.x = x;
		to.y = y;
		to.w = from.w * scale;
		to.h = LETTERS_CELL * scale;
		if (*text != ' ')
			SDL_RenderCopy(renderer, font->sheet, &from, &to);
		x += (font->widths[index] + FONT_SPACING) * scale;
	}
}

void letters_draw(SDL_Renderer *renderer, const letters *font, const char *text, int y, int scale, SDL_Color colour,
	int width)
{
	draw_from(renderer, font, text, (width - letters_width(font, text, scale)) / 2, y, scale, colour);
}

void letters_draw_at(SDL_Renderer *renderer, const letters *font, const char *text, int centre_x, int y, int scale,
	SDL_Color colour)
{
	draw_from(renderer, font, text, centre_x - letters_width(font, text, scale) / 2, y, scale, colour);
}

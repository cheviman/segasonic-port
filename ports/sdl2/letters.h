/**
 *    Name:        letters.h
 *    Description: the window's own letters, before or beside the game's: Kenney Mini (Kenney Fonts, CC0),
 *                 drawn with SDL (the first run, the touch controls' labels)
 *    Version:     0.1.0
 *    Created:     23-10-2026
 *    Modified:    07-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    font.bmp (tools/make_font.py): a 16 x 16 cell a character from 20h, each as wide as its lit columns.
 */
#ifndef SONIC_SDL2_LETTERS_H
#define SONIC_SDL2_LETTERS_H

#include <SDL.h>

#define LETTERS_CELL 16

typedef struct letters
{
	SDL_Texture *sheet;
	int widths[0x7E - 0x20 + 1];
} letters;

/* font.bmp from the folder 'base' (with its separator): false when it is not there */
int letters_load(SDL_Renderer *renderer, const char *base, letters *font);

void letters_free(letters *font);

/* A line of text, its pixels at 'scale', centred across 'width' pixels with its top at y */
void letters_draw(SDL_Renderer *renderer, const letters *font, const char *text, int y, int scale, SDL_Color colour,
	int width);

/* A line's width in pixels at 'scale' */
int letters_width(const letters *font, const char *text, int scale);

/* ... centred on centre_x */
void letters_draw_at(SDL_Renderer *renderer, const letters *font, const char *text, int centre_x, int y, int scale,
	SDL_Color colour);

#endif

/**
 *    Name:        video.c
 *    Description: Sega System 32 video: the tilemaps, text, bitmap and backdrop layers, the sprite frame
 *                 buffers and the mixer, drawing each frame from the board's memories
 *    Version:     0.1.0
 *    Created:     13-10-2026
 *    Modified:    31-10-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    Behavior follows MAME's sega/segas32_v.cpp (Aaron Giles) pixel for pixel: tools/reference/video
 *    builds that file unmodified, and tools/check_scenarios.py --video compares its frames with these.
 *    MAME's terms are in tools/reference/mame/LICENSE.
 *
 *    How a frame is made: every layer draws a palette offset per pixel (0 is transparent) into a buffer
 *    of its own, through its clip windows. The sprites were drawn before, into the hidden one of two
 *    frame buffers, when the board sent the list. The mixer then takes, pixel by pixel, the topmost
 *    layer that is not transparent in the order its priority registers give, maybe blends it with the
 *    one below, and looks the color up in the palette.
 *
 *    Video RAM holds the tilemap pages (32 x 16 cells of a 16x16 tile, 0x400 bytes each), the text cells
 *    and their graphics, the bitmap layer, the line tables, and the registers at 0x31FF00.
 */
#include "s32/video.h"

#include "s32/machine.h"
#include "s32/workers.h"

#include <stdbool.h>
#include <string.h>

#define WIDTH   S32_VIDEO_MAX_WIDTH
#define HEIGHT  S32_VIDEO_HEIGHT

/* The layers in the mixer's order: its registers come in this order */
enum
{
	LAYER_TEXT,
	LAYER_NBG0,
	LAYER_NBG1,
	LAYER_NBG2,
	LAYER_NBG3,
	LAYER_BITMAP,
	LAYER_SPRITES,
	LAYER_BACKDROP,
	LAYERS
};

/* A rectangle with inclusive edges (the clip windows keep one past their right and bottom edges) */
typedef struct area
{
	int left;
	int right;
	int top;
	int bottom;
} area;

/* What a layer drew: a palette offset per pixel, and the rows where everything is transparent */
typedef struct layer
{
	uint16_t pixels[HEIGHT][WIDTH];
	bool empty[HEIGHT];
} layer;

/* Where the clip windows turn a layer on and off along each row: lists of columns that start at the
   left edge and end one past the right edge, one for each set of windows (0-31) and one for each row
   whose windows come from the line table (32 + the row) */
typedef struct clip_runs
{
	uint8_t list_of_row[256];
	uint16_t lists[256][16];
} clip_runs;

/* The source of one row of a layer: its pixel at a screen column */
typedef uint16_t (*pixel_source)(const void *row, int x);

/* A layer in the mixer's list */
typedef struct mix_layer
{
	uint16_t palette_base;
	uint16_t sprite_blend;      /* sprite groups this layer blends with */
	uint8_t blend;              /* layers this one blends with */
	uint8_t index;              /* LAYER_ */
	uint8_t priority;           /* the register's priority << 3 | the layer's own rank */
	uint8_t shift;              /* of the pixel, before the palette base is added */
	uint8_t offsets;            /* color offsets 0 or 1, or none (2) */
} mix_layer;

/* What one sprite draws with */
typedef struct sprite_drawing
{
	uint16_t *row;
	const area *inside;         /* the clip it draws in */
	const area *outside;        /* the clip it leaves alone, on the rows it covers */
	bool outside_row;
	bool indirect;              /* pens go through a table of 16 colors */
	bool shadow;                /* darkens what is under it instead */
	bool bits8;
	int color;
	int transparent_bits;       /* an indirect color with all these bits set is transparent */
	uint16_t table[16];
} sprite_drawing;

static struct
{
	const uint8_t *tiles;
	uint32_t tile_count;
	const uint8_t *sprite_rom;
	uint32_t sprite_rom_words;
	uint32_t sprite_banks;                  /* of 4 MB */

	area visible;                           /* 320 or 416 x 224 */
	bool wide;                              /* s32_video_set_wide */
	int side;                               /* this frame's: S32_VIDEO_WIDE_SIDE when wide, else 0 */
	int sprite_side;                        /* the sprite list's: every sprite that much to the right */
	layer layers[LAYERS];                   /* the sprites' entry is unused: they have their own buffers */
	uint16_t sprites[2][HEIGHT][WIDTH];     /* frame buffers: one is shown, the list draws into the other */
	int shown_sprites;
	uint32_t frame[HEIGHT][WIDTH];
	clip_runs runs;
} video;

static const uint16_t transparent_row[WIDTH] = { 0 };

/* ---- the board's memories ---- */

static bool bit(uint32_t value, int number)
{
	return (value >> number) & 1;
}

static int sign_extend(uint32_t value, int bits)
{
	const uint32_t sign = 1u << (bits - 1);
	return (int)((value & ((sign << 1) - 1)) ^ sign) - (int)sign;
}

/* x >> count, rounding down for negative numbers too */
static int shift_down(int value, int count)
{
	return value >= 0 ? value >> count : ~(~value >> count);
}

static uint16_t video_word(uint32_t index)
{
	const uint8_t *at = &s32_board.video_ram[(index & 0xffff) * 2];
	return (uint16_t)(at[0] | at[1] << 8);
}

/* A register of the tilemap chip, by its offset from 0x31FF00 */
static uint16_t video_register(uint32_t offset)
{
	return video_word((0x1ff00 + offset) / 2);
}

/* A mixer register, by its offset in the mixer's window */
static uint16_t mixer_register(uint32_t offset)
{
	return s32_board.mixer[offset / 2];
}

static uint16_t sprite_word(uint32_t index)
{
	const uint8_t *at = &s32_board.sprite_ram[(index & 0xffff) * 2];
	return (uint16_t)(at[0] | at[1] << 8);
}

static uint32_t big_endian_word(const uint8_t *at)
{
	return (uint32_t)at[0] << 24 | (uint32_t)at[1] << 16 | (uint32_t)at[2] << 8 | at[3];
}

static void clip_area(area *target, const area *bounds)
{
	if (bounds->left > target->left)
		target->left = bounds->left;
	if (bounds->right < target->right)
		target->right = bounds->right;
	if (bounds->top > target->top)
		target->top = bounds->top;
	if (bounds->bottom < target->bottom)
		target->bottom = bounds->bottom;
}

/* ---- tiles and text cells ---- */

/* The pen (0-15) of a pixel of a 16x16 tile: rows of 8 bytes, two pens a byte with the high nibble
   first, and the byte pairs of each half of the row swapped */
static int tile_pen(uint32_t tile, int x, int y)
{
	static const uint8_t byte_of_column[16] = { 0, 0, 2, 2, 1, 1, 3, 3, 4, 4, 6, 6, 5, 5, 7, 7 };
	const uint8_t value = video.tiles[(size_t)(tile % video.tile_count) * 128 + y * 8 + byte_of_column[x]];

	return (x & 1) ? (value & 0x0f) : (value >> 4);
}

/* A pixel of a tilemap page (32 x 16 cells, 512 x 256 pixels). A cell is the tile number (13 bits, the
   bank above them), its color (bits 4-12, over the number) and its flips (bit 14 x, bit 15 y). */
static const uint16_t *own_cell_words;    /* s32_video_set_cells */
static const uint8_t *own_cell_marks;

static uint16_t page_pixel(int page, int bank, int y, int x)
{
	const uint32_t index = (uint32_t)page << 9 | (uint32_t)(y >> 4) << 5 | (uint32_t)(x >> 4);
	const uint16_t cell = own_cell_marks && own_cell_marks[index] ? own_cell_words[index] : video_word(index);
	const uint32_t tile = (uint32_t)bank << 13 | (cell & 0x1fff);
	const int column = (cell & 0x4000) ? 15 - (x & 15) : (x & 15);
	const int line = (cell & 0x8000) ? 15 - (y & 15) : (y & 15);
	const int pen = tile_pen(tile, column, line);

	return pen ? (uint16_t)(((cell >> 4) & 0x1ff) << 4 | pen) : 0;
}

/* A pixel of the text layer (64 x 32 cells of 8x8, a 512 x 256 map). The cells' page and the bank of
   their graphics come from register 0x5C; the graphics are in video RAM too, 32 bytes a character,
   four bytes a row, the high nibble first. */
static uint16_t text_pixel(int y, int x)
{
	const uint16_t control = video_register(0x5c);
	const uint16_t cell = video_word((uint32_t)((control >> 4) & 0x1f) << 11 | (uint32_t)(y >> 3) << 6 | (uint32_t)(x >> 3));
	const uint32_t character = (uint32_t)(control & 7) << 9 | (cell & 0x1ff);
	const uint8_t value = s32_board.video_ram[character * 32 + (y & 7) * 4 + (x & 7) / 2];
	const int pen = (x & 1) ? (value & 0x0f) : (value >> 4);

	return pen ? (uint16_t)(((cell >> 9) & 0x7f) << 4 | pen) : 0;
}

/* ---- clip windows ---- */

/* Window 0-4 (0x60 + 8 each: left, top, right, bottom), with one past its right and bottom edges */
static void window_of(int number, bool flip, area *window)
{
	const uint32_t at = 0x60 + 8 * (uint32_t)number;
	const int left = video_register(at) & 0x1ff;
	const int top = video_register(at + 2) & 0xff;
	const int right = (video_register(at + 4) & 0x1ff) + 1;
	const int bottom = (video_register(at + 6) & 0xff) + 1;

	if (!flip)
	{
		window->left = left + video.side;
		window->right = right + video.side;
		if (video.side && left == 0 && right >= 320)
		{
			window->left = 0;
			window->right = video.visible.right + 1;
		}
		window->top = top;
		window->bottom = bottom;
	}
	else
	{
		window->left = video.visible.right + 1 - right;
		window->right = video.visible.right + 1 - left;
		window->top = video.visible.bottom + 1 - bottom;
		window->bottom = video.visible.bottom + 1 - top;
	}
}

static void sort_by_left(const area *windows, int *order)
{
	for (int index = 1; index < 5; index++)
	{
		const int key = order[index];
		int place = index - 1;

		while (place >= 0 && windows[order[place]].left > windows[key].left)
		{
			order[place + 1] = order[place];
			place--;
		}
		order[place + 1] = key;
	}
}

/* The list for a set of windows: taken from left to right, a window that starts inside the last run
   stretches it */
static void merge_windows(uint16_t *list, const area *windows, const int *order, int set, const area *edges)
{
	uint16_t *end = list;

	*end++ = (uint16_t)edges->left;
	for (int index = 0; index < 5; index++)
	{
		const area *window = &windows[order[index]];

		if (!bit((uint32_t)set, order[index]))
			continue;
		if (end != list + 1 && window->left <= end[-1])
		{
			if (window->right > end[-1])
				end[-1] = (uint16_t)window->right;
		}
		else
		{
			*end++ = (uint16_t)window->left;
			*end++ = (uint16_t)window->right;
		}
	}
	*end = (uint16_t)edges->right;
}

/* Windows 2 and 3 can take their columns from the line table instead, row by row (0xFFFF: none) */
static void line_window(int number, int line, bool flip, uint32_t table, const area *edges, area *window, int *crossing)
{
	const uint16_t left = video_word(table + 0x100 * (uint32_t)(number - 2) + (uint32_t)line);
	const uint16_t right = video_word(table + 0x200 + 0x100 * (uint32_t)(number - 2) + (uint32_t)line);

	if (left == 0xffff || right == 0xffff)
		*crossing &= ~(1 << number);
	else if (!flip)
	{
		window->left = (left & 0x1ff) + video.side;
		window->right = (right & 0x1ff) + 1 + video.side;
		if (video.side && (left & 0x1ff) == 0 && (right & 0x1ff) + 1 >= 320)
		{
			window->left = 0;
			window->right = video.visible.right + 1;
		}
	}
	else
	{
		window->left = video.visible.right + 1 - ((right & 0x1ff) + 1);
		window->right = video.visible.right + 1 - (left & 0x1ff);
	}
	clip_area(window, edges);
	if (window->left >= window->right)
		*crossing &= ~(1 << number);
}

/* A layer's runs: 'selected' picks windows 0-4. The first run of each row is drawn when the layer draws
   outside its windows (or has them off), and the runs alternate from there. */
static bool find_clip_runs(bool enabled, bool outside, int selected, clip_runs *runs)
{
	const bool flip = bit(video_register(0x00), 9);
	const uint16_t line_control = video_register(0x04);
	const bool line_windows = bit(line_control, 4) || bit(line_control, 5);
	const area edges = { video.visible.left, video.visible.right + 1, video.visible.top, video.visible.bottom + 1 };
	area windows[5];
	int order[5];

	memset(runs, 0, sizeof(*runs));
	runs->lists[0][0] = (uint16_t)edges.left;
	runs->lists[0][1] = (uint16_t)edges.right;
	if (!enabled)
		return true;

	for (int number = 0; number < 5; number++)
	{
		window_of(number, flip, &windows[number]);
		clip_area(&windows[number], &edges);
		order[number] = number;
	}
	sort_by_left(windows, order);
	for (int set = 1; set < 32; set++)
		if (set & selected)
			merge_windows(runs->lists[set], windows, order, set, &edges);

	for (int y = edges.top; y < edges.bottom; y++)
	{
		int crossing = 0;
		area row_windows[5];
		int row_order[5];

		for (int number = 0; number < 5; number++)
			if (bit((uint32_t)selected, number) && y >= windows[number].top && y < windows[number].bottom)
				crossing |= 1 << number;
		if (!line_windows)
		{
			runs->list_of_row[y] = (uint8_t)crossing;
			continue;
		}

		for (int number = 0; number < 5; number++)
		{
			row_windows[number] = windows[number];
			row_order[number] = number;
		}
		for (int number = 2; number <= 3; number++)
			if (bit(line_control, number + 2) && bit((uint32_t)selected, number))
				line_window(number, flip ? video.visible.bottom - y : y, flip, (uint32_t)(line_control >> 10) * 0x400,
					&edges, &row_windows[number], &crossing);
		sort_by_left(row_windows, row_order);
		merge_windows(runs->lists[32 + y], row_windows, row_order, crossing, &edges);
		runs->list_of_row[y] = (uint8_t)(32 + y);
	}
	return outside;
}

/* One row of a layer through its runs, which alternate between drawn and cleared starting with 'drawn'.
   Overlapping windows can make a run start before the last one ended: it is drawn again. */
static void draw_row(layer *out, int y, const uint16_t *runs, bool drawn, pixel_source pixel, const void *row)
{
	uint16_t *pixels = out->pixels[y];
	int transparent = 0;

	for (;; runs++, drawn = !drawn)
	{
		if (drawn)
		{
			for (int x = runs[0]; x < runs[1] && x < WIDTH; x++)
			{
				pixels[x] = pixel(row, x);
				transparent += pixels[x] == 0;
			}
		}
		else
		{
			for (int x = runs[0]; x < runs[1] && x < WIDTH; x++)
				pixels[x] = 0;
			transparent += runs[1] - runs[0];
		}
		if (runs[1] > video.visible.right)
			break;
	}
	out->empty[y] = transparent == video.visible.right + 1 - video.visible.left;
}

/* ---- the layers ---- */

/* The four pages of tilemap 0-3 (upper left, upper right, lower left, lower right) and its tile bank */
static int pages_of(int number, int *pages)
{
	const uint16_t upper = video_register(0x40 + 4 * (uint32_t)number);
	const uint16_t lower = video_register(0x42 + 4 * (uint32_t)number);

	pages[0] = upper & 0x7f;
	pages[1] = (upper >> 8) & 0x7f;
	pages[2] = lower & 0x7f;
	pages[3] = (lower >> 8) & 0x7f;
	return (s32_board.tilebank_external & 1) << 1 | ((video_register(0x00) >> 10) & 1);
}

/* Register 0x00 flips tilemap 0-3 against the whole screen's flip (bit 9); bit 8 keeps them from
   flipping vertically */
static void flips_of(int number, bool *flip_x, bool *flip_y)
{
	const uint16_t control = video_register(0x00);
	const bool screen_flip = bit(control, 9);
	const bool layer_flip = bit(control, number);

	*flip_x = layer_flip ? !screen_flip : screen_flip;
	*flip_y = (layer_flip && !bit(control, 8)) ? !screen_flip : screen_flip;
}

typedef struct zoomed_row
{
	int pages[2];       /* left and right page of the row's half */
	int bank;
	int line;           /* the row within the pages */
	uint32_t x_start;   /* source column at the left edge, 12.20 */
	uint32_t x_step;
} zoomed_row;

static uint16_t zoomed_pixel(const void *source, int x)
{
	const zoomed_row *row = (const zoomed_row *)source;
	const uint32_t column = row->x_start + row->x_step * (uint32_t)(x - video.visible.left);

	return page_pixel(row->pages[(column >> 29) & 1], row->bank, row->line, (column >> 20) & 0x1ff);
}

/* What NBG0 or NBG1 draws with, the same on every row: the source at the top row, and how it moves */
typedef struct zoomed_layer
{
	int number;
	int pages[4];
	int bank;
	bool first_drawn;
	uint32_t x_start;
	uint32_t x_step;
	uint32_t source_y;  /* at the top row, 12.20 */
	uint32_t y_step;
} zoomed_layer;

static void draw_zoomed_rows(void *context, int first, int last)
{
	const zoomed_layer *zoomed = (const zoomed_layer *)context;

	for (int y = first; y <= last; y++)
	{
		const uint32_t source_y = zoomed->source_y + (uint32_t)(y - video.visible.top) * zoomed->y_step;
		const int half = (source_y >> 27) & 2;
		const zoomed_row row = {
			{ zoomed->pages[half], zoomed->pages[half + 1] }, zoomed->bank, (source_y >> 20) & 0xff,
			zoomed->x_start, zoomed->x_step
		};

		draw_row(&video.layers[LAYER_NBG0 + zoomed->number], y, video.runs.lists[video.runs.list_of_row[y]],
			zoomed->first_drawn, zoomed_pixel, &row);
	}
}

/* NBG0 and NBG1: scrolled (with a fraction) and zoomed about a center. The steps are screen pixels per
   source pixel (0x200 = 1); the source moves by their reciprocal, in 12.20. */
static void draw_zoomed(int number)
{
	const uint32_t scroll = 0x10 + 8 * (uint32_t)number;
	const uint32_t center = 0x30 + 4 * (uint32_t)number;
	int pages[4];
	const int bank = pages_of(number, pages);
	bool flip_x;
	bool flip_y;
	int x_zoom = video_register(0x50 + 4 * (uint32_t)number) & 0xfff;
	int y_zoom = bit(video_register(0x00), 14) ? video_register(0x52 + 4 * (uint32_t)number) & 0xfff : x_zoom;
	uint32_t x_step;
	uint32_t y_step;
	uint32_t x_start;
	uint32_t source_y;
	const bool first_drawn = find_clip_runs(bit(video_register(0x02), 11 + number), bit(video_register(0x02), 6 + number),
		(video_register(0x06) >> (4 * number)) & 0x0f, &video.runs);

	flips_of(number, &flip_x, &flip_y);
	if (x_zoom < 0x80)
		x_zoom = 0x80;
	if (y_zoom < 0x80)
		y_zoom = 0x80;
	x_step = (0x200u << 20) / (uint32_t)x_zoom;
	y_step = (0x200u << 20) / (uint32_t)y_zoom;

	/* the scroll with its fraction, less the center (one bit more of it while zoomed) */
	x_start = (uint32_t)(video_register(scroll + 2) & 0x3ff) << 20;
	x_start += (uint32_t)(video_register(scroll) & 0xff00) << 4;
	source_y = (uint32_t)(video_register(scroll + 6) & 0x1ff) << 20;
	source_y += (uint32_t)(video_register(scroll + 4) & 0xfe00) << 4;
	x_start -= (uint32_t)sign_extend(video_register(center), x_zoom != 0x200 ? 10 : 9) * x_step;
	source_y -= (uint32_t)sign_extend(video_register(center + 2), y_zoom != 0x200 ? 10 : 9) * y_step;
	x_start += (uint32_t)(video.visible.left - video.side) * x_step;
	source_y += (uint32_t)video.visible.top * y_step;
	if (flip_y)
	{
		source_y += (uint32_t)(video.visible.bottom - 2 * video.visible.top) * y_step;
		y_step = 0u - y_step;
	}
	if (flip_x)
	{
		x_start += (uint32_t)(video.visible.right - 2 * video.visible.left) * x_step;
		x_step = 0u - x_step;
	}

	{
		const zoomed_layer zoomed = {
			number, { pages[0], pages[1], pages[2], pages[3] }, bank, first_drawn, x_start, x_step, source_y, y_step
		};

		s32_workers_rows(draw_zoomed_rows, (void *)&zoomed, video.visible.top, video.visible.bottom);
	}
}

typedef struct scrolled_row
{
	int pages[2];
	int bank;
	int line;
	int x_start;        /* source column at the left edge */
	int x_step;
} scrolled_row;

static uint16_t scrolled_pixel(const void *source, int x)
{
	const scrolled_row *row = (const scrolled_row *)source;
	const uint32_t column = (uint32_t)(row->x_start + row->x_step * (x - video.visible.left));

	return page_pixel(row->pages[(column >> 9) & 1], row->bank, row->line, column & 0x1ff);
}

/* What NBG2 or NBG3 draws with, the same on every row */
typedef struct scrolled_layer
{
	int number;
	uint32_t table;
	bool row_scroll;
	bool row_select;
	int x_scroll;
	int y_scroll;
	int pages[4];
	int bank;
	bool flip_x;
	bool flip_y;
	bool first_drawn;
} scrolled_layer;

static void draw_scrolled_rows(void *context, int first, int last)
{
	const scrolled_layer *scrolled = (const scrolled_layer *)context;

	for (int y = first; y <= last; y++)
	{
		const int line = scrolled->flip_y ? video.visible.bottom - y : y;
		int source_y = scrolled->y_scroll + line;
		scrolled_row row;

		row.x_start = (scrolled->flip_x ? video.visible.right : video.visible.left) + scrolled->x_scroll - video.side;
		row.x_step = scrolled->flip_x ? -1 : 1;
		if (scrolled->row_scroll)
			row.x_start += video_word(scrolled->table + (uint32_t)line) & 0x3ff;
		if (scrolled->row_select)
			source_y = (scrolled->y_scroll + video_word(scrolled->table + 0x200 + (uint32_t)line)) & 0x1ff;
		row.pages[0] = scrolled->pages[(source_y >> 7) & 2];
		row.pages[1] = scrolled->pages[((source_y >> 7) & 2) + 1];
		row.bank = scrolled->bank;
		row.line = source_y & 0xff;
		draw_row(&video.layers[LAYER_NBG0 + scrolled->number], y, video.runs.lists[video.runs.list_of_row[y]],
			scrolled->first_drawn, scrolled_pixel, &row);
	}
}

/* NBG2 and NBG3: scrolled, and row by row the line table can add to the column (row scroll) or pick
   the source row (row select). Register 0x04 turns those on; its top bits place the table. */
static void draw_scrolled(int number)
{
	const uint16_t line_control = video_register(0x04);
	const bool line_windows = bit(line_control, number + 2);
	scrolled_layer scrolled;

	scrolled.number = number;
	scrolled.table = (uint32_t)(line_control >> 10) * 0x400 + 0x100 * (uint32_t)(number - 2);
	scrolled.row_scroll = bit(line_control, number - 2) && !line_windows;
	scrolled.row_select = bit(line_control, number) && !line_windows;
	scrolled.x_scroll = (video_register(0x12 + 8 * (uint32_t)number) & 0x3ff) - (video_register(0x30 + 4 * (uint32_t)number) & 0x1ff);
	scrolled.y_scroll = video_register(0x16 + 8 * (uint32_t)number) & 0x1ff;
	scrolled.bank = pages_of(number, scrolled.pages);
	scrolled.first_drawn = find_clip_runs(bit(video_register(0x02), 11 + number), bit(video_register(0x02), 6 + number),
		(video_register(0x06) >> (4 * number)) & 0x0f, &video.runs);
	flips_of(number, &scrolled.flip_x, &scrolled.flip_y);
	s32_workers_rows(draw_scrolled_rows, &scrolled, video.visible.top, video.visible.bottom);
}

/* The text layer, with no scroll and no windows; the screen's flip turns the whole 512 x 256 map */
static void draw_text_rows(void *context, int first, int last)
{
	const bool flip = *(const bool *)context;

	for (int y = first; y <= last; y++)
		for (int x = video.visible.left; x <= video.visible.right; x++)
		{
			const int column = x - video.side;

			if (column < 0 || column >= (video.side ? 320 : video.visible.right + 1))
				video.layers[LAYER_TEXT].pixels[y][x] = 0;
			else
				video.layers[LAYER_TEXT].pixels[y][x] = flip ? text_pixel(255 - y, 511 - column) : text_pixel(y, column);
		}
}

static void draw_text(void)
{
	bool flip = bit(video_register(0x00), 9);

	s32_workers_rows(draw_text_rows, &flip, video.visible.top, video.visible.bottom);
}

typedef struct bitmap_row
{
	uint32_t start;     /* the row in video RAM: a byte offset (8 bits a pixel) or a word (4 bits) */
	int x_scroll;
	int color;
	bool bits8;
} bitmap_row;

static uint16_t bitmap_pixel(const void *source, int x)
{
	const bitmap_row *row = (const bitmap_row *)source;
	const int column = (x - video.side + row->x_scroll) & 0x1ff;
	int pixel;

	if (row->bits8)
	{
		pixel = s32_board.video_ram[row->start + (uint32_t)column] + row->color;
		return (pixel & 0xff) ? (uint16_t)pixel : 0;
	}
	pixel = ((video_word(row->start + (uint32_t)column / 4) >> (4 * (column & 3))) & 0x0f) + row->color;
	return (pixel & 0x0f) ? (uint16_t)pixel : 0;
}

/* What the bitmap layer draws with, the same on every row */
typedef struct bitmap_layer
{
	bool bits8;
	int x_scroll;
	int y_scroll;
	int color;
	bool first_drawn;
} bitmap_layer;

static void draw_bitmap_rows(void *context, int first, int last)
{
	const bitmap_layer *bitmap = (const bitmap_layer *)context;

	for (int y = first; y <= last; y++)
	{
		const bitmap_row row = {
			bitmap->bits8 ? 512 * (uint32_t)((y + bitmap->y_scroll) & 0xff) : 128 * (uint32_t)((y + bitmap->y_scroll) & 0x1ff),
			bitmap->x_scroll, bitmap->color, bitmap->bits8
		};

		draw_row(&video.layers[LAYER_BITMAP], y, video.runs.lists[video.runs.list_of_row[y]], bitmap->first_drawn,
			bitmap_pixel, &row);
	}
}

/* The bitmap layer: 512 pixels a row from the start of video RAM, 4 or 8 bits each (register 0x00
   bit 11), scrolled by registers 0x88 and 0x8A, its palette from 0x8C; window 4 clips it */
static void draw_bitmap(void)
{
	bitmap_layer bitmap;

	bitmap.bits8 = bit(video_register(0x00), 11);
	bitmap.x_scroll = video_register(0x88) & 0x1ff;
	bitmap.y_scroll = video_register(0x8a) & 0x1ff;
	bitmap.color = (video_register(0x8c) << 4) & 0x1fff0 & ~((1 << (bitmap.bits8 ? 8 : 4)) - 1);
	bitmap.first_drawn = find_clip_runs(bit(video_register(0x02), 15), bit(video_register(0x02), 10), 0x10, &video.runs);
	s32_workers_rows(draw_bitmap_rows, &bitmap, video.visible.top, video.visible.bottom);
}

/* The backdrop: one color for the screen, or one per line (register 0x5E bit 15), wrapping at 511 */
static void draw_backdrop(void)
{
	const uint16_t control = video_register(0x5e);
	const bool flip = bit(video_register(0x00), 9);

	for (int y = video.visible.top; y <= video.visible.bottom; y++)
	{
		const int color = (control & 0x1e00) + (bit(control, 15) ? (control + y) & 0x1ff : 0);
		uint16_t *row = video.layers[LAYER_BACKDROP].pixels[flip ? video.visible.bottom - y : y];

		for (int x = video.visible.left; x <= video.visible.right; x++)
			row[x] = (uint16_t)color;
	}
}

/* Every layer the registers leave on (0x02 and 0x8E turn them off, 0x00 bits 12-13 too for NBG2-3);
   the backdrop is always there. The layers drawn, as bits by LAYER_. */
s32_tile_observer s32_video_tile_observer = NULL;

/* Every cell of the pages tilemap 'number' shows, to s32_video_tile_observer */
static void observe_tilemap(int number)
{
	int pages[4];
	const int bank = pages_of(number, pages);

	for (int index = 0; index < 4; index++)
	{
		bool seen = false;

		for (int earlier = 0; earlier < index; earlier++)
			seen = seen || pages[earlier] == pages[index];
		if (seen)
			continue;
		for (uint32_t cell = 0; cell < 32 * 16; cell++)
		{
			const uint16_t word = video_word((uint32_t)pages[index] << 9 | cell);

			s32_video_tile_observer(number, (uint32_t)bank << 13 | (word & 0x1fff), (word >> 4) & 0x1ff);
		}
	}
}

static int draw_layers(void)
{
	const uint16_t control = video_register(0x00);
	const uint16_t off = video_register(0x02);
	const uint16_t more_off = video_register(0x8e);
	int drawn = 0;

	if (!bit(off, 0) && !bit(more_off, 1))
	{
		draw_zoomed(0);
		if (s32_video_tile_observer != NULL)
			observe_tilemap(0);
		drawn |= 1 << LAYER_NBG0;
	}
	if (!bit(off, 1) && !bit(more_off, 2))
	{
		draw_zoomed(1);
		if (s32_video_tile_observer != NULL)
			observe_tilemap(1);
		drawn |= 1 << LAYER_NBG1;
	}
	if (!bit(off, 2) && !bit(more_off, 3) && !bit(control, 12))
	{
		draw_scrolled(2);
		if (s32_video_tile_observer != NULL)
			observe_tilemap(2);
		drawn |= 1 << LAYER_NBG2;
	}
	if (!bit(off, 3) && !bit(more_off, 4) && !bit(control, 13))
	{
		draw_scrolled(3);
		if (s32_video_tile_observer != NULL)
			observe_tilemap(3);
		drawn |= 1 << LAYER_NBG3;
	}
	if (!bit(off, 4) && !bit(more_off, 0))
	{
		draw_text();
		drawn |= 1 << LAYER_TEXT;
	}
	if (!bit(off, 5) && !bit(more_off, 5))
	{
		draw_bitmap();
		drawn |= 1 << LAYER_BITMAP;
	}
	draw_backdrop();
	return drawn;
}

/* ---- sprites ---- */

s32_sprite_observer s32_video_sprite_observer = NULL;

static void put_sprite_pixel(const sprite_drawing *sprite, int x, int pen, int skipped_pen)
{
	int pixel;

	if (x < sprite->inside->left || x > sprite->inside->right || pen == skipped_pen)
		return;
	if (sprite->outside_row && x >= sprite->outside->left && x <= sprite->outside->right)
		return;
	if (!sprite->indirect)
	{
		if (pen == 0)
			return;
		pixel = sprite->color | pen;
	}
	else
	{
		pixel = sprite->bits8 ? sprite->table[pen >> 4] | (pen & 0x0f) : sprite->table[pen];
		if ((pixel & sprite->transparent_bits) == sprite->transparent_bits)
			return;
	}
	if (!sprite->shadow)
		sprite->row[x] = (uint16_t)pixel;
	else
		sprite->row[x] &= 0x7fff;
}

/* A 32-bit word of sprite graphics: from the ROM bank, or from sprite RAM itself */
static uint32_t sprite_graphics(bool from_ram, uint32_t bank, uint32_t index)
{
	if (from_ram)
		return big_endian_word(&s32_board.sprite_ram[(index & 0x7fff) * 4]);
	return big_endian_word(&video.sprite_rom[(size_t)(((bank << 20) | (index & 0xfffff)) % video.sprite_rom_words) * 4]);
}

/* One row of a sprite: words of 8 pens (4 bits) or 4 (8 bits), each pen repeated or skipped by the
   zoom. The first and last pen of a word are transparent at all ones (unless the sprite is opaque),
   and a word that ends with them ends the row. */
static void draw_sprite_row(const sprite_drawing *sprite, bool from_ram, uint32_t bank, uint32_t address,
	int x, int x_end, int x_delta, int x_zoom, int end_pen)
{
	const int bits = sprite->bits8 ? 8 : 4;
	const int pens = 32 / bits;
	const int pen_mask = (1 << bits) - 1;
	int x_accumulator = 0;

	while (x != x_end)
	{
		const uint32_t word = sprite_graphics(from_ram, bank, address++);
		int pen = 0;

		for (int index = 0; index < pens; index++)
		{
			const int skipped_pen = (index == 0 || index == pens - 1) ? end_pen : 0;

			pen = (int)(word >> (32 - bits * (index + 1))) & pen_mask;
			while (x_accumulator < 0x10000 && x != x_end)
			{
				put_sprite_pixel(sprite, x, pen, skipped_pen);
				x += x_delta;
				x_accumulator += x_zoom;
			}
			x_accumulator -= 0x10000;
		}
		if (end_pen != 0 && pen == pen_mask)
			break;
	}
}

/* A sprite entry (8 words): flags, source height and width, screen height (and bank bits), screen
   width, y, x, the graphics' offset in the bank, and the color. Hands back how many more entries it
   takes: an indirect table kept in the list takes two. */
static int draw_sprite(uint32_t entry, int x_offset, int y_offset, const area *inside, const area *outside)
{
	static const uint16_t transparent_bits[4][4] = {
		{ 0x7fff, 0x3fff, 0x1fff, 0x0fff },
		{ 0x3fff, 0x1fff, 0x0fff, 0x07ff },
		{ 0x3fff, 0x1fff, 0x0fff, 0x07ff },
		{ 0x1fff, 0x0fff, 0x07ff, 0x03ff }
	};
	const uint8_t *latched = s32_board.sprite_control_latched;
	const uint16_t flags = sprite_word(entry);
	const uint16_t size = sprite_word(entry + 1);
	const uint16_t height_word = sprite_word(entry + 2);
	const uint16_t width_word = sprite_word(entry + 3);
	const bool table_inline = bit(flags, 12);
	const bool from_ram = bit(flags, 10);
	const bool flip_y = bit(flags, 7);
	const bool flip_x = bit(flags, 6);
	const int source_height = size >> 8;
	const int height = height_word & 0x3ff;
	const int width = width_word & 0x3ff;
	const uint32_t bank = ((width_word & 0x0800) >> 11 | (width_word & 0x4000) >> 13) % (video.sprite_banks ? video.sprite_banks : 1);
	const int taken = (bit(flags, 13) && table_inline) ? 2 : 0;
	sprite_drawing sprite;
	int source_width;
	int end_pen;
	int x;
	int y;
	int x_delta = 1;
	int y_delta = 1;
	int x_end;
	int y_end;
	int x_zoom;
	int y_zoom;
	int y_accumulator = 0;
	uint32_t address = sprite_word(entry + 6) | (uint32_t)(height_word & 0xf000) << 4;

	sprite.inside = inside;
	sprite.outside = outside;
	sprite.indirect = bit(flags, 13);
	sprite.shadow = bit(flags, 11) && bit(latched[5], 0);
	sprite.bits8 = bit(flags, 9);
	sprite.color = 0x8000 | (sprite_word(entry + 7) & (sprite.bits8 ? 0x7f00 : 0x7ff0));
	source_width = sprite.bits8 ? (size & 0x3f) : ((size >> 1) & 0x3f);
	end_pen = bit(flags, 8) ? 0 : (sprite.bits8 ? 0xff : 0x0f);
	if (source_width == 0 || source_height == 0 || width == 0 || height == 0)
		return taken;

	sprite.transparent_bits = transparent_bits[latched[4] & 3][latched[5] & 3];
	if (sprite.bits8)
		sprite.transparent_bits &= 0xfff0;
	if (sprite.indirect)
	{
		/* the table follows the entry, or is an entry of the list picked by the color word */
		const uint32_t table = table_inline ? entry + 8 : 8 * (uint32_t)(sprite_word(entry + 7) & 0x1fff);

		for (int index = 0; index < 16; index++)
			sprite.table[index] = (uint16_t)((sprite_word(table + (uint32_t)index) & (sprite.bits8 ? 0xfff0 : 0xffff)) |
				(bit(latched[5], 0) ? 0x8000 : 0));
	}

	if (s32_video_sprite_observer != NULL && !from_ram)
	{
		/* the pixel each pen puts in the buffer, as put_sprite_pixel works it out */
		uint16_t pixels[256];
		const int pens = sprite.bits8 ? 256 : 16;

		for (int pen = 0; pen < pens; pen++)
			pixels[pen] = (uint16_t)(!sprite.indirect ? (sprite.color | pen)
				: (sprite.bits8 ? sprite.table[pen >> 4] | (pen & 0x0f) : sprite.table[pen]));
		s32_video_sprite_observer(bank << 20 | (address & 0xfffff), source_width, source_height, sprite.bits8,
			end_pen == 0, pixels);
	}

	x_zoom = (((sprite.bits8 ? 4 : 8) * source_width) << 16) / width;
	y_zoom = (source_height << 16) / height;

	/* place it: the list's offset, then its alignment (0 or 3 centered, 1 by the far edge, 2 by the near) */
	x = sign_extend(sprite_word(entry + 5), 12) + (bit(flags, 4) ? x_offset : 0) + video.sprite_side;
	y = sign_extend(sprite_word(entry + 4), 12) + (bit(flags, 5) ? y_offset : 0);
	switch (flags & 3)
	{
		case 0:
		case 3: x -= (width - 1) / 2; break;
		case 1: x -= width - 1; break;
		default: break;
	}
	switch ((flags >> 2) & 3)
	{
		case 0:
		case 3: y -= (height - 1) / 2; break;
		case 1: y -= height - 1; break;
		default: break;
	}
	if (flip_x)
	{
		x += width - 1;
		x_delta = -1;
	}
	if (flip_y)
	{
		y += height - 1;
		y_delta = -1;
	}
	x_end = x + x_delta * width;
	y_end = y + y_delta * height;

	/* rows end at the sprite's width, or at the clip. (MAME tests a flipped sprite's end against the clip's
	   right edge instead of its left one, so the sprite runs on to the left edge reading whatever follows
	   it in the ROM: an opaque one, which ignores end codes, leaves copies of the next frames there. The
	   reference, tools/reference/mame/sega/segas32_v.cpp, has the same line fixed.) */
	if (x_delta > 0 && x_end > inside->right)
	{
		x_end = inside->right + 1;
		if (x >= x_end)
			return taken;
	}
	if (x_delta < 0 && x_end < inside->left)
	{
		x_end = inside->left - 1;
		if (x <= x_end)
			return taken;
	}

	for (int row = y; row != y_end; row += y_delta)
	{
		if (row >= inside->top && row <= inside->bottom)
		{
			sprite.row = video.sprites[video.shown_sprites ^ 1][row];
			sprite.outside_row = row >= outside->top && row <= outside->bottom;
			draw_sprite_row(&sprite, from_ram, bank, address, x, x_end, x_delta, x_zoom, end_pen);
		}
		/* the zoom steps through the source rows */
		y_accumulator += y_zoom;
		address += (uint32_t)(source_width * (y_accumulator >> 16));
		y_accumulator &= 0xffff;
	}
	return taken;
}

/* The list in sprite RAM, 16 bytes an entry, its command in the top two bits of the first word: draw a
   sprite, set the clips (bit 12 the one to draw in, bit 13 the one to leave alone), jump (and with
   bit 13, set the offset sprites can add), or end. */
static void draw_sprite_list(void)
{
	const bool wide = video.wide && !bit(s32_board.sprite_control_latched[6], 0);
	const int side = wide ? S32_VIDEO_WIDE_SIDE : 0;
	const area screen = { 0, wide || bit(s32_board.sprite_control_latched[6], 0) ? 415 : 319, 0, HEIGHT - 1 };
	area inside = screen;
	area outside = { 0, -1, 0, -1 };
	int x_offset = 0;

	video.sprite_side = side;
	int y_offset = 0;
	int number = 0;

	for (int entries = 0; entries < 0x20000 / 16; entries++)
	{
		const uint32_t entry = 8 * (uint32_t)(number & 0x1fff);
		const uint16_t command = sprite_word(entry);

		switch (command >> 14)
		{
			case 0:
				number += 1 + draw_sprite(entry, x_offset, y_offset, &inside, &outside);
				break;
			case 1:
				if (bit(command, 12))
				{
					inside.left = sign_extend(sprite_word(entry + 2), 12);
					inside.right = sign_extend(sprite_word(entry + 3), 12);
					inside.top = sign_extend(command, 12);
					inside.bottom = sign_extend(sprite_word(entry + 1), 12);
					if (wide && inside.left <= 0 && inside.right >= 319)
						inside.right = screen.right;    /* (the whole screen: the wide one) */
					else
					{
						inside.left += side;
						inside.right += side;
					}
					clip_area(&inside, &screen);
				}
				if (bit(command, 13))
				{
					outside.left = sign_extend(sprite_word(entry + 6), 12) + side;
					outside.right = sign_extend(sprite_word(entry + 7), 12) + side;
					outside.top = sign_extend(sprite_word(entry + 4), 12);
					outside.bottom = sign_extend(sprite_word(entry + 5), 12);
				}
				number++;
				break;
			case 2:
				if (bit(command, 13))
				{
					y_offset = sign_extend(sprite_word(entry + 1), 12);
					x_offset = sign_extend(sprite_word(entry + 2), 12);
				}
				number = command & 0x1fff;
				break;
			default:
				return;
		}
	}
}

void s32_video_sprites(bool erase, bool draw)
{
	if (erase)
	{
		uint16_t *shown = &video.sprites[video.shown_sprites][0][0];

		for (size_t index = 0; index < (size_t)HEIGHT * WIDTH; index++)
			shown[index] = 0xffff;
	}
	if (draw)
	{
		video.shown_sprites ^= 1;
		draw_sprite_list();
	}
}

/* ---- the mixer ---- */

/* Which color offset a layer takes (0x3E bit 15 and the layer's bit there pick how its own flag counts) */
static uint8_t color_offsets(bool layer_bit, bool layer_flag)
{
	switch ((bit(mixer_register(0x3e), 15) ? 2 : 0) | (layer_bit ? 1 : 0))
	{
		case 1: return 2;
		case 2: return layer_flag ? 0 : 2;
		default: return layer_flag ? 0 : 1;
	}
}

/* The sprite groups a layer blends with: the group equal to a value, at most it, at least it, or all */
static uint16_t sprite_blend(int encoding)
{
	const int value = encoding & 0x0f;

	switch ((encoding >> 4) & 3)
	{
		case 0: return (uint16_t)(1 << value);
		case 1: return (uint16_t)((1 << value) | ((1 << value) - 1));
		case 2: return (uint16_t)(~((1 << value) - 1) & 0xffff);
		default: return 0xffff;
	}
}

typedef struct sprite_grouping
{
	uint8_t shift;      /* where the group is in a sprite pixel */
	uint8_t mask;
	uint8_t base;       /* the first group's priority register */
} sprite_grouping;

/* The sprite groupings register 0x4C picks */
static const sprite_grouping groupings[16] = {
	{ 14, 0x00, 0x01 }, { 14, 0x01, 0x02 }, { 13, 0x03, 0x04 }, { 12, 0x07, 0x08 },
	{ 14, 0x01, 0x00 }, { 13, 0x03, 0x00 }, { 12, 0x07, 0x00 }, { 11, 0x0f, 0x00 },
	{ 14, 0x01, 0x00 }, { 13, 0x03, 0x00 }, { 12, 0x07, 0x00 }, { 11, 0x0f, 0x00 },
	{ 13, 0x01, 0x00 }, { 12, 0x03, 0x00 }, { 11, 0x07, 0x00 }, { 10, 0x0f, 0x00 }
};

/* The first layer down the list that is not transparent there; the backdrop always is. A sprite pixel
   all ones lets the layers below show, and the shadow pen darkens them. */
static const mix_layer *top_layer(const mix_layer *entry, const uint16_t *const *rows, int x, int sprite_pixel,
	int shadow_mask, int sprite_bits, int shadow_pen, int *pixel, int *shadow)
{
	for (;; entry++)
	{
		if (entry->index != LAYER_SPRITES)
		{
			*pixel = rows[entry->index][x] & 0x1fff;
			if (*pixel != 0 || entry->index == LAYER_BACKDROP)
				return entry;
			continue;
		}
		*pixel = sprite_pixel;
		*shadow = ~sprite_pixel & shadow_mask;
		if ((sprite_pixel & 0x7fff) != 0x7fff)
		{
			*pixel = sprite_pixel & sprite_bits;
			if ((*pixel & 0x7ffe) != shadow_pen)
				return entry;
			*shadow = 1;
		}
	}
}

static uint16_t palette_color(const mix_layer *entry, int pixel)
{
	return s32_board.palette[(entry->palette_base + ((pixel >> entry->shift) & 0xfff0) + (pixel & 0x0f)) & 0x3fff];
}

uint16_t s32_video_tile_pen_color(int tilemap, uint16_t color, int pen)
{
	const uint16_t control = mixer_register(0x20 + 2 * (uint32_t)(LAYER_NBG0 + tilemap));
	mix_layer layer;

	layer.palette_base = (uint16_t)((control & 0x00f0) << 6);
	layer.shift = (control >> 8) & 3;
	return palette_color(&layer, ((color & 0x1ff) << 4 | (pen & 0x0f)) & 0x1fff);
}

uint16_t s32_video_sprite_pixel_color(uint16_t pixel)
{
	const uint16_t sprite_control = mixer_register(0x4c);
	const sprite_grouping grouping = groupings[sprite_control & 0x0f];
	const int group = (pixel >> grouping.shift) & grouping.mask;
	const uint16_t control = mixer_register(2 * (uint32_t)(grouping.base | group));
	mix_layer sprites;

	sprites.palette_base = (uint16_t)((((sprite_control & 3) != 3) ? control : sprite_control) & 0x00f0) << 6;
	sprites.shift = (control >> 8) & 3;
	return palette_color(&sprites, pixel & (((1 << grouping.shift) - 1) & 0x3fff));
}

/* What the mixer mixes with, the same on every row of a frame: the layers' order under each sprite group */
typedef struct mix_frame
{
	int blend_factor;
	sprite_grouping grouping;
	int shadow_mask;
	int sprite_bits;
	int shadow_pen;
	uint8_t sprite_flips;
	int offsets[3][3];
	mix_layer orders[16][8];
} mix_frame;

static void mix_rows(void *context, int first, int last)
{
	const mix_frame *frame = (const mix_frame *)context;
	const int blend_factor = frame->blend_factor;
	const sprite_grouping grouping = frame->grouping;

	for (int y = first; y <= last; y++)
	{
		const uint16_t *rows[LAYERS];
		/* (the sprite buffer is scanned flipped as the sprite controls say) */
		const int sprite_y = bit(frame->sprite_flips, 1) ? video.visible.bottom - (y - video.visible.top) : y;
		int sprite_x = bit(frame->sprite_flips, 0) ? video.visible.right : video.visible.left;
		const int sprite_dx = bit(frame->sprite_flips, 0) ? -1 : 1;

		for (int index = LAYER_TEXT; index <= LAYER_BITMAP; index++)
			rows[index] = video.layers[index].empty[y] ? transparent_row : video.layers[index].pixels[y];
		rows[LAYER_SPRITES] = video.sprites[video.shown_sprites][sprite_y];
		rows[LAYER_BACKDROP] = video.layers[LAYER_BACKDROP].pixels[y];

		for (int x = video.visible.left; x <= video.visible.right; x++, sprite_x += sprite_dx)
		{
			const int sprite_pixel = rows[LAYER_SPRITES][sprite_x];
			const int group = (sprite_pixel >> grouping.shift) & grouping.mask;
			int shadow = 0;
			int pixel;
			const mix_layer *first_layer = top_layer(frame->orders[group], rows, x, sprite_pixel, frame->shadow_mask,
				frame->sprite_bits, frame->shadow_pen, &pixel, &shadow);
			uint16_t color = palette_color(first_layer, pixel);
			int red = (color & 0x1f) + frame->offsets[first_layer->offsets][0];
			int green = ((color >> 5) & 0x1f) + frame->offsets[first_layer->offsets][1];
			int blue = ((color >> 10) & 0x1f) + frame->offsets[first_layer->offsets][2];

			if (first_layer->blend != 0)
			{
				int below;
				const mix_layer *second = top_layer(first_layer + 1, rows, x, sprite_pixel, frame->shadow_mask,
					frame->sprite_bits, frame->shadow_pen, &below, &shadow);

				if (bit(first_layer->blend, second->index)
					&& (second->index != LAYER_SPRITES || bit(first_layer->sprite_blend, group)))
				{
					color = palette_color(second, below);
					red = shift_down(red * (7 - blend_factor) + ((color & 0x1f) + frame->offsets[second->offsets][0]) * (blend_factor + 1), 3);
					green = shift_down(green * (7 - blend_factor) + (((color >> 5) & 0x1f) + frame->offsets[second->offsets][1]) * (blend_factor + 1), 3);
					blue = shift_down(blue * (7 - blend_factor) + (((color >> 10) & 0x1f) + frame->offsets[second->offsets][2]) * (blend_factor + 1), 3);
				}
			}
			if (shadow)
			{
				red = shift_down(red, 1);
				green = shift_down(green, 1);
				blue = shift_down(blue, 1);
			}
			red = red > 31 ? 31 : (red > 0 ? red : 0);
			green = green > 31 ? 31 : (green > 0 ? green : 0);
			blue = blue > 31 ? 31 : (blue > 0 ? blue : 0);
			video.frame[y][x] = (uint32_t)red << 19 | (uint32_t)green << 11 | (uint32_t)blue << 3;
		}
	}
}

/* The frame from the layers: registers 0x00-0x1E give each sprite group its priority and palette, 0x20-0x2C
   each layer, 0x30-0x3A what each blends with, 0x40-0x4A two color offsets, 0x4C the sprite grouping and
   shadows, 0x4E blending */
static void mix(int drawn)
{
	const uint16_t blend_control = mixer_register(0x4e);
	const bool blending = bit(blend_control, 11);
	const uint16_t sprite_control = mixer_register(0x4c);
	const sprite_grouping grouping = groupings[sprite_control & 0x0f];
	static mix_frame frame;
	int (*offsets)[3] = frame.offsets;
	mix_layer (*orders)[8] = frame.orders;
	mix_layer sorted[8];
	int count = 0;

	frame.blend_factor = (blend_control >> 8) & 7;
	frame.grouping = grouping;
	frame.shadow_mask = bit(sprite_control, 2) ? 0x8000 : 0;
	frame.sprite_bits = ((1 << grouping.shift) - 1) & 0x3fff;
	frame.shadow_pen = 0x7ffe & frame.sprite_bits;
	frame.sprite_flips = s32_board.sprite_control_latched[2];
	for (int channel = 0; channel < 3; channel++)
	{
		offsets[0][channel] = sign_extend(mixer_register(0x40 + 2 * (uint32_t)channel), 6);
		offsets[1][channel] = sign_extend(mixer_register(0x46 + 2 * (uint32_t)channel), 6);
		offsets[2][channel] = 0;
	}

	/* the layers that are on and have a priority, then the backdrop, from the top down */
	for (int index = LAYER_TEXT; index <= LAYER_BITMAP; index++)
	{
		const uint16_t control = mixer_register(0x20 + 2 * (uint32_t)index);
		const uint16_t blend = mixer_register(0x30 + 2 * (uint32_t)index);

		if (!bit((uint32_t)drawn, index) || (control & 0x0f) == 0)
			continue;
		sorted[count].index = (uint8_t)index;
		sorted[count].priority = (uint8_t)((control & 0x0f) << 3 | (6 - index));
		sorted[count].palette_base = (uint16_t)((control & 0x00f0) << 6);
		sorted[count].shift = (control >> 8) & 3;
		sorted[count].blend = blending ? (uint8_t)((blend >> 6) & 0xff) : 0;
		sorted[count].sprite_blend = sprite_blend(blend & 0x3f);
		sorted[count].offsets = color_offsets(bit(mixer_register(0x3e), index), bit(blend, 14));
		count++;
	}
	sorted[count].index = LAYER_BACKDROP;
	sorted[count].priority = 1 << 3;
	sorted[count].palette_base = (uint16_t)((mixer_register(0x2c) & 0x00f0) << 6);
	sorted[count].shift = (mixer_register(0x2c) >> 8) & 3;
	sorted[count].blend = 0;
	sorted[count].sprite_blend = 0;
	sorted[count].offsets = color_offsets(bit(mixer_register(0x3e), 8), bit(mixer_register(0x3e), 14));
	count++;
	for (int index = 0; index < count; index++)
		for (int other = index + 1; other < count; other++)
			if (sorted[other].priority > sorted[index].priority)
			{
				const mix_layer swap = sorted[other];

				sorted[other] = sorted[index];
				sorted[index] = swap;
			}

	/* each sprite group goes in above the first layer of a lower priority */
	for (int group = 0; group <= grouping.mask; group++)
	{
		const uint16_t control = mixer_register(2 * (uint32_t)(grouping.base | group));
		const int priority = (control & 0x0f) << 3 | 7;
		int place = count;
		int copied = 0;
		mix_layer *sprites;

		for (int index = 0; index < count; index++)
		{
			if (priority > sorted[index].priority && place == count)
				place = copied++;
			orders[group][copied++] = sorted[index];
		}
		sprites = &orders[group][place];
		sprites->index = LAYER_SPRITES;
		sprites->priority = (uint8_t)priority;
		sprites->palette_base = (uint16_t)((((sprite_control & 3) != 3) ? control : sprite_control) & 0x00f0) << 6;
		sprites->shift = (control >> 8) & 3;
		sprites->blend = 0;
		sprites->sprite_blend = 0;
		sprites->offsets = color_offsets(bit(mixer_register(0x3e), 6), bit(sprite_control, 15));
	}
	s32_workers_rows(mix_rows, &frame, video.visible.top, video.visible.bottom);
}

/* ---- the frame ---- */

void s32_video_draw_frame(void)
{
	const bool board_wide = bit(video_register(0x00), 15);
	const int width = board_wide || video.wide ? 416 : 320;

	video.side = video.wide && !board_wide ? S32_VIDEO_WIDE_SIDE : 0;
	video.visible.left = 0;
	video.visible.right = width - 1;
	video.visible.top = 0;
	video.visible.bottom = HEIGHT - 1;

	/* the I/O chip's CNT1 output turns the display on */
	if (!bit(s32_board.io_cnt, 1))
	{
		for (int y = 0; y < HEIGHT; y++)
			memset(video.frame[y], 0, (size_t)width * sizeof(video.frame[y][0]));
		return;
	}
	mix(draw_layers());
}

#define BLANK_TILES_MOST 0x8000    /* 4 banks of 8192 tiles */

static uint8_t blank_tiles[BLANK_TILES_MOST];    /* by tile: 0 not looked at yet, 1 blank, 2 drawn */

bool s32_video_cell_blank(uint16_t cell)
{
	const uint32_t bank = (s32_board.tilebank_external & 1) << 1 | ((video_register(0x00) >> 10) & 1);
	const uint32_t tile = (bank << 13 | (cell & 0x1fff)) % video.tile_count % BLANK_TILES_MOST;

	if (blank_tiles[tile] == 0)
	{
		const uint8_t *bytes = &video.tiles[(size_t)tile * 128];
		int index = 0;

		while (index < 128 && bytes[index] == 0)
			index++;
		blank_tiles[tile] = index == 128 ? 1 : 2;
	}
	return blank_tiles[tile] == 1;
}

int s32_video_tile_pen(uint32_t tile, int x, int y)
{
	return tile_pen(tile, x & 15, y & 15);
}

void s32_video_set_cells(const uint16_t *words, const uint8_t *marks)
{
	own_cell_words = words;
	own_cell_marks = words ? marks : NULL;
}

void s32_video_init(const uint8_t *tiles, size_t tiles_size, const uint8_t *sprites, size_t sprites_size)
{
	memset(&video, 0, sizeof(video));
	memset(blank_tiles, 0, sizeof(blank_tiles));
	video.tiles = tiles;
	video.tile_count = (uint32_t)(tiles_size / 128);
	video.sprite_rom = sprites;
	video.sprite_rom_words = (uint32_t)(sprites_size / 4);
	video.sprite_banks = video.sprite_rom_words >> 20;
	video.visible.right = 319;
	video.visible.bottom = HEIGHT - 1;
	s32_board.sprites_due = s32_video_sprites;
	s32_board.frame_shown = s32_video_draw_frame;
}

void s32_video_set_wide(bool wide)
{
	video.wide = wide;
}

int s32_video_side(void)
{
	return video.side;
}

const uint32_t *s32_video_frame(int *width)
{
	if (width)
		*width = video.visible.right + 1;
	return &video.frame[0][0];
}

uint64_t s32_video_frame_hash(void)
{
	uint64_t hash = 0xcbf29ce484222325ull;

	for (int y = 0; y < HEIGHT; y++)
		for (int x = 0; x <= video.visible.right; x++)
			for (int shift = 0; shift < 24; shift += 8)
				hash = (hash ^ ((video.frame[y][x] >> shift) & 0xff)) * 0x100000001b3ull;
	return hash;
}

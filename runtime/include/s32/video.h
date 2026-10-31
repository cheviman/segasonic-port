/**
 *    Name:        video.h
 *    Description: Sega System 32 video: draws each frame of the board from its memories, as the hardware does
 *    Version:     0.1.0
 *    Created:     13-10-2026
 *    Modified:    31-10-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    The video chips of the board as a library: the four tilemaps (NBG0-3), the text layer, the bitmap
 *    layer, the backdrop, the two sprite frame buffers and the mixer. s32_video_init() hooks it to the
 *    board: the sprite list is drawn into the hidden buffer when the board sends it, and the frame is
 *    composed when VBLANK starts, from video RAM, sprite RAM, palette and mixer as they are then.
 *
 *    A port shows s32_video_frame() and nothing else of the board's video: 320 or 416 x 224 pixels,
 *    0x00RRGGBB, five bits a channel in the top bits of each byte.
 */
#ifndef S32_VIDEO_H
#define S32_VIDEO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define S32_VIDEO_MAX_WIDTH  416
#define S32_VIDEO_HEIGHT     224

/* The tile ROM (16x16 tiles, 128 bytes each) and the sprite ROMs (32-bit words, big endian), as
   game/data.h holds them; the video keeps pointers to them. Hooks the video to s32_board. */
void s32_video_init(const uint8_t *tiles, size_t tiles_size, const uint8_t *sprites, size_t sprites_size);

/* A wider picture than the board's, for a port that wants one (the arcade's is never wide): a 320-pixel
   frame drawn 416 wide, its own picture in the middle (S32_VIDEO_WIDE_SIDE more pixels of the tilemaps,
   the bitmap and the sprites at each side; the text layer only where it was, so the HUD stays); a frame
   the board draws 416 wide stays as it is */
#define S32_VIDEO_WIDE_SIDE  48
void s32_video_set_wide(bool wide);

/* How far right the last frame drew the board's picture: S32_VIDEO_WIDE_SIDE when it was wide, else 0 */
int s32_video_side(void);

/* A tilemap cell (as the pages hold it: tile number, color, flips) draws nothing: its tile, in the bank
   the tilemaps use now, has no pen but 0 */
bool s32_video_cell_blank(uint16_t cell);

/* The pen (0-15) of pixel 'x', 'y' (0-15) of a tile (bank << 13 | number), as the tilemaps draw it */
int s32_video_tile_pen(uint32_t tile, int x, int y);

/* A port's own tilemap cells, drawn in place of video RAM's: by video RAM word (page << 9 | cell), the
   cells whose mark is not 0 take their word from 'words'; NULL, none (the arcade's) */
#define S32_VIDEO_CELLS 0x10000
void s32_video_set_cells(const uint16_t *words, const uint8_t *marks);

/* What the board calls (s32_video_init sets them): the sprite commands, and the start of VBLANK */
void s32_video_sprites(bool erase, bool draw);
void s32_video_draw_frame(void);

/* The last frame: S32_VIDEO_MAX_WIDTH pixels a row, S32_VIDEO_HEIGHT rows; its width is 320 or 416 */
const uint32_t *s32_video_frame(int *width);

/* FNV-1a hash of the last frame's visible pixels (red, green, blue bytes), as tools/reference/s32ref
   writes it */
uint64_t s32_video_frame_hash(void);

/* For tools that look at what the sprites are drawn with (the asset extractor's census): called for every
   sprite the list draws, before it is drawn, with its graphics' place in the sprite ROM (32-bit words,
   bank << 20 | offset), its size in words and rows, whether it is 8 bits a pen, whether it is opaque (it
   draws the end code, all ones at a word's edges, and goes on past it), and the pixel each pen puts in the
   sprite buffer (16 or 256 of them). NULL, the default, calls nothing. */
typedef void (*s32_sprite_observer)(uint32_t address, int words, int rows, bool bits8, bool opaque,
	const uint16_t *pixels);
extern s32_sprite_observer s32_video_sprite_observer;

/* The color (RGB555) a pixel of the sprite buffer shows with the mixer's registers as they are now: its
   group's palette, without blending, shadow or color offsets */
uint16_t s32_video_sprite_pixel_color(uint16_t pixel);

/* Likewise for the tilemaps: called, when a frame is drawn, for every cell of the pages each tilemap
   (NBG0-3: 0-3) that is on shows, with its tile (bank << 13 | number) and color (the cell's 9 bits). NULL,
   the default, calls nothing. */
typedef void (*s32_tile_observer)(int tilemap, uint32_t tile, uint16_t color);
extern s32_tile_observer s32_video_tile_observer;

/* The color (RGB555) pen 'pen' of a tile drawn with color 'color' shows on tilemap 'tilemap' (0-3), with the
   mixer's registers as they are now, without blending or color offsets */
uint16_t s32_video_tile_pen_color(int tilemap, uint16_t color, int pen);

#ifdef __cplusplus
}
#endif

#endif

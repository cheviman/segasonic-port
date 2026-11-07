/**
 *    Name:        data.h
 *    Description: the game's data in the board's memory, loaded from its assets pack
 *    Version:     0.2.0
 *    Created:     13-10-2026
 *    Modified:    07-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    No executable carries the ROM or anything from it. game_data_load fills the regions from the assets
 *    pack (tools/make_pack.py writes it from the assets extract_assets.py takes out of the player's ROM
 *    set): every asset where the board's memory has it, then the board's mirrors of its chips. The original
 *    code is in no region: the C replaces it. The reference (tools/reference) fills them from the ROM set,
 *    which it runs, when it starts.
 */
#ifndef GAME_DATA_H
#define GAME_DATA_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GAME_PROGRAM_SIZE   0x200000     /* the V60's program space, as it sees it at 000000: the data */
#define GAME_TILES_SIZE     0x200000     /* 16x16 tiles at 4 bits a pixel */
#define GAME_SPRITES_SIZE   0x1000000    /* sprite pixels, every 32-bit word big endian */
#define GAME_SOUND_SIZE     0x400000     /* the Z80's program space (its data), then three 1 MB banks of
                                            PCM samples */

/* The regions: 24 MB taken from the heap the first time they are filled (game_data_regions), not in the
   executable's image, where a section that large and empty on disk looks to some antivirus heuristics like
   a packed program's (VirusTotal's MaxSecure, "susgen", on v1.3.1's executables). NULL until then */
extern uint8_t *game_program;
extern uint8_t *game_tiles;
extern uint8_t *game_sprites;
extern uint8_t *game_sound;

/* The regions taken, once (zeroed); false when there is not the memory for them */
int game_data_regions(void);

/* A region by its number in the pack (0 program, 1 tiles, 2 sprites, 3 sound), and its size */
uint8_t *game_data_region(unsigned region);
uint32_t game_data_region_size(unsigned region);

typedef enum
{
	GAME_DATA_LOADED,
	GAME_DATA_MISSING,    /* no pack at the path */
	GAME_DATA_DAMAGED     /* a pack the game cannot read: another version, cut short, or out of bounds */
} game_data_result;

game_data_result game_data_load(const char *path);

/* The pack from a file already open at its start (Android's: in the APK, with no path of its own); the
   caller closes it */
game_data_result game_data_read(FILE *input);

/* The board's mirrors of its chips: what the regions repeat where the board decodes the same chip again */
void game_data_mirror(void);

#ifdef __cplusplus
}
#endif

#endif

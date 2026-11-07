/**
 *    Name:        game_data.c
 *    Description: loads the game's assets pack into the board's memory (game/data.h)
 *    Version:     0.1.0
 *    Created:     20-10-2026
 *    Modified:    07-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    The pack's format is tools/make_pack.py's: "SSAP", u16 version, u16 0, u32 entries; each entry a u8
 *    kind (0 bytes, 1 fill), u8 region, u16 name length, u32 offset, u32 size, the name, then the bytes or
 *    the fill byte; all little endian.
 */
#include "game/data.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PACK_VERSION   1u
#define KIND_BYTES     0u
#define KIND_FILL      1u
#define REGIONS        4u

#define REGION_PROGRAM 0u
#define REGION_SOUND   3u

uint8_t *game_program;
uint8_t *game_tiles;
uint8_t *game_sprites;
uint8_t *game_sound;

static uint8_t *regions[REGIONS];
static const uint32_t region_sizes[REGIONS] = { GAME_PROGRAM_SIZE, GAME_TILES_SIZE, GAME_SPRITES_SIZE, GAME_SOUND_SIZE };

int game_data_regions(void)
{
	size_t region;

	if (regions[0])
		return 1;
	for (region = 0; region < REGIONS; region++)
		if ((regions[region] = (uint8_t *)calloc(region_sizes[region], 1)) == NULL)
		{
			while (region > 0)
			{
				region--;
				free(regions[region]);
				regions[region] = NULL;
			}
			return 0;
		}
	game_program = regions[0];
	game_tiles = regions[1];
	game_sprites = regions[2];
	game_sound = regions[3];
	return 1;
}

uint8_t *game_data_region(unsigned region)
{
	return region < REGIONS ? regions[region] : NULL;
}

uint32_t game_data_region_size(unsigned region)
{
	return region < REGIONS ? region_sizes[region] : 0u;
}

/* The chips the board decodes more than once (MAME's ROM_START( sonic )): the region, where the chip's
   bytes are, how many, and where they show again */
typedef struct
{
	unsigned region;
	uint32_t from, size;
	uint32_t to[3];
} chip_mirror;

static const chip_mirror mirrors[] = {
	{ REGION_PROGRAM, 0x000000u, 0x020000u, { 0x020000u, 0x040000u, 0x060000u } },    /* epr-15787c, x4 */
	{ REGION_PROGRAM, 0x080000u, 0x020000u, { 0x0A0000u, 0x0C0000u, 0x0E0000u } },    /* epr-15786c, x4 */
	{ REGION_PROGRAM, 0x100000u, 0x080000u, { 0x180000u } },                          /* the data chips, x2 */
	{ REGION_SOUND, 0x000000u, 0x040000u, { 0x040000u, 0x080000u, 0x0C0000u } },      /* epr-15785, x4 */
};

static uint32_t little16(const uint8_t *bytes)
{
	return (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8;
}

static uint32_t little32(const uint8_t *bytes)
{
	return little16(bytes) | little16(bytes + 2) << 16;
}

void game_data_mirror(void)
{
	size_t mirror, copy;

	if (!regions[0])
		return;    /* (no regions taken yet: nothing to mirror) */
	for (mirror = 0; mirror < sizeof(mirrors) / sizeof(mirrors[0]); mirror++)
		for (copy = 0; copy < 3 && mirrors[mirror].to[copy] != 0u; copy++)
			memcpy(regions[mirrors[mirror].region] + mirrors[mirror].to[copy],
				regions[mirrors[mirror].region] + mirrors[mirror].from, mirrors[mirror].size);
}

/* The entries of the pack at 'input', each where it goes; false when the pack is cut short or one is out
   of its region */
static int read_entries(FILE *input, uint32_t entries)
{
	uint8_t head[12];
	uint32_t entry;

	for (entry = 0; entry < entries; entry++)
	{
		uint32_t kind, region, offset, size;
		int fill;

		if (fread(head, 1, sizeof(head), input) != sizeof(head))
			return 0;
		kind = head[0];
		region = head[1];
		offset = little32(head + 4);
		size = little32(head + 8);
		if (region >= REGIONS || offset > region_sizes[region] || size > region_sizes[region] - offset
			|| fseek(input, (long)little16(head + 2), SEEK_CUR) != 0)
			return 0;
		if (kind == KIND_FILL)
		{
			if ((fill = fgetc(input)) == EOF)
				return 0;
			memset(regions[region] + offset, fill, size);
		}
		else if (kind != KIND_BYTES || fread(regions[region] + offset, 1, size, input) != size)
			return 0;
	}
	return 1;
}

game_data_result game_data_read(FILE *input)
{
	uint8_t head[12];
	size_t region;

	if (!game_data_regions())
		return GAME_DATA_DAMAGED;    /* (no memory for them: as good as a pack that cannot be read) */
	for (region = 0; region < REGIONS; region++)
		memset(regions[region], 0, region_sizes[region]);
	if (fread(head, 1, sizeof(head), input) != sizeof(head) || memcmp(head, "SSAP", 4) != 0
		|| little16(head + 4) != PACK_VERSION || !read_entries(input, little32(head + 8)))
		return GAME_DATA_DAMAGED;
	game_data_mirror();
	return GAME_DATA_LOADED;
}

game_data_result game_data_load(const char *path)
{
	FILE *input = fopen(path, "rb");
	game_data_result result;

	if (!input)
		return GAME_DATA_MISSING;
	result = game_data_read(input);
	fclose(input);
	return result;
}

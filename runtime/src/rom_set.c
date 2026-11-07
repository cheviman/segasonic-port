/**
 *    Name:        rom_set.c
 *    Description: the player's ROM set into the board's memory (game/data.h), from a .zip or a folder
 *    Version:     0.2.0
 *    Created:     20-10-2026
 *    Modified:    07-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    Each of the set's 18 files is known by its CRC32: in a .zip by its own, whatever its name (stored or
 *    deflated: inflate.c); in a folder by MAME's name or the older epr-c-NN one. The regions are laid out
 *    as tools/sonic_rom.py lays them, after ROM_START( sonic ) in MAME's src/mame/sega/segas32.cpp
 *    (BSD-3-Clause). The reference runs the original code, so it reads the set when it starts, as MAME
 *    does; the game reads it once, to write its assets pack (extract.c).
 */
#include "game/extract.h"

#include "game/data.h"
#include "inflate.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ZIP_END             0x06054B50u    /* the end of the central directory */
#define ZIP_ENTRY           0x02014B50u    /* a central directory entry */
#define ZIP_LOCAL           0x04034B50u    /* a local header */
#define ZIP_STORED          0
#define ZIP_DEFLATED        8
#define ZIP_COMMENT_MOST    0xFFFFu

typedef struct
{
	const char *name;
	const char *older;    /* the name older dumps give it, or NULL */
	uint32_t crc, size;
} rom_file;

enum
{
	IC17, IC8, IC18, IC9, IC36, IC35, IC34, IC33, IC14, IC5, IC32, IC30, IC28, IC26, IC31, IC29, IC27, IC25, FILES
};

static const rom_file files[FILES] = {
	{ "epr-15787c.ic17", "epr-c-87.ic17", 0x25e3c27eu, 0x020000u },
	{ "epr-15786c.ic8", "epr-c-86.ic8", 0xefe9524cu, 0x020000u },
	{ "epr-15781c.ic18", "epr-c-81.ic18", 0x65b06c25u, 0x040000u },
	{ "epr-15780c.ic9", "epr-c-80.ic9", 0x2db66fd2u, 0x040000u },
	{ "epr-15785.ic36", NULL, 0x0fe7422eu, 0x040000u },
	{ "mpr-15784.ic35", NULL, 0x42f06714u, 0x100000u },
	{ "mpr-15783.ic34", NULL, 0xe4220eeau, 0x100000u },
	{ "mpr-15782.ic33", NULL, 0xcf56b5a0u, 0x100000u },
	{ "mpr-15789.ic14", NULL, 0x4378f12bu, 0x100000u },
	{ "mpr-15788.ic5", NULL, 0xa6ed5d7au, 0x100000u },
	{ "mpr-15790.ic32", NULL, 0xc69d51b1u, 0x200000u },
	{ "mpr-15792.ic30", NULL, 0x1006bb67u, 0x200000u },
	{ "mpr-15794.ic28", NULL, 0x8672b480u, 0x200000u },
	{ "mpr-15796.ic26", NULL, 0x95b8011cu, 0x200000u },
	{ "mpr-15791.ic31", NULL, 0x42217066u, 0x100000u },
	{ "mpr-15793.ic29", NULL, 0x75bafe55u, 0x100000u },
	{ "mpr-15795.ic27", NULL, 0x7f3dad30u, 0x100000u },
	{ "mpr-15797.ic25", NULL, 0x013c6cabu, 0x100000u },
};

static uint32_t crc32_of(const uint8_t *data, uint32_t size)
{
	static uint32_t table[256];
	uint32_t crc = 0xFFFFFFFFu, index;
	int bit;

	if (table[1] == 0u)
		for (index = 0; index < 256u; index++)
		{
			table[index] = index;
			for (bit = 0; bit < 8; bit++)
				table[index] = (table[index] >> 1) ^ (0xEDB88320u & (0u - (table[index] & 1u)));
		}
	for (index = 0; index < size; index++)
		crc = (crc >> 8) ^ table[(crc ^ data[index]) & 0xFFu];
	return ~crc;
}

static uint32_t little16(const uint8_t *bytes)
{
	return (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8;
}

static uint32_t little32(const uint8_t *bytes)
{
	return little16(bytes) | little16(bytes + 2) << 16;
}

/* A whole file into memory (the caller frees it), or NULL */
static uint8_t *read_whole(const char *path, size_t *size)
{
	FILE *input = fopen(path, "rb");
	uint8_t *data = NULL;
	long length;

	if (!input)
		return NULL;
	if (fseek(input, 0, SEEK_END) == 0 && (length = ftell(input)) > 0 && fseek(input, 0, SEEK_SET) == 0
		&& (data = malloc((size_t)length)) != NULL && fread(data, 1, (size_t)length, input) != (size_t)length)
	{
		free(data);
		data = NULL;
	}
	fclose(input);
	if (data)
		*size = (size_t)length;
	return data;
}

/* The set's files in the .zip 'zip' ('size' bytes) into 'chips', by their CRC32: 0 when it is no .zip */
static int read_zip(const uint8_t *zip, size_t size, uint8_t **chips)
{
	size_t end, entry, local;
	uint32_t entries, index;
	int file;

	if (size < 22)
		return 0;
	for (end = size - 22;; end--)
	{
		if (little32(zip + end) == ZIP_END)
			break;
		if (end == 0 || size - end > 22 + ZIP_COMMENT_MOST)
			return 0;
	}
	entries = little16(zip + end + 10);
	entry = little32(zip + end + 16);
	for (index = 0; index < entries; index++)
	{
		uint32_t method, crc, packed, unpacked;

		if (entry > size || size - entry < 46 || little32(zip + entry) != ZIP_ENTRY)
			return 0;
		method = little16(zip + entry + 10);
		crc = little32(zip + entry + 16);
		packed = little32(zip + entry + 20);
		unpacked = little32(zip + entry + 24);
		local = little32(zip + entry + 42);
		entry += 46 + little16(zip + entry + 28) + little16(zip + entry + 30) + little16(zip + entry + 32);
		for (file = 0; file < FILES; file++)
			if (!chips[file] && files[file].crc == crc && files[file].size == unpacked)
				break;
		if (file == FILES || local > size || size - local < 30 || little32(zip + local) != ZIP_LOCAL)
			continue;
		local += 30 + little16(zip + local + 26) + little16(zip + local + 28);
		if (local > size || packed > size - local || (method == ZIP_STORED && packed != unpacked)
			|| !(chips[file] = malloc(unpacked)))
			continue;    /* (what the entry says does not fit the file) */
		if ((method == ZIP_STORED ? (memcpy(chips[file], zip + local, unpacked), 1)
				: method == ZIP_DEFLATED && inflate_raw(zip + local, packed, chips[file], unpacked))
			&& crc32_of(chips[file], unpacked) == crc)
			continue;
		free(chips[file]);
		chips[file] = NULL;
	}
	return 1;
}

/* The set's files in the folder 'folder' into 'chips', by either of their names and their CRC32 */
static void read_folder(const char *folder, uint8_t **chips)
{
	char path[1024];
	int file, name;

	for (file = 0; file < FILES; file++)
		for (name = 0; name < 2 && !chips[file]; name++)
		{
			const char *names[2] = { files[file].name, files[file].older };
			size_t size = 0;

			if (!names[name])
				continue;
			snprintf(path, sizeof(path), "%s/%s", folder, names[name]);
			chips[file] = read_whole(path, &size);
			if (chips[file] && (size != files[file].size || crc32_of(chips[file], files[file].size) != files[file].crc))
			{
				free(chips[file]);
				chips[file] = NULL;
			}
		}
}

/* ROM_LOAD16_BYTE: even bytes from one chip, odd bytes from the other */
static void interleave16(uint8_t *out, const uint8_t *even, const uint8_t *odd, uint32_t size)
{
	uint32_t index;

	for (index = 0; index < size; index++)
	{
		out[index * 2u] = even[index];
		out[index * 2u + 1u] = odd[index];
	}
}

/* ROM_LOAD64_WORD: four chips' 16-bit words spread over 8-byte groups, from 'base' */
static void sprite_lanes(uint32_t base, uint8_t *const *lanes, uint32_t size)
{
	uint32_t lane, word;

	for (lane = 0; lane < 4u; lane++)
		for (word = 0; word < size / 2u; word++)
		{
			game_sprites[base + word * 8u + lane * 2u] = lanes[lane][word * 2u];
			game_sprites[base + word * 8u + lane * 2u + 1u] = lanes[lane][word * 2u + 1u];
		}
}

const char *game_rom_set_load(const char *source)
{
	static char problem[96];
	uint8_t *chips[FILES] = { NULL };
	size_t size = 0;
	uint8_t *whole;
	int file, copy;

	if (!game_data_regions())
		return "not enough memory";
	whole = read_whole(source, &size);
	if (whole)
	{
		const int zip = read_zip(whole, size, chips);

		free(whole);
		if (!zip)
			return "not a .zip";
	}
	else
		read_folder(source, chips);
	problem[0] = '\0';
	for (file = 0; file < FILES && !problem[0]; file++)
		if (!chips[file])
			snprintf(problem, sizeof(problem), "no %s (or not this version of it)", files[file].name);
	if (!problem[0])
	{
		for (copy = 0; copy < 4; copy++)
		{
			memcpy(game_program + copy * 0x20000u, chips[IC17], 0x20000u);
			memcpy(game_program + 0x080000u + copy * 0x20000u, chips[IC8], 0x20000u);
			memcpy(game_sound + copy * 0x40000u, chips[IC36], 0x40000u);
		}
		interleave16(game_program + 0x100000u, chips[IC18], chips[IC9], 0x40000u);
		memcpy(game_program + 0x180000u, game_program + 0x100000u, 0x80000u);
		interleave16(game_tiles, chips[IC14], chips[IC5], 0x100000u);
		memset(game_sprites, 0xFF, GAME_SPRITES_SIZE);
		sprite_lanes(0x000000u, &chips[IC32], 0x200000u);
		sprite_lanes(0x800000u, &chips[IC31], 0x100000u);
		memcpy(game_sound + 0x100000u, chips[IC35], 0x100000u);
		memcpy(game_sound + 0x200000u, chips[IC34], 0x100000u);
		memcpy(game_sound + 0x300000u, chips[IC33], 0x100000u);
	}
	for (file = 0; file < FILES; file++)
		free(chips[file]);
	return problem[0] ? problem : NULL;
}

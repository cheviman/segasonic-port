/**
 *    Name:        extract.c
 *    Description: the first run: the player's ROM set into the game's assets pack (game/extract.h)
 *    Version:     0.1.0
 *    Created:     20-10-2026
 *    Modified:    07-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    The pack is the one tools/make_pack.py writes from the assets (its format there): pack_layout.c's
 *    pieces, in its order, each with the bytes the board's memory has there or its fill byte. None of them
 *    holds the original code.
 */
#include "game/extract.h"

#include "game/data.h"

#include <stdio.h>
#include <string.h>

#define PACK_VERSION   1u

static int put16(FILE *output, uint32_t value)
{
	const uint8_t bytes[2] = { (uint8_t)value, (uint8_t)(value >> 8) };

	return fwrite(bytes, 1, 2, output) == 2;
}

static int put32(FILE *output, uint32_t value)
{
	return put16(output, value & 0xFFFFu) && put16(output, value >> 16);
}

const char *game_data_extract(const char *source, const char *pack)
{
	const char *problem = game_rom_set_load(source);
	FILE *output;
	uint32_t index;
	int written;

	if (problem)
		return problem;
	if (!(output = fopen(pack, "wb")))
		return "the assets pack cannot be written";
	written = fwrite("SSAP", 1, 4, output) == 4 && put16(output, PACK_VERSION) && put16(output, 0u)
		&& put32(output, pack_piece_count);
	for (index = 0; index < pack_piece_count && written; index++)
	{
		const pack_piece *piece = &pack_pieces[index];
		const uint32_t name = (uint32_t)strlen(piece->name);

		written = fputc(piece->kind, output) != EOF && fputc(piece->region, output) != EOF && put16(output, name)
			&& put32(output, piece->offset) && put32(output, piece->size)
			&& fwrite(piece->name, 1, name, output) == name
			&& (piece->kind == 1 ? fputc(piece->fill, output) != EOF
				: fwrite(game_data_region(piece->region) + piece->offset, 1, piece->size, output) == piece->size);
	}
	if (fclose(output) != 0 || !written)
	{
		remove(pack);
		return "the assets pack cannot be written";
	}
	return NULL;
}

/**
 *    Name:        system/test_mode.c
 *    Description: the test mode: the menu, the memory, input and sound tests, the CRT test, the game and coin assignments, the bookkeeping screen, the backup clear, the output test, the exit
 *    Version:     0.1.0
 *    Created:     12-10-2026
 *    Modified:    23-10-2026
 *    Author:      Sebastian Toyos
 *
 *    SegaSonic The Hedgehog (arcade, Sega System 32): the game's V60 program decompiled into C by
 *    tools/decompile.py, with the names of symbols.txt.
 */
#include "game.h"

/* Enters the test mode: display off, show_eeprom_write_count when service is held, palettes, video
   registers, sprites, init_sound and sound 0x94, objects reset, text and bitmap layers cleared (8 bpp,
   filled with 9), port D outputs and colour offsets off, test_mode 0xFF on the menu; then the first
   run_test_mode. */
void start_test_mode(uint32_t *p_color, uint32_t *p_blocks_per_row)
{
	uint8_t is_8bpp = 0, mode = 0, sound_id = 0;
	uint16_t scroll_x = 0;
	uint32_t clear_value = 0, count = 0, ignored = 0, ignored_2 = 0, ignored_3 = 0, ignored_4 = 0, ignored_5 = 0, ignored_6 = 0, ignored_7 = 0, scroll_y = 0;
	uint8_t cnt_bits = 0, port_d_bits = 0, seat_port_bits = 0;

	cnt_bits = mem_read8(io_cnt);
	mem_write8(io_cnt, cnt_bits & ~2u);
	show_eeprom_write_count = 0u;
	if ((0x1001u & input_ports_active) != 0u)
		show_eeprom_write_count = 1u;
	display_off_frames = 0x14u;
	load_default_palettes();
	init_video_registers();
	init_sprites(&ignored_2, &ignored_3);
	init_sound();
	port_d_bits = mem_read8(io_port_d);
	mem_write8(io_port_d, port_d_bits & ~8u);
	sound_id = 0x94u;
	queue_sound(sound_id, &ignored_4, &ignored_5);
	count = wait_for_frames();
	reset_objects(&ignored_2, &ignored_6);
	clear_sprite_buckets();
	mode = 0u;
	set_screen_mode(mode);
	clear_text_layer(&clear_value, &ignored_4, &ignored);
	clear_bitmap_layer((uint8_t)clear_value, &scroll_y, p_color, &ignored_7, &count, p_blocks_per_row, &ignored_3);
	mem_write16(video_layer_disable, 0xFu);
	scroll_x = 0u;
	scroll_y &= ~0xFFFFu;
	is_8bpp = 1u;
	set_bitmap_scroll(scroll_x, (uint16_t)scroll_y, is_8bpp);
	test_mode = 0xFFu;
	test_screen = 0u;
	test_cursor = 0u;
	test_continue_mode = 0u;
	test_screen_ready = 0u;
	seat_port_bits = mem_read8(io_port_d);
	mem_write8(io_port_d, seat_port_bits & ~0xCu);
	color_offset_red = 0u;
	color_offset_blue = 0u;
	color_offset_green = 0u;
	color_offset_2_red = 0u;
	color_offset_2_blue = 0u;
	color_offset_2_green = 0u;
	backup_clear_timer = 0u;
	mem_write16(mixer_bitmap, 0x21u);
	mem_write32(video_control, mem_read32(video_control) | 0x800u);
	fill_value = (fill_value & ~0xFFFFu) | 0x909u;
	set_halfwords(VIDEO_RAM, 0xE000u, fill_value);
	run_test_mode(scroll_y, (uint16_t)count, p_color, p_blocks_per_row);
}

/* One frame of the test mode: runs the screen test_screen names: 0 test_menu, 1
   toggle_test_continue_mode, 2 test_memory, 3 test_inputs, 4 test_sound, 5 test_crt, 6
   test_game_assignments, 7 test_coin_assignments, 8 test_bookkeeping, 9 test_backup_clear, 0xA
   test_outputs, 0xB exit_test_mode. */
void CONSOLE_ORIGINAL(run_test_mode)(uint32_t pressed, uint16_t count, uint32_t *p_color, uint32_t *p_blocks_per_row)
{
	uint32_t color = *p_color;

	switch (test_screen)
	{
	case 0:
		test_menu();
		break;
	case 1:
		toggle_test_continue_mode();
		break;
	case 2:
		test_memory(&color, p_blocks_per_row);
		break;
	case 3:
		test_inputs(count, &color, p_blocks_per_row);
		break;
	case 4:
		test_sound();
		break;
	case 5:
		test_crt(pressed, &color, p_blocks_per_row);
		break;
	case 6:
		test_game_assignments(&color, p_blocks_per_row);
		break;
	case 7:
		color = test_coin_assignments(color);
		break;
	case 8:
		test_bookkeeping();
		break;
	case 9:
		color = test_backup_clear(color);
		break;
	case 10:
		test_outputs();
		break;
	case 11:
		exit_test_mode();
		break;
	case 12:
		test_menu();
		break;
	default:
		invalid_jump(0x9CA0Eu + test_screen * 3u);
	}
	*p_color = color;
}

/* test_screen 0, the TEST MODE menu: drawn on its first frame with the cursor on EXIT (item 0xA);
   service moves test_cursor down the items, wrapping, and skips OUTPUT TEST (item 9) without
   moving_seat_dip; test opens item n as test_screen n + 1 (test_menu_item = n). */
void test_menu(void)
{
	uint32_t cursor_entry = 0, cursor_positions = 0, ignored = 0, ignored_2 = 0, ignored_3 = 0;

	cursor_positions = test_menu_cursor_positions_ADDRESS;
	erase_test_cursor(cursor_positions);
	if (test_screen_ready == 0u)
	{
		test_screen_ready++;
		clear_text_layer(&ignored, &ignored_2, &ignored_3);
		test_cursor = 0xAu;
		draw_test_menu();
		return;
	}
	if ((inputs_pressed & 1u) != 0u || (inputs_pressed & 0x1000u) != 0u)
	{
		test_cursor++;
		if (mem_read8(moving_seat_dip) == 0u && test_cursor == 9u)
			test_cursor++;
	}
	for (;;)
	{
		cursor_entry = test_menu_cursor_positions_ADDRESS + ((uint32_t)test_cursor << 1);
		if ((int8_t)TEXT_CELL_POSITION(cursor_entry)->column >= 0)
			break;
		test_cursor = 0u;
	}
	test_cursor_column = TEXT_CELL_POSITION(cursor_entry)->column;
	test_cursor_row = TEXT_CELL_POSITION(cursor_entry)->row;
	blink_test_cursor();
	if ((inputs_pressed & 2u) != 0u || (inputs_pressed & 0x2000u) != 0u)
	{
		test_screen = test_cursor;
		test_screen++;
		test_menu_item = test_cursor;
		test_screen_ready = 0u;
	}
}

/* Prints the TEST MODE menu (test_menu_text) on a cleared text layer, its OUTPUT TEST line only with
   moving_seat_dip, and eeprom_write_count (decimal, at 31,26) when show_eeprom_write_count is set;
   test_cursor_positions = test_menu_cursor_positions. */
void draw_test_menu(void)
{
	uint16_t attribute = 0;
	uint32_t ignored = 0, ignored_2 = 0, ignored_3 = 0, print_position = 0, text = 0, write_count_bcd = 0;

	test_cursor_positions = test_menu_cursor_positions_ADDRESS;
	clear_text_layer(&ignored, &ignored_2, &ignored_3);
	if (show_eeprom_write_count != 0u)
	{
		write_count_bcd = (uint16_t)eeprom_write_count;
		convert_to_bcd(&write_count_bcd, &ignored_2);
		write_count_bcd &= 0xFFFFFu;
		print_position = TEXT_CELL(31, 26);
		attribute = 0x1C00u;
		draw_hex_8(write_count_bcd, attribute, print_position);
	}
	text = test_menu_text_ADDRESS;
	print_text_list(&text, &ignored_3);
	if (mem_read8(moving_seat_dip) != 0u)
	{
		text = test_menu_output_test_text_ADDRESS;
		print_text_list(&text, &ignored_3);
	}
}

/* Menu item 0: flips test_continue_mode and prints INDIVIDUAL MODE or CONTINUE MODE, then back to the
   menu at once (test_screen 0, test_screen_ready 1). In continue mode each test goes on to the next
   instead of the menu. */
void toggle_test_continue_mode(void)
{
	uint32_t ignored = 0, text = 0;

	text = individual_mode_text_ADDRESS;
	test_continue_mode = ~test_continue_mode;
	if (test_continue_mode != 0u)
		text = continue_mode_text_ADDRESS;
	print_text_list(&text, &ignored);
	test_screen = 0u;
	test_screen_ready = 1u;
}

/* MEMORY TEST, all on its first frame: with interrupts off and the sound CPU held (sound_cpu_hold),
   check_rom_checksums and the work, video, sprite, palette and sound RAM tests; then video and sound set
   up again and the 0x13 results printed from memory_test_errors, whose clearing also clears
   show_eeprom_write_count, its second byte. */
void test_memory(uint32_t *p_results_left, uint32_t *p_blocks_per_row)
{
	uint8_t is_8bpp = 0, mode = 0;
	uint16_t scroll_x = 0, scroll_y = 0;
	uint32_t clear_value = 0, delay_count = 0, font_pixels = 0, ignored = 0, ignored_2 = 0, ignored_3 = 0, ignored_4 = 0, ignored_5 = 0, ignored_6 = 0, ignored_7 = 0, ignored_8 = 0, offset_entry = 0, print_position = 0, result_index = 0, text = 0;
	uint8_t cnt_bits = 0;

	if (test_screen_ready == 0u)
	{
		interrupts_off();
		test_screen_ready++;
		clear_text_layer(&ignored, &ignored_2, &print_position);
		text = memory_test_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		text = memory_test_dashes_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		memory_test_errors = 0u;
		mem_write8(sound_cpu_hold, 1u);
		delay_count = 0x1000u;
		do
		{
			delay_count--;
		} while (delay_count != 0u);
		check_rom_checksums();
		test_work_ram();
		test_video_ram();
		test_sprite_ram();
		test_palette_ram();
		test_sound_ram();
		cnt_bits = mem_read8(io_cnt);
		mem_write8(io_cnt, cnt_bits & ~2u);
		interrupts_on();
		display_off_frames = 0x78u;
		delay_count = 0x3Cu;
		do
		{
			delay_count--;
		} while (delay_count != 0u);
		load_default_palettes();
		load_text_font(&font_pixels, &ignored_4);
		init_video_registers();
		reset_objects(&ignored, &ignored_5);
		init_sound();
		init_sprites(&ignored, &ignored_6);
		clear_sprite_buckets();
		mode = 0u;
		set_screen_mode(mode);
		clear_text_layer(&clear_value, &ignored_2, &print_position);
		clear_bitmap_layer((uint8_t)clear_value, &ignored_2, &font_pixels, &ignored_4, &ignored_7, &ignored_8, &ignored_6);
		mem_write16(video_layer_disable, 0xFu);
		scroll_x = 0u;
		scroll_y = 0u;
		is_8bpp = 1u;
		set_bitmap_scroll(scroll_x, scroll_y, is_8bpp);
		mem_write32(video_control, mem_read32(video_control) | 0x800u);
		fill_value = (fill_value & ~0xFFFFu) | 0x909u;
		set_halfwords(VIDEO_RAM, 0xE000u, fill_value);
		color_offset_red = 0u;
		color_offset_blue = 0u;
		color_offset_green = 0u;
		color_offset_2_red = 0u;
		color_offset_2_blue = 0u;
		color_offset_2_green = 0u;
		text = memory_test_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		text = memory_test_done_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		text = memory_test_dashes_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		if (memory_test_errors != 0u)
		{
			text = memory_test_error_text_ADDRESS;
			print_text_list(&text, &ignored_3);
		}
		*p_results_left = 0x13u;
		result_index = 0u;
		do
		{
			offset_entry = memory_test_good_offsets_ADDRESS + result_index * 2u;
			if ((memory_test_errors & shl32(1u, (int)(result_index))) != 0u)
				offset_entry = memory_test_bad_offsets_ADDRESS + result_index * 2u;
			*p_blocks_per_row = memory_test_result_texts_ADDRESS;
			text = MEMORY_TEST_RESULT_OFFSETS(offset_entry)[0] + memory_test_result_texts_ADDRESS;
			print_text_list(&text, &ignored_3);
			result_index = (uint8_t)(result_index + 1u);
			(*p_results_left)--;
		} while (*p_results_left != 0u);
	}
	if ((0x2002u & inputs_pressed) == 0u)
	{
		return;
	}
	if (test_continue_mode != 0u)
	{
		test_screen++;
		test_screen_ready = 0u;
		return;
	}
	test_cursor = test_menu_item;
	test_screen = 0u;
	test_screen_ready = 1u;
	draw_test_menu();
}

/* The ROM part of the MEMORY TEST: compute_crc16 of 0x000000-0x01FFFF and of 0x080000-0x09EFFF,
   compute_crc16_alternate_bytes from 0x100000 and from 0x100001, each against its word of rom_checksums;
   a mismatch sets memory_test_errors 1, 4, 2 or 8 in that order. */
void BOARD_ORIGINAL(check_rom_checksums)(void)
{
	uint32_t count = 0, crc = 0, data = 0, ignored = 0, stored_crc = 0;
	uint16_t ic8_crc = 0, ic18_crc = 0;

	data = 0u;
	count = IC17_BYTES;
	compute_crc16(count, &crc, &data, &ignored);
	stored_crc = rom_checksum_ic8_ADDRESS;
	if ((uint16_t)crc != rom_checksums)
		memory_test_errors |= 1u;
	data = IC8_START;
	count = IC8_BYTES;
	compute_crc16(count, &crc, &data, &ignored);
	ic8_crc = ROM_CHIP_CHECKSUMS(stored_crc)[0];
	stored_crc += 2u;
	if ((uint16_t)crc != ic8_crc)
		memory_test_errors |= 4u;
	data = IC18_START;
	count = IC18_BYTES;
	crc = compute_crc16_alternate_bytes(data, count);
	ic18_crc = ROM_CHIP_CHECKSUMS(stored_crc)[0];
	stored_crc += 2u;
	if ((uint16_t)crc != ic18_crc)
		memory_test_errors |= 2u;
	data = IC9_START;
	count = IC9_BYTES;
	crc = compute_crc16_alternate_bytes(data, count);
	if ((uint16_t)crc != ROM_CHIP_CHECKSUMS(stored_crc)[0])
		memory_test_errors |= 8u;
}

/* Saves work RAM 0x200000-0x20FFFF in video RAM, fills it with ram_test_patterns and checks it
   (test_work_ram_verify, which copies it back). */
void test_work_ram(void)
{
	uint32_t bytes_left = 0, passes_left = 0, pattern = 0, pattern_index = 0, ram_cursor = 0;

	move_bytes(VIDEO_RAM, WORK_RAM, 0x10000u);
	ram_cursor = WORK_RAM;
	passes_left = 0x42u;
	do
	{
		pattern_index = 4u;
		do
		{
			pattern = ram_test_patterns[pattern_index];
			bytes_left = 0xF9u;
			do
			{
				mem_write8(ram_cursor, RAM_TEST_PATTERN_BYTES(pattern)[0]);
				pattern++;
				ram_cursor++;
				if ((int32_t)ram_cursor >= 0x210000)
				{
					test_work_ram_verify();
					return;
				}
				bytes_left--;
			} while (bytes_left != 0u);
			pattern_index--;
		} while (pattern_index != 0u);
		passes_left--;
	} while (passes_left != 0u);
	test_work_ram_verify();
}

/* Compares work RAM with the patterns test_work_ram wrote: a wrong byte sets 0x20 (even address) or 0x40
   (odd) in memory_test_video_word, the saved copy of memory_test_errors, so the bits land in
   memory_test_errors when work RAM is then copied back from video RAM. */
void test_work_ram_verify(void)
{
	uint32_t bytes_left = 0, passes_left = 0, pattern = 0, pattern_index = 0, ram_cursor = 0;
	uint8_t expected = 0, actual = 0;

	ram_cursor = WORK_RAM;
	passes_left = 0x42u;
	do
	{
		pattern_index = 4u;
		do
		{
			pattern = ram_test_patterns[pattern_index];
			bytes_left = 0xF9u;
			do
			{
				expected = RAM_TEST_PATTERN_BYTES(pattern)[0];
				actual = mem_read8(ram_cursor);
				pattern++;
				ram_cursor++;
				if (actual != expected)
				{
					if ((ram_cursor & 1u) != 0u)
						mem_write32(memory_test_video_word, mem_read32(memory_test_video_word) | 0x20u);
					else
						mem_write32(memory_test_video_word, mem_read32(memory_test_video_word) | 0x40u);
				}
				if ((int32_t)ram_cursor >= 0x210000)
				{
					move_bytes(WORK_RAM, VIDEO_RAM, 0x10000u);
					return;
				}
				bytes_left--;
			} while (bytes_left != 0u);
			pattern_index--;
		} while (pattern_index != 0u);
		passes_left--;
	} while (passes_left != 0u);
	move_bytes(WORK_RAM, VIDEO_RAM, 0x10000u);
}

/* Fills video RAM 0x300000-0x31FFFF with ram_test_patterns, then test_video_ram_verify. */
void test_video_ram(void)
{
	uint32_t bytes_left = 0, passes_left = 0, pattern = 0, pattern_index = 0, ram_cursor = 0;

	ram_cursor = VIDEO_RAM;
	passes_left = 0x84u;
	do
	{
		pattern_index = 4u;
		do
		{
			pattern = ram_test_patterns[pattern_index];
			bytes_left = 0xF9u;
			do
			{
				mem_write8(ram_cursor, RAM_TEST_PATTERN_BYTES(pattern)[0]);
				ram_cursor++;
				pattern++;
				if ((int32_t)ram_cursor >= 0x320000)
				{
					test_video_ram_verify();
					return;
				}
				bytes_left--;
			} while (bytes_left != 0u);
			pattern_index--;
		} while (pattern_index != 0u);
		passes_left--;
	} while (passes_left != 0u);
	test_video_ram_verify();
}

/* Compares video RAM 0x300000-0x31FFFF with the patterns: a wrong byte at an even address sets
   memory_test_errors 0x10000 (low nibble) or 0x8000 (high nibble), and any wrong byte, even ones too,
   sets 0x4000 or 0x2000. */
void test_video_ram_verify(void)
{
	uint32_t bytes_left = 0, passes_left = 0, pattern = 0, pattern_index = 0, ram_cursor = 0;
	uint8_t expected = 0, actual = 0;

	ram_cursor = VIDEO_RAM;
	passes_left = 0x84u;
	do
	{
		pattern_index = 4u;
		do
		{
			pattern = ram_test_patterns[pattern_index];
			bytes_left = 0xF9u;
			do
			{
				expected = RAM_TEST_PATTERN_BYTES(pattern)[0];
				actual = mem_read8(ram_cursor);
				ram_cursor++;
				pattern++;
				if (actual != expected)
				{
					if ((ram_cursor & 1u) != 0u)
					{
						if ((mem_read8(ram_cursor - 1u) & 0xFu) != (RAM_TEST_PATTERN_BYTES(pattern)[-1] & 0xFu))
							memory_test_errors |= 0x10000u;
						if ((mem_read8(ram_cursor - 1u) & ~0xFu) != (RAM_TEST_PATTERN_BYTES(pattern)[-1] & ~0xFu))
							memory_test_errors |= 0x8000u;
					}
					if ((mem_read8(ram_cursor - 1u) & 0xFu) != (RAM_TEST_PATTERN_BYTES(pattern)[-1] & 0xFu))
						memory_test_errors |= 0x4000u;
					if ((mem_read8(ram_cursor - 1u) & ~0xFu) != (RAM_TEST_PATTERN_BYTES(pattern)[-1] & ~0xFu))
						memory_test_errors |= 0x2000u;
				}
				if ((int32_t)ram_cursor >= 0x320000)
					return;
				bytes_left--;
			} while (bytes_left != 0u);
			pattern_index--;
		} while (pattern_index != 0u);
		passes_left--;
	} while (passes_left != 0u);
}

/* Fills sprite RAM 0x400000-0x41FFFF with ram_test_patterns, then test_sprite_ram_verify. */
void test_sprite_ram(void)
{
	uint32_t bytes_left = 0, passes_left = 0, pattern = 0, pattern_index = 0, ram_cursor = 0;

	ram_cursor = SPRITE_RAM;
	passes_left = 0x84u;
	do
	{
		pattern_index = 4u;
		do
		{
			pattern = ram_test_patterns[pattern_index];
			bytes_left = 0xF9u;
			do
			{
				mem_write8(ram_cursor, RAM_TEST_PATTERN_BYTES(pattern)[0]);
				ram_cursor++;
				pattern++;
				if ((int32_t)ram_cursor >= 0x420000)
				{
					test_sprite_ram_verify();
					return;
				}
				bytes_left--;
			} while (bytes_left != 0u);
			pattern_index--;
		} while (pattern_index != 0u);
		passes_left--;
	} while (passes_left != 0u);
	test_sprite_ram_verify();
}

/* Compares sprite RAM 0x400000-0x41FFFF with the patterns: a wrong byte at an even address sets
   memory_test_errors 0x1000 (low nibble) or 0x800 (high nibble), and any wrong byte, even ones too, sets
   0x400 or 0x200. */
void test_sprite_ram_verify(void)
{
	uint32_t bytes_left = 0, passes_left = 0, pattern = 0, pattern_index = 0, ram_cursor = 0;
	uint8_t expected = 0, actual = 0;

	ram_cursor = SPRITE_RAM;
	passes_left = 0x84u;
	do
	{
		pattern_index = 4u;
		do
		{
			pattern = ram_test_patterns[pattern_index];
			bytes_left = 0xF9u;
			do
			{
				expected = RAM_TEST_PATTERN_BYTES(pattern)[0];
				actual = mem_read8(ram_cursor);
				ram_cursor++;
				pattern++;
				if (actual != expected)
				{
					if ((ram_cursor & 1u) != 0u)
					{
						if ((mem_read8(ram_cursor - 1u) & 0xFu) != (RAM_TEST_PATTERN_BYTES(pattern)[-1] & 0xFu))
							memory_test_errors |= 0x1000u;
						if ((mem_read8(ram_cursor - 1u) & ~0xFu) != (RAM_TEST_PATTERN_BYTES(pattern)[-1] & ~0xFu))
							memory_test_errors |= 0x800u;
					}
					if ((mem_read8(ram_cursor - 1u) & 0xFu) != (RAM_TEST_PATTERN_BYTES(pattern)[-1] & 0xFu))
						memory_test_errors |= 0x400u;
					if ((mem_read8(ram_cursor - 1u) & ~0xFu) != (RAM_TEST_PATTERN_BYTES(pattern)[-1] & ~0xFu))
						memory_test_errors |= 0x200u;
				}
				if ((int32_t)ram_cursor >= 0x420000)
					return;
				bytes_left--;
			} while (bytes_left != 0u);
			pattern_index--;
		} while (pattern_index != 0u);
		passes_left--;
	} while (passes_left != 0u);
}

/* Fills palette RAM from 0x600000 with ram_test_patterns, then test_palette_ram_verify_first_half. The 8
   passes of 4 * 0xF9 words end at 0x603E40, so its exit at 0x604000 is never taken and 0x603E40-0x603FFF
   goes untested (0x607E40-0x607FFF likewise in the second half). */
void test_palette_ram(void)
{
	uint32_t halfwords_left = 0, passes_left = 0, pattern = 0, pattern_index = 0, ram_cursor = 0;

	ram_cursor = PALETTE_RAM;
	passes_left = 8u;
	do
	{
		pattern_index = 4u;
		do
		{
			pattern = ram_test_patterns[pattern_index];
			halfwords_left = 0xF9u;
			do
			{
				mem_write16(ram_cursor, RAM_TEST_PATTERN_WORDS(pattern)[0]);
				ram_cursor += 2u;
				pattern += 2u;
				if ((int32_t)ram_cursor >= 0x604000)
				{
					test_palette_ram_verify_first_half();
					return;
				}
				halfwords_left--;
			} while (halfwords_left != 0u);
			pattern_index--;
		} while (pattern_index != 0u);
		passes_left--;
	} while (passes_left != 0u);
	test_palette_ram_verify_first_half();
}

/* Compares palette RAM from 0x600000 (up to 0x603E40, as filled) with the patterns: a wrong low byte
   sets memory_test_errors 0x100, a wrong high byte 0x80; then test_palette_ram_fill_second_half. */
void test_palette_ram_verify_first_half(void)
{
	uint32_t halfwords_left = 0, passes_left = 0, pattern = 0, pattern_index = 0, ram_cursor = 0;
	uint16_t expected_first = 0, actual_first = 0;

	ram_cursor = PALETTE_RAM;
	passes_left = 8u;
	do
	{
		pattern_index = 4u;
		do
		{
			pattern = ram_test_patterns[pattern_index];
			halfwords_left = 0xF9u;
			do
			{
				expected_first = RAM_TEST_PATTERN_WORDS(pattern)[0];
				actual_first = mem_read16(ram_cursor);
				ram_cursor += 2u;
				pattern += 2u;
				if (actual_first != expected_first)
				{
					if ((mem_read16(ram_cursor - 2u) & 0xFFu) != (RAM_TEST_PATTERN_WORDS(pattern)[-1] & 0xFFu))
						memory_test_errors |= 0x100u;
					if ((mem_read16(ram_cursor - 2u) & ~0xFFu) != (RAM_TEST_PATTERN_WORDS(pattern)[-1] & ~0xFFu))
						memory_test_errors |= 0x80u;
				}
				if ((int32_t)ram_cursor >= 0x604000)
				{
					test_palette_ram_fill_second_half();
					return;
				}
				halfwords_left--;
			} while (halfwords_left != 0u);
			pattern_index--;
		} while (pattern_index != 0u);
		passes_left--;
	} while (passes_left != 0u);
	test_palette_ram_fill_second_half();
}

/* Fills palette RAM from 0x604000 with the patterns (8 passes, up to 0x607E40), then
   test_palette_ram_verify_second_half. */
void test_palette_ram_fill_second_half(void)
{
	uint32_t halfwords_left = 0, passes_left = 0, pattern = 0, pattern_index = 0, ram_cursor = 0;

	ram_cursor = PALETTE_RAM + 0x4000u;
	passes_left = 8u;
	do
	{
		pattern_index = 4u;
		do
		{
			pattern = ram_test_patterns[pattern_index];
			halfwords_left = 0xF9u;
			do
			{
				mem_write16(ram_cursor, RAM_TEST_PATTERN_WORDS(pattern)[0]);
				ram_cursor += 2u;
				pattern += 2u;
				if ((int32_t)ram_cursor >= 0x608000)
				{
					test_palette_ram_verify_second_half();
					return;
				}
				halfwords_left--;
			} while (halfwords_left != 0u);
			pattern_index--;
		} while (pattern_index != 0u);
		passes_left--;
	} while (passes_left != 0u);
	test_palette_ram_verify_second_half();
}

/* Compares palette RAM from 0x604000 (up to 0x607E40, as filled) with the patterns: a wrong low byte
   sets memory_test_errors 0x40000, a wrong high byte 0x20000. */
void test_palette_ram_verify_second_half(void)
{
	uint32_t halfwords_left = 0, passes_left = 0, pattern = 0, pattern_index = 0, ram_cursor = 0;
	uint16_t expected_second = 0, actual_second = 0;

	ram_cursor = PALETTE_RAM + 0x4000u;
	passes_left = 8u;
	do
	{
		pattern_index = 4u;
		do
		{
			pattern = ram_test_patterns[pattern_index];
			halfwords_left = 0xF9u;
			do
			{
				expected_second = RAM_TEST_PATTERN_WORDS(pattern)[0];
				actual_second = mem_read16(ram_cursor);
				ram_cursor += 2u;
				pattern += 2u;
				if (actual_second != expected_second)
				{
					if ((mem_read16(ram_cursor - 2u) & 0xFFu) != (RAM_TEST_PATTERN_WORDS(pattern)[-1] & 0xFFu))
						memory_test_errors |= 0x40000u;
					if ((mem_read16(ram_cursor - 2u) & ~0xFFu) != (RAM_TEST_PATTERN_WORDS(pattern)[-1] & ~0xFFu))
						memory_test_errors |= 0x20000u;
				}
				if ((int32_t)ram_cursor >= 0x608000)
					return;
				halfwords_left--;
			} while (halfwords_left != 0u);
			pattern_index--;
		} while (pattern_index != 0u);
		passes_left--;
	} while (passes_left != 0u);
}

/* Saves sound RAM 0x700000-0x701FFF in video RAM and fills it with ram_test_patterns, then
   test_sound_ram_verify. */
void test_sound_ram(void)
{
	uint32_t bytes_left = 0, passes_left = 0, pattern = 0, pattern_index = 0, ram_cursor = 0;

	move_halfwords(VIDEO_RAM, SOUND_RAM, 0x1000u);
	ram_cursor = SOUND_RAM;
	passes_left = 9u;
	do
	{
		pattern_index = 4u;
		do
		{
			pattern = ram_test_patterns[pattern_index];
			bytes_left = 0xF9u;
			do
			{
				mem_write8(ram_cursor, RAM_TEST_PATTERN_BYTES(pattern)[0]);
				pattern++;
				ram_cursor++;
				if ((int32_t)ram_cursor >= 0x702000)
				{
					test_sound_ram_verify();
					return;
				}
				bytes_left--;
			} while (bytes_left != 0u);
			pattern_index--;
		} while (pattern_index != 0u);
		passes_left--;
	} while (passes_left != 0u);
	test_sound_ram_verify();
}

/* Compares sound RAM 0x700000-0x701FFF with the patterns (any wrong byte sets memory_test_errors 0x10),
   then copies the saved contents back from video RAM. */
void test_sound_ram_verify(void)
{
	uint32_t bytes_left = 0, passes_left = 0, pattern = 0, pattern_index = 0, ram_cursor = 0;
	uint8_t expected = 0, actual = 0;

	ram_cursor = SOUND_RAM;
	passes_left = 9u;
	do
	{
		pattern_index = 4u;
		do
		{
			pattern = ram_test_patterns[pattern_index];
			bytes_left = 0xF9u;
			do
			{
				expected = RAM_TEST_PATTERN_BYTES(pattern)[0];
				actual = mem_read8(ram_cursor);
				pattern++;
				ram_cursor++;
				if (actual != expected)
					memory_test_errors |= 0x10u;
				if ((int32_t)ram_cursor >= 0x702000)
				{
					move_bytes(SOUND_RAM, VIDEO_RAM, 0x2000u);
					return;
				}
				bytes_left--;
			} while (bytes_left != 0u);
			pattern_index--;
		} while (pattern_index != 0u);
		passes_left--;
	} while (passes_left != 0u);
	move_bytes(SOUND_RAM, VIDEO_RAM, 0x2000u);
}

/* INPUT TEST: on its first frame prints the texts, fills its panels on the bitmap layer (a third for 3
   players) and sets up three trackball cursors in one pool 2 block (screen_sprite_block); then every
   frame each switch shows ON or OFF from input_ports_active and the cursors move
   (move_input_test_cursors). Test, pressed and released, leaves. */
void test_inputs(uint16_t count, uint32_t *p_panel_color, uint32_t *p_blocks_per_row)
{
	uint16_t sprite_block = 0;
	uint32_t bottom = 0, coin_text = 0, first_panel_color = 0, ignored = 0, ignored_2 = 0, ignored_3 = 0, ignored_4 = 0, ignored_5 = 0, input_bits = 0, input_index = 0, inputs_left = 0, left = 0, object = 0, off_texts = 0, on_texts = 0, right = 0, text = 0, text_entry = 0, top = 0, value = 0;

	if (test_screen_ready != 0u)
	{
		if (setting_no_start_buttons != 0u)
		{
			inputs_left = 6u;
			input_bits = input_test_bits_no_start_2p_ADDRESS;
			on_texts = input_test_on_texts_no_start_2p_ADDRESS;
			off_texts = input_test_off_texts_no_start_2p_ADDRESS;
			if (setting_cabinet_type != 0u)
			{
				inputs_left = 7u;
				input_bits = input_test_bits_no_start_3p_ADDRESS;
				on_texts = input_test_on_texts_no_start_3p_ADDRESS;
				off_texts = input_test_off_texts_no_start_3p_ADDRESS;
			}
		}
		else
		{
			inputs_left = 8u;
			input_bits = input_test_bits_2p_ADDRESS;
			on_texts = input_test_on_texts_2p_ADDRESS;
			off_texts = input_test_off_texts_2p_ADDRESS;
			if (setting_cabinet_type != 0u)
			{
				inputs_left = 0xAu;
				input_bits = input_test_bits_3p_ADDRESS;
				on_texts = input_test_on_texts_3p_ADDRESS;
				off_texts = input_test_off_texts_3p_ADDRESS;
			}
		}
		do
		{
			input_index = (inputs_left & ~0xFFu) | (uint8_t)(inputs_left - 1u);
			if ((input_ports_active & shl32(1u, (int)(INPUT_TEST_BIT_NUMBERS(input_bits)[input_index]))) == 0u)
				text_entry = off_texts + input_index * 4u;
			else
				text_entry = on_texts + input_index * 4u;
			text = INPUT_TEST_TEXTS(text_entry)[0];
			print_text_list(&text, &ignored_4);
			inputs_left--;
		} while (inputs_left != 0u);
		text = input_test_coin_1_on_text_ADDRESS;
		if ((input_ports_active & 4u) == 0u)
			text = input_test_coin_1_off_text_ADDRESS;
		print_text_list(&text, &ignored_4);
		coin_text = input_test_coin_2_on_text_ADDRESS;
		if ((input_ports_active & 8u) == 0u)
			coin_text = input_test_coin_2_off_text_ADDRESS;
		print_text_list(&coin_text, &ignored_4);
		if (setting_cabinet_type != 0u && setting_coin_chute_type != 0u)
		{
			coin_text = input_test_coin_3_on_text_ADDRESS;
			if ((input_ports_active & 0x40u) == 0u)
				coin_text = input_test_coin_3_off_text_ADDRESS;
			print_text_list(&coin_text, &ignored_4);
		}
		move_input_test_cursors(count, coin_text, p_panel_color, p_blocks_per_row);
		if ((input_ports_active & 0x2002u) != 0u && input_test_switch_released == 0u)
		{
			return;
		}
		input_test_switch_released = 1u;
		if ((input_ports_active & 0x2002u) != 0u)
		{
			input_test_switch_pressed = 1u;
			return;
		}
		if (input_test_switch_pressed == 0u)
		{
			return;
		}
		sprite_block = screen_sprite_block;
		free_object_2(sprite_block);
		init_sprites(&ignored_2, &ignored_5);
		mem_write32(video_control, mem_read32(video_control) | 0x800u);
		fill_value = (fill_value & ~0xFFFFu) | 0x909u;
		set_halfwords(VIDEO_RAM, 0xE000u, fill_value);
		if (test_continue_mode != 0u)
		{
			test_screen++;
			test_screen_ready = 0u;
			return;
		}
		test_cursor = test_menu_item;
		test_screen = 0u;
		test_screen_ready = 1u;
		draw_test_menu();
		return;
	}
	test_screen_ready = 1u;
	input_test_switch_pressed = 0u;
	input_test_switch_released = 0u;
	clear_text_layer(&value, &ignored, &object);
	mem_write16(mixer_bitmap, 0x21u);
	text = input_test_text_ADDRESS;
	print_text_list(&text, &object);
	if (setting_cabinet_type != 0u)
	{
		text = input_test_text_3p_ADDRESS;
		print_text_list(&text, &object);
	}
	text = input_test_chute_2_text_ADDRESS;
	print_text_list(&text, &object);
	if (setting_coin_chute_type != 0u && setting_cabinet_type != 0u)
	{
		text = input_test_chute_3_text_ADDRESS;
		print_text_list(&text, &object);
	}
	if (setting_no_start_buttons != 0u)
	{
		text = input_test_start_erase_text_ADDRESS;
		print_text_list(&text, &object);
	}
	left = (value & ~0xFFFFu) | 0xA0u;
	top = 0x30u;
	right = 0x138u;
	bottom = 0x60u;
	first_panel_color = (*p_panel_color & ~0xFFFFu) | 2u;
	if ((mem_read32(video_control) & 0x200u) != 0u)
		flip_test_coordinates(&left, &top, &right, &bottom);
	fill_rect(top, right, bottom, &left, &first_panel_color, p_blocks_per_row);
	left = (left & ~0xFFFFu) | 0xA0u;
	top = 0x68u;
	right = 0x138u;
	bottom = 0x98u;
	*p_panel_color = (first_panel_color & ~0xFFFFu) | 0xEu;
	if ((mem_read32(video_control) & 0x200u) != 0u)
		flip_test_coordinates(&left, &top, &right, &bottom);
	fill_rect(top, right, bottom, &left, p_panel_color, p_blocks_per_row);
	if (setting_cabinet_type != 0u)
	{
		left = (left & ~0xFFFFu) | 0xA0u;
		top = 0xA0u;
		right = 0x138u;
		bottom = 0xD0u;
		*p_panel_color = (*p_panel_color & ~0xFFFFu) | 6u;
		if ((mem_read32(video_control) & 0x200u) != 0u)
			flip_test_coordinates(&left, &top, &right, &bottom);
		fill_rect(top, right, bottom, &left, p_panel_color, p_blocks_per_row);
	}
	allocate_object_2(&ignored_2, &ignored_3, &object);
	screen_sprite_block = object;
	current_object = object;
	OBJECT(current_object)->screen_x = 0xEC;
	OBJECT(current_object)->handler_high = 0xECu;
	OBJECT(current_object)->projected_y_int = 0xEC;
	OBJECT(current_object)->screen_y = 0x48;
	OBJECT(current_object)->handler_low = 0x80u;
	OBJECT(current_object)->projected_y_fraction = 0xB8u;
	OBJECT(current_object)->sprite_layer = 0u;
	OBJECT(current_object)->sprite_width = 0;
	OBJECT(current_object)->projected_x = 0;
	OBJECT(current_object)->sprite_scale = 0x80u;
	OBJECT(current_object)->sprite_height = 0x80;
	OBJECT(current_object)->child = 0x80u;
	OBJECT(current_object)->sprite_frame = 0x1120u;
	OBJECT(current_object)->anchor_x = 0x1120;
	OBJECT(current_object)->direction = 0x1120u;
	OBJECT(current_object)->sprite_palette = 0x130u;
	OBJECT(current_object)->anchor_y = 0x140;
	OBJECT(current_object)->parent = 0x150u;
	OBJECT(current_object)->sprite_flags = 0u;
	OBJECT(current_object)->hitbox_size_y = 0u;
	OBJECT(current_object)->next = 0u;
	trackball_1_quadrants = 0u;
	trackball_2_quadrants = 0u;
	trackball_3_quadrants = 0u;
}

/* Queues the input test's cursors (the third for 3 players), prints GOOD for each trackball whose
   quadrant mask is 0xF, draws the sprites (build_sprite_list) and moves each cursor by its trackball /
   4, within x 0xA5-0x132 and its panel's rows. Cursors 2 and 3 show as handler_high/handler_low and
   projected_y_int/projected_y_fraction. */
void move_input_test_cursors(uint32_t count, uint32_t text, uint32_t *p_cursors_left, uint32_t *p_height)
{
	uint16_t trackball_dx = 0, trackball_dy = 0;
	uint32_t bottom_limit = 0, cursors_left = 0, ignored = 0, ignored_2 = 0, ignored_3 = 0, quadrant_bit = 0, sprite_entry = 0, top_limit = 0;

	current_object = (current_object & ~0xFFFFu) | screen_sprite_block;
	queue_sprite_in_layer();
	current_object = current_object + 0x10u;
	queue_sprite_in_layer();
	if (trackball_1_quadrants == 0xFu)
	{
		text = input_test_trackball_1_good_text_ADDRESS;
		print_text_list(&text, &ignored);
	}
	if (trackball_2_quadrants == 0xFu)
	{
		text = input_test_trackball_2_good_text_ADDRESS;
		print_text_list(&text, &ignored);
	}
	if (setting_cabinet_type != 0u)
	{
		current_object = current_object + 0x10u;
		queue_sprite_in_layer();
		if (trackball_3_quadrants == 0xFu)
		{
			text = input_test_trackball_3_good_text_ADDRESS;
			print_text_list(&text, &ignored);
		}
	}
	build_sprite_list(&ignored_2, &count, p_height, &text, &sprite_entry);
	draw_queued_strings((uint16_t)count, (uint16_t)text, sprite_entry, &ignored_2, &ignored_3, p_height);
	current_object = (current_object & ~0xFFFFu) | screen_sprite_block;
	trackball_dx = (uint16_t)trackball_1_x;
	trackball_dy = (uint16_t)trackball_1_y;
	quadrant_bit = get_trackball_quadrant((uint8_t)trackball_dx, (uint8_t)trackball_dy, 0u);
	trackball_1_quadrants |= quadrant_bit;
	OBJECT(current_object)->screen_x += (int16_t)trackball_dx >> 2;
	OBJECT(current_object)->screen_y += (int16_t)trackball_dy >> 2;
	trackball_dx = (uint16_t)trackball_2_x;
	trackball_dy = (uint16_t)trackball_2_y;
	quadrant_bit = get_trackball_quadrant((uint8_t)trackball_dx, (uint8_t)trackball_dy, quadrant_bit);
	trackball_2_quadrants |= quadrant_bit;
	OBJECT(current_object)->handler_high += (int16_t)trackball_dx >> 2;
	OBJECT(current_object)->handler_low += (int16_t)trackball_dy >> 2;
	trackball_dx = (uint16_t)trackball_3_x;
	trackball_dy = (uint16_t)trackball_3_y;
	quadrant_bit = get_trackball_quadrant((uint8_t)trackball_dx, (uint8_t)trackball_dy, quadrant_bit);
	trackball_3_quadrants |= quadrant_bit;
	OBJECT(current_object)->projected_y_int += (int16_t)trackball_dx >> 2;
	OBJECT(current_object)->projected_y_fraction += (int16_t)trackball_dy >> 2;
	current_object = (current_object & ~0xFFFFu) | screen_sprite_block;
	cursors_left = 3u;
	do
	{
		if (OBJECT(current_object)->screen_x <= 0xA5)
			OBJECT(current_object)->screen_x = 0xA5;
		current_object = current_object + 0x10u;
		cursors_left--;
	} while (cursors_left != 0u);
	current_object = (current_object & ~0xFFFFu) | screen_sprite_block;
	cursors_left = 3u;
	do
	{
		if (OBJECT(current_object)->screen_x >= 0x132)
			OBJECT(current_object)->screen_x = 0x132;
		current_object = current_object + 0x10u;
		cursors_left--;
	} while (cursors_left != 0u);
	current_object = (current_object & ~0xFFFFu) | screen_sprite_block;
	cursors_left = 3u;
	do
	{
		bottom_limit = input_test_cursor_bottom_limits_ADDRESS + cursors_left * 2u;
		if (INPUT_TEST_CURSOR_LIMITS(bottom_limit)[0] <= OBJECT(current_object)->screen_y)
			OBJECT(current_object)->screen_y = INPUT_TEST_CURSOR_LIMITS(bottom_limit)[0];
		current_object = current_object + 0x10u;
		cursors_left--;
	} while (cursors_left != 0u);
	current_object = (current_object & ~0xFFFFu) | screen_sprite_block;
	cursors_left = 3u;
	do
	{
		top_limit = input_test_cursor_top_limits_ADDRESS + cursors_left * 2u;
		if (OBJECT(current_object)->screen_y <= INPUT_TEST_CURSOR_LIMITS(top_limit)[0])
			OBJECT(current_object)->screen_y = INPUT_TEST_CURSOR_LIMITS(top_limit)[0];
		current_object = current_object + 0x10u;
		cursors_left--;
	} while (cursors_left != 0u);
	*p_cursors_left = cursors_left;
}

/* quadrant's low byte = the direction bit of a trackball move: 1 (dx and dy not negative), 2 (dy
   negative), 4 (dx negative) or 8 (both negative); a trackball at rest counts as 1. */
uint32_t get_trackball_quadrant(uint8_t dx, uint8_t dy, uint32_t quadrant)
{
	if ((int8_t)dx < 0)
	{
		if ((int8_t)dy < 0)
			quadrant = (quadrant & ~0xFFu) | 8u;
		else
			quadrant = (quadrant & ~0xFFu) | 4u;
	}
	else if ((int8_t)dy < 0)
		quadrant = (quadrant & ~0xFFu) | 2u;
	else
		quadrant = (quadrant & ~0xFFu) | 1u;
	return quadrant;
}

/* SOUND TEST: sound_test_number shown at 20,12; service steps it 0-0x8C (wrapping) and plays it after
   sounds 0x94 and 0x95 (number 0: 0x94 and 0x95 twice); test plays 0x94 and leaves. */
void test_sound(void)
{
	uint8_t sound_id = 0;
	uint32_t attribute = 0, ignored = 0, ignored_2 = 0, ignored_3 = 0, ignored_4 = 0, print_position = 0, text = 0, text_position = 0, value = 0;

	if (test_screen_ready == 0u)
	{
		clear_text_layer(&ignored, &ignored_2, &print_position);
		sound_test_number = 0u;
		text = sound_test_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		test_screen_ready++;
		sound_id = 0x94u;
		queue_sound(sound_id, &ignored_2, &ignored_4);
		value = sound_test_number;
		convert_to_bcd(&value, &ignored_2);
		text_position = TEXT_CELL(20, 12);
		attribute = 0x200u;
		draw_hex_4(&value, &attribute, &text_position);
		return;
	}
	if ((0x2002u & inputs_pressed) != 0u)
	{
		sound_id = 0x94u;
		queue_sound(sound_id, &ignored_2, &ignored_4);
		if (test_continue_mode != 0u)
		{
			test_screen++;
			test_screen_ready = 0u;
			return;
		}
		test_cursor = test_menu_item;
		test_screen = 0u;
		test_screen_ready = 1u;
		draw_test_menu();
		return;
	}
	if ((0x1001u & inputs_pressed) == 0u)
		return;
	sound_test_number++;
	sound_id = 0x94u;
	queue_sound(sound_id, &ignored_2, &ignored_4);
	sound_id = 0x95u;
	queue_sound(sound_id, &ignored_2, &ignored_4);
	wrap_sound_test_number();
	sound_id = sound_test_number;
	if (sound_test_number == 0u)
	{
		sound_id = 0x94u;
		queue_sound(sound_id, &ignored_2, &ignored_4);
		sound_id = 0x95u;
	}
	queue_sound(sound_id, &ignored_2, &ignored_4);
	value = sound_test_number;
	convert_to_bcd(&value, &ignored_2);
	text_position = TEXT_CELL(20, 12);
	attribute = 0x200u;
	draw_hex_4(&value, &attribute, &text_position);
}

/* sound_test_number past 0x8C goes back to 0. */
void wrap_sound_test_number(void)
{
	if ((int16_t)(sound_test_number - 0x8Cu) > 0)
		sound_test_number = 0u;
}

/* Unused: sound_test_number past 0x8C (0 stepped down gives 0xFF) becomes 0x8C, the downward twin of
   wrap_sound_test_number; test_sound only steps up. */
void wrap_sound_test_number_down(void)
{
	if ((int16_t)sound_test_number > 0x8C)
		sound_test_number = 0x8Cu;
}

/* C.R.T. TEST, by test_screen_ready: 1/2 is four rows of 0x20 colour bars (crt_test_color_bars) on the
   bitmap layer; test shows 2/2, a border (crt_test_border_lines) and a crosshatch of lines every 8
   pixels in colour 0x7E; test again fills the bitmap with 9 and leaves. */
void test_crt(uint32_t start_y, uint32_t *p_color, uint32_t *p_blocks_per_row)
{
	uint32_t color = *p_color;
	uint16_t end_x = 0, end_y = 0;
	uint32_t bar_record = 0, bars_left = 0, bottom = 0, ignored = 0, ignored_2 = 0, ignored_3 = 0, line_record = 0, lines_left = 0, print_position = 0, right = 0, rows_left = 0, start_x = 0, text = 0, top = 0;

	if (test_screen_ready == 0u)
	{
		test_screen_ready = 1u;
		clear_text_layer(&ignored, &start_y, &print_position);
		text = crt_test_1_text_ADDRESS;
		print_text_list(&text, &ignored_2);
	}
	start_x = test_screen_ready * 3u;
	switch (test_screen_ready)
	{
	case 0:
	case 5:
		break;
	case 1:
		bar_record = crt_test_color_bars_ADDRESS;
		rows_left = 4u;
		do
		{
			crt_test_x = CRT_TEST_COLOR_BAR_ROW(bar_record)->x;
			bar_record += 2u;
			crt_test_y = CRT_TEST_COLOR_BAR_ROW(bar_record - 0x2u)->y;
			bar_record += 2u;
			crt_test_color = CRT_TEST_COLOR_BAR_ROW(bar_record - 0x4u)->first_color;
			bars_left = 0x20u;
			bar_record++;
			do
			{
				start_x = (start_x & ~0xFFFFu) | crt_test_x;
				top = crt_test_y;
				color = (color & ~0xFFu) | crt_test_color;
				right = (uint16_t)(start_x + 7u);
				bottom = (uint16_t)(top + 0x1Fu);
				if ((mem_read32(video_control) & 0x200u) != 0u)
					flip_test_coordinates(&start_x, &top, &right, &bottom);
				fill_rect(top, right, bottom, &start_x, &color, p_blocks_per_row);
				crt_test_x += 8u;
				crt_test_color++;
				bars_left--;
			} while (bars_left != 0u);
			rows_left--;
		} while (rows_left != 0u);
		test_screen_ready++;
		break;
	case 2:
		if ((0x2002u & inputs_pressed) != 0u)
		{
			clear_text_layer(&ignored, &ignored_3, &ignored_2);
			test_screen_ready++;
		}
		break;
	case 3:
		mem_write32(video_control, mem_read32(video_control) | 0x800u);
		fill_value = fill_value & ~0xFFFFu;
		set_halfwords(VIDEO_RAM, 0xE000u, fill_value);
		text = crt_test_2_text_ADDRESS;
		print_text_list(&text, &ignored_2);
		line_record = crt_test_border_lines_ADDRESS;
		lines_left = 4u;
		do
		{
			start_x = (start_x & ~0xFFFFu) | CRT_TEST_LINE(line_record)->start_x;
			line_record += 2u;
			start_y = (start_y & ~0xFFFFu) | CRT_TEST_LINE(line_record - 0x2u)->start_y;
			line_record += 2u;
			end_x = CRT_TEST_LINE(line_record - 0x4u)->end_x;
			line_record += 2u;
			end_y = CRT_TEST_LINE(line_record - 0x6u)->end_y;
			color = (color & ~0xFFu) | 0x7Eu;
			line_record += 2u;
			draw_line(end_x, end_y, &start_x, &start_y, &color, &lines_left);
			lines_left--;
		} while (lines_left != 0u);
		crt_test_x = 0u;
		crt_test_y = 0u;
		lines_left = 0x1Cu;
		do
		{
			start_x &= ~0xFFFFu;
			start_y = (start_y & ~0xFFFFu) | crt_test_x;
			end_x = 0x13Fu;
			end_y = crt_test_x;
			color = (color & ~0xFFFFu) | 0x7Eu;
			draw_line(end_x, end_y, &start_x, &start_y, &color, &lines_left);
			crt_test_x += 8u;
			lines_left--;
		} while (lines_left != 0u);
		lines_left = 0x28u;
		do
		{
			start_x = (start_x & ~0xFFFFu) | crt_test_y;
			start_y &= ~0xFFFFu;
			end_x = crt_test_y;
			end_y = 0xDFu;
			color = (color & ~0xFFFFu) | 0x7Eu;
			draw_line(end_x, end_y, &start_x, &start_y, &color, &lines_left);
			crt_test_y += 8u;
			lines_left--;
		} while (lines_left != 0u);
		test_screen_ready++;
		break;
	case 4:
		if ((0x2002u & inputs_pressed) != 0u)
		{
			mem_write32(video_control, mem_read32(video_control) | 0x800u);
			fill_value = (fill_value & ~0xFFFFu) | 0x909u;
			set_halfwords(VIDEO_RAM, 0xE000u, fill_value);
			if (test_continue_mode != 0u)
			{
				test_screen++;
				test_screen_ready = 0u;
			}
			else
			{
				test_cursor = test_menu_item;
				test_screen = 0u;
				test_screen_ready = 1u;
				draw_test_menu();
			}
		}
		break;
	default:
		invalid_jump(0x9D8CCu + test_screen_ready * 3u);
	}
	*p_color = color;
}

/* GAME ASSIGNMENTS: service moves the cursor (past MOVING SEAT without moving_seat_dip), test changes
   difficulty, vitality, advertise sound, monitor flip, cabinet type (type 1 turns the moving seat off),
   start buttons or moving seat; EXIT saves the EEPROM when test_settings_changed. Item 8,
   setting_small_large, is past the cursor list. */
void test_game_assignments(uint32_t *p_color, uint32_t *p_blocks_per_row)
{
	uint8_t mode = 0;
	uint32_t cabinet_type = 0, cursor_positions = 0, ignored = 0, ignored_2 = 0, ignored_3 = 0, ignored_6 = 0, text = 0;

	if (test_screen_ready == 0u)
	{
		test_settings_changed = 0u;
		test_screen_ready++;
		clear_text_layer(&ignored, &ignored_2, &ignored_3);
		text = game_assignments_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		test_cursor_positions = game_assignments_cursor_positions_ADDRESS;
		test_cursor = 7u;
		draw_difficulty_setting(p_color, p_blocks_per_row);
		draw_vitality_setting(p_color, p_blocks_per_row);
		text = advertise_sound_off_text_ADDRESS;
		setting_advertise_sound &= 1u;
		if (setting_advertise_sound != 0u)
			text = advertise_sound_on_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		text = monitor_flip_normal_text_ADDRESS;
		if ((setting_monitor_flip & 2u) != 0u)
			text = monitor_flip_reverse_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		cabinet_type = setting_cabinet_type;
		text = cabinet_type_texts[cabinet_type & 1u];
		print_text_list(&text, &ignored_3);
		text = start_button_use_text_ADDRESS;
		setting_no_start_buttons &= 1u;
		if (setting_no_start_buttons != 0u)
			text = start_button_no_use_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		if ((setting_cabinet_type & 1u) != 0u && setting_moving_seat != 0u)
		{
			setting_moving_seat = 0u;
			test_settings_changed = 1u;
		}
		text = moving_seat_no_use_text_ADDRESS;
		if (setting_moving_seat != 0u)
			text = moving_seat_use_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		if (mem_read8(moving_seat_dip) == 0u)
		{
			text = moving_seat_erase_text_ADDRESS;
			print_text_list(&text, &ignored_3);
		}
	}
	cursor_positions = game_assignments_cursor_positions_ADDRESS;
	erase_test_cursor(cursor_positions);
	if ((0x1001u & inputs_pressed) != 0u)
	{
		test_cursor++;
		if (test_cursor == 6u && mem_read8(moving_seat_dip) == 0u)
			test_cursor++;
		set_test_cursor();
	}
	blink_test_cursor();
	if ((0x2002u & inputs_pressed) == 0u)
	{
		return;
	}
	switch (test_cursor)
	{
	case 0:
		test_settings_changed = 1u;
		setting_difficulty++;
		draw_difficulty_setting(p_color, p_blocks_per_row);
		return;
	case 1:
		test_settings_changed = 1u;
		setting_initial_vitality++;
		draw_vitality_setting(p_color, p_blocks_per_row);
		return;
	case 2:
		test_settings_changed = 1u;
		setting_advertise_sound++;
		setting_advertise_sound &= 1u;
		text = advertise_sound_off_text_ADDRESS;
		if (setting_advertise_sound == 0u)
		{
			print_text_list(&text, &ignored_3);
			return;
		}
		text = advertise_sound_on_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		return;
	case 3:
		test_settings_changed = 1u;
		setting_monitor_flip += 2u;
		setting_monitor_flip &= 2u;
		text = monitor_flip_normal_text_ADDRESS;
		if (setting_monitor_flip != 0u)
			text = monitor_flip_reverse_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		mem_write32(video_control, mem_read32(video_control) | 0x800u);
		fill_value = (fill_value & ~0xFFFFu) | 0x909u;
		set_halfwords(VIDEO_RAM, 0xE000u, fill_value);
		mode = 0u;
		set_screen_mode(mode);
		draw_difficulty_setting(p_color, p_blocks_per_row);
		draw_vitality_setting(p_color, p_blocks_per_row);
		return;
	case 4:
		test_settings_changed = 1u;
		setting_cabinet_type++;
		setting_cabinet_type &= 1u;
		text = cabinet_type_texts[setting_cabinet_type];
		print_text_list(&text, &ignored_3);
		if (mem_read8(moving_seat_dip) == 0u || setting_cabinet_type == 0u)
		{
			return;
		}
		setting_moving_seat = 1u;
		test_settings_changed = 1u;
		setting_moving_seat++;
		setting_moving_seat &= 1u;
		text = moving_seat_no_use_text_ADDRESS;
		if (setting_moving_seat == 0u)
		{
			print_text_list(&text, &ignored_3);
			return;
		}
		text = moving_seat_use_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		return;
	case 5:
		test_settings_changed = 1u;
		setting_no_start_buttons++;
		setting_no_start_buttons &= 1u;
		text = start_button_use_text_ADDRESS;
		if (setting_no_start_buttons == 0u)
		{
			print_text_list(&text, &ignored_3);
			return;
		}
		text = start_button_no_use_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		return;
	case 6:
		if (setting_cabinet_type != 0u)
		{
			return;
		}
		test_settings_changed = 1u;
		setting_moving_seat++;
		setting_moving_seat &= 1u;
		text = moving_seat_no_use_text_ADDRESS;
		if (setting_moving_seat == 0u)
		{
			print_text_list(&text, &ignored_3);
			return;
		}
		text = moving_seat_use_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		return;
	case 7:
		setting_cabinet_config = setting_cabinet_type | ((uint32_t)setting_coin_chute_type << 1);
		if (test_settings_changed != 0u)
			eeprom_save_and_lock(p_color, &ignored_6);
		mem_write32(video_control, mem_read32(video_control) | 0x800u);
		fill_value = (fill_value & ~0xFFFFu) | 0x909u;
		set_halfwords(VIDEO_RAM, 0xE000u, fill_value);
		if (test_continue_mode != 0u)
		{
			test_screen++;
			test_screen_ready = 0u;
			return;
		}
		test_cursor = test_menu_item;
		test_screen = 0u;
		test_screen_ready = 1u;
		draw_test_menu();
		return;
	case 8:
		test_settings_changed = 1u;
		setting_small_large++;
		setting_small_large &= 1u;
		text = small_large_texts[setting_small_large];
		print_text_list(&text, &ignored_3);
		return;
	default:
		invalid_jump(0x9DBDBu + test_cursor * 3u);
	}
}

/* Draws setting_initial_vitality (0-3, back to 0 when out of range) as its vitality_setting_numbers
   value at text cell 25,9 and as a bar on the bitmap layer: colour 0x17 from x 0x78 to 0xF8, then colour
   0x49 over (n + 1) * 0x20 pixels, rows 0x58-0x60; flipped coordinates on a flipped screen. */
void draw_vitality_setting(uint32_t *p_color, uint32_t *p_blocks_per_row)
{
	uint32_t attribute = 0, bar_back_color = 0, bottom = 0, left = 0, right = 0, text_position = 0, top = 0, vitality_number = 0;

	if ((setting_initial_vitality & ~3u) != 0u)
		setting_initial_vitality = 0u;
	vitality_number = vitality_setting_numbers[setting_initial_vitality];
	text_position = TEXT_CELL(25, 9);
	attribute = 0x200u;
	draw_hex_4(&vitality_number, &attribute, &text_position);
	left = (vitality_number & ~0xFFFFu) | 0x78u;
	top = 0x58u;
	right = 0xF8u;
	bottom = 0x60u;
	bar_back_color = (*p_color & ~0xFFu) | 0x17u;
	if ((mem_read32(video_control) & 0x200u) != 0u)
		flip_test_coordinates(&left, &top, &right, &bottom);
	fill_rect(top, right, bottom, &left, &bar_back_color, p_blocks_per_row);
	left = (left & ~0xFFFFu) | 0x78u;
	top = 0x58u;
	bottom = 0x60u;
	right = (uint16_t)(((setting_initial_vitality + 1u) << 5) + left);
	*p_color = (bar_back_color & ~0xFFu) | 0x49u;
	if ((mem_read32(video_control) & 0x200u) == 0u)
	{
		fill_rect(top, right, bottom, &left, p_color, p_blocks_per_row);
		return;
	}
	flip_test_coordinates(&left, &top, &right, &bottom);
	fill_rect(top, right, bottom, &left, p_color, p_blocks_per_row);
}

/* Draws setting_difficulty (0-3, back to 0 when out of range) as its number + 1 at text cell 27,5 and as
   a bar on the bitmap layer: colour 0x17 from x 0x78 to 0xF8, then colour 0x49 over (n + 1) * 0x20
   pixels, rows 0x38-0x40; flipped coordinates on a flipped screen. */
void draw_difficulty_setting(uint32_t *p_color, uint32_t *p_blocks_per_row)
{
	uint16_t attribute = 0;
	uint32_t bar_back_color = 0, bottom = 0, difficulty_number = 0, left = 0, right = 0, text_position = 0, top = 0;

	if ((setting_difficulty & ~3u) != 0u)
		setting_difficulty = 0u;
	difficulty_number = (uint8_t)(setting_difficulty + 1u);
	text_position = TEXT_CELL(27, 5);
	attribute = 0x200u;
	difficulty_number = draw_hex_2(difficulty_number, attribute, text_position);
	left = (difficulty_number & ~0xFFFFu) | 0x78u;
	top = 0x38u;
	right = 0xF8u;
	bottom = 0x40u;
	bar_back_color = (*p_color & ~0xFFu) | 0x17u;
	if ((mem_read32(video_control) & 0x200u) != 0u)
		flip_test_coordinates(&left, &top, &right, &bottom);
	fill_rect(top, right, bottom, &left, &bar_back_color, p_blocks_per_row);
	left = (left & ~0xFFFFu) | 0x78u;
	top = 0x38u;
	bottom = 0x40u;
	right = (uint16_t)(((setting_difficulty + 1u) << 5) + left);
	*p_color = (bar_back_color & ~0xFFu) | 0x49u;
	if ((mem_read32(video_control) & 0x200u) == 0u)
	{
		fill_rect(top, right, bottom, &left, p_color, p_blocks_per_row);
		return;
	}
	flip_test_coordinates(&left, &top, &right, &bottom);
	fill_rect(top, right, bottom, &left, p_color, p_blocks_per_row);
}

/* COIN ASSIGNMENTS: service moves the cursor, test acts on the item: chute type (COMMON/INDIVIDUAL;
   setting 1 again unless manual), credits to start (1 or 2), the next coin/credit setting
   (apply_coin_setting), MANUAL SETTING (test_coin_assignments_manual_page while coin_manual_page is set)
   or EXIT, saving the EEPROM if test_settings_changed. */
uint32_t test_coin_assignments(uint32_t color)
{
	uint8_t mode = 0;
	uint16_t attribute = 0;
	uint32_t credits_to_start = 0, cursor_positions = 0, ignored = 0, ignored_2 = 0, text = 0, text_position = 0, value = 0;

	if (test_screen_ready == 0u)
	{
		test_screen_ready++;
		test_settings_changed = 0u;
		test_cursor = 4u;
		test_coin_assignments_draw_main_page();
		return color;
	}
	if (coin_manual_page != 0u)
		return test_coin_assignments_manual_page(color);
	cursor_positions = coin_assignments_cursor_positions_ADDRESS;
	erase_test_cursor(cursor_positions);
	if ((0x1001u & inputs_pressed) != 0u)
	{
		test_cursor++;
		set_test_cursor();
	}
	blink_test_cursor();
	if ((0x2002u & inputs_pressed) == 0u)
		return color;
	switch (test_cursor)
	{
	case 0:
	case 5:
		test_settings_changed = 1u;
		setting_coin_chute_type++;
		setting_coin_chute_type &= 1u;
		text = coin_chute_type_individual_text_ADDRESS;
		if (setting_coin_chute_type == 0u)
			text = coin_chute_type_common_text_ADDRESS;
		print_text_list(&text, &ignored);
		if (setting_coin_number < 0)
		{
			text = coin_setting_erase_text_ADDRESS;
			print_text_list(&text, &ignored);
			text = coin_chute_2_label_erase_text_ADDRESS;
			print_text_list(&text, &ignored);
			text = coin_setting_number_manual_text_ADDRESS;
			print_text_list(&text, &ignored);
			if (setting_coin_chute_type == 0u)
			{
				text = coin_setting_text_manual_ADDRESS;
				print_text_list(&text, &ignored);
				text = coin_chute_labels_text_ADDRESS;
				print_text_list(&text, &ignored);
			}
			else
			{
				text = coin_setting_text_manual_individual_ADDRESS;
				print_text_list(&text, &ignored);
			}
			return color;
		}
		text = coin_setting_erase_text_ADDRESS;
		print_text_list(&text, &ignored);
		text = coin_chute_2_label_erase_text_ADDRESS;
		print_text_list(&text, &ignored);
		value = 1u;
		text_position = TEXT_CELL(31, 9);
		attribute = 0x1200u;
		draw_hex_2(value, attribute, text_position);
		if (setting_coin_chute_type == 0u)
		{
			text = coin_chute_labels_text_ADDRESS;
			print_text_list(&text, &ignored);
			text = coin_setting_text_1_ADDRESS;
			print_text_list(&text, &ignored);
			setting_coin_number = 0;
		}
		else
		{
			text = coin_setting_text_1_individual_ADDRESS;
			print_text_list(&text, &ignored);
			setting_coin_number_individual = 0;
		}
		apply_coin_setting();
		return color;
	case 1:
		test_settings_changed = 1u;
		setting_credits_to_start++;
		if (setting_credits_to_start == 3u)
			setting_credits_to_start = 1u;
		credits_to_start = setting_credits_to_start;
		text = credits_to_start_texts[credits_to_start >> 1];
		print_text_list(&text, &ignored);
		return color;
	case 2:
		setting_coin_number++;
		setting_coin_number_individual++;
		apply_coin_setting();
		return color;
	case 3:
		test_settings_changed = 1u;
		coin_manual_page++;
		test_screen_ready = 0u;
		mode = 1u;
		set_screen_mode(mode);
		mem_write16(video_control, mem_read16(video_control) | 0x8000u);
		return test_coin_assignments_manual_page(color);
	case 4:
		setting_cabinet_config = setting_cabinet_type | ((uint32_t)setting_coin_chute_type << 1);
		if (test_settings_changed != 0u)
			eeprom_save_and_lock(&color, &ignored_2);
		if (test_continue_mode != 0u)
		{
			test_screen++;
			test_screen_ready = 0u;
			return color;
		}
		test_cursor = test_menu_item;
		test_screen = 0u;
		test_screen_ready = 1u;
		draw_test_menu();
		return color;
	default:
		invalid_jump(0x9E0A9u + test_cursor * 3u);
	}
}

/* Draws the COIN ASSIGNMENTS main page (coin_manual_page 0): clamps setting_coins_per_credit,
   setting_bonus_adder and the chute multipliers to 1-9 (an adder of 1 then becomes 0), then prints the
   chute type, the coin/credit setting and its number (MANUAL SETTING when setting_coin_number is
   negative) and the credits to start. */
void test_coin_assignments_draw_main_page(void)
{
	uint16_t attribute = 0;
	uint32_t adder_setting = 0, credits_to_start = 0, ignored = 0, ignored_2 = 0, ignored_3 = 0, print_position = 0, setting = 0, setting_number = 0, text = 0;

	clear_text_layer(&ignored, &ignored_2, &ignored_3);
	coin_manual_page = 0u;
	test_cursor_positions = coin_assignments_cursor_positions_ADDRESS;
	if (setting_coin_number < 0)
		setting_coin_number_individual = 0;
	setting = setting_coins_per_credit_ADDRESS;
	clamp_coin_setting(setting);
	adder_setting = setting_bonus_adder_ADDRESS;
	clamp_coin_setting(adder_setting);
	if (setting_bonus_adder == 1u)
		setting_bonus_adder = 0u;
	setting = setting_chute_1_multiplier_ADDRESS;
	clamp_coin_setting(setting);
	setting = setting_chute_2_multiplier_ADDRESS;
	clamp_coin_setting(setting);
	if (setting_coin_chute_type == 0u)
	{
		text = coin_chute_type_common_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		text = coin_assignments_common_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		if (setting_coin_number >= 0)
		{
			text = coin_setting_texts[(uint8_t)setting_coin_number];
			print_text_list(&text, &ignored_3);
			setting_number = (uint8_t)setting_coin_number;
			setting_number = (uint8_t)(setting_number + 1u);
			convert_to_bcd(&setting_number, &ignored_2);
			print_position = TEXT_CELL(31, 9);
			attribute = 0x1200u;
			draw_hex_2(setting_number, attribute, print_position);
		}
	}
	else
	{
		text = coin_chute_type_individual_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		text = coin_setting_texts_individual[(uint8_t)setting_coin_number_individual];
		print_text_list(&text, &ignored_3);
		text = coin_assignments_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		setting_number = individual_coin_setting_numbers[(uint8_t)setting_coin_number_individual];
		convert_to_bcd(&setting_number, &ignored_2);
		print_position = TEXT_CELL(31, 9);
		attribute = 0x1200u;
		draw_hex_2(setting_number, attribute, print_position);
	}
	credits_to_start = setting_credits_to_start;
	text = credits_to_start_texts[credits_to_start >> 1];
	print_text_list(&text, &ignored_3);
	if (setting_coin_number >= 0)
		return;
	setting_coin_number_individual = -1;
	text = coin_setting_erase_text_ADDRESS;
	print_text_list(&text, &ignored_3);
	text = coin_setting_number_manual_text_ADDRESS;
	print_text_list(&text, &ignored_3);
	text = coin_setting_text_manual_ADDRESS;
	if (setting_coin_chute_type != 0u)
		text = coin_setting_text_manual_individual_ADDRESS;
	print_text_list(&text, &ignored_3);
}

/* Unused: a second return right after the end of test_coin_assignments' credits-to-start item; nothing
   jumps to it. */
void test_coin_assignments_spare_return(void)
{
}

/* Applies the chosen coin/credit setting: setting_coin_number wraps past 0x19 and
   setting_coin_number_individual past 0xE to 0; prints its text and number (common or individual chutes)
   and copies its 4 bytes of coin_setting_values into setting_coins_per_credit, setting_bonus_adder and
   the two chute multipliers. test_settings_changed = 1. */
void apply_coin_setting(void)
{
	uint32_t coin_setting = 0, ignored = 0, setting_number = 0, text = 0, values_record = 0;

	test_settings_changed = 1u;
	text = coin_setting_number_erase_text_ADDRESS;
	print_text_list(&text, &ignored);
	text = coin_setting_erase_text_ADDRESS;
	print_text_list(&text, &ignored);
	if (setting_coin_number > 0x19)
		setting_coin_number = 0;
	if (setting_coin_number_individual > 0xE)
		setting_coin_number_individual = 0;
	if (setting_coin_chute_type != 0u)
	{
		text = coin_setting_texts_individual[(uint8_t)setting_coin_number_individual];
		print_text_list(&text, &ignored);
		setting_number = individual_coin_setting_numbers[(uint8_t)setting_coin_number_individual];
		draw_coin_setting_number(setting_number);
		coin_setting = individual_coin_setting_numbers[(uint8_t)setting_coin_number_individual];
	}
	else
	{
		text = coin_setting_texts[(uint8_t)setting_coin_number];
		print_text_list(&text, &ignored);
		coin_setting = (uint8_t)setting_coin_number;
		coin_setting = (uint8_t)(coin_setting + 1u);
		draw_coin_setting_number(coin_setting);
		coin_setting = (uint8_t)setting_coin_number;
		coin_setting = (uint8_t)(coin_setting + 1u);
	}
	values_record = coin_setting_values_ADDRESS + (uint8_t)(coin_setting - 1u) * 4u;
	setting_coins_per_credit = COIN_SETTING_VALUE(values_record)->coins_per_credit;
	values_record++;
	setting_bonus_adder = COIN_SETTING_VALUE(values_record - 0x1u)->bonus_coins;
	values_record++;
	setting_chute_1_multiplier = COIN_SETTING_VALUE(values_record - 0x2u)->chute_1_coin_value;
	setting_chute_2_multiplier = COIN_SETTING_VALUE(values_record - 0x2u)->chute_2_coin_value;
}

/* Prints setting_number in decimal (convert_to_bcd, draw_hex_2) at text cell 31,9 with attribute 0x1200. */
void draw_coin_setting_number(uint32_t setting_number)
{
	uint16_t attribute = 0;
	uint32_t ignored = 0, text_position = 0;

	convert_to_bcd(&setting_number, &ignored);
	text_position = TEXT_CELL(31, 9);
	attribute = 0x1200u;
	draw_hex_2(setting_number, attribute, text_position);
}

/* The MANUAL SETTING page, printed on its first frame: coins per credit (1-9), bonus adder (0 or 2-9)
   and the chute multipliers (1-9, chute 2 only with common chutes), each changed by test
   (setting_coin_number -1: manual) with the credit table redrawn; EXIT goes back to the main page. */
uint32_t test_coin_assignments_manual_page(uint32_t color)
{
	uint8_t mode = 0;
	uint16_t attribute = 0;
	uint32_t coins_left = 0, cursor_positions = 0, ignored = 0, ignored_2 = 0, ignored_3 = 0, manual_print_position = 0, text = 0, text_position = 0, value = 0;

	if (test_screen_ready == 0u)
	{
		test_screen_ready++;
		clear_text_layer(&ignored, &ignored_2, &manual_print_position);
		test_cursor_positions = coin_manual_cursor_positions_ADDRESS;
		test_cursor = 4u;
		text = coin_manual_page_common_text_ADDRESS;
		if (setting_coin_chute_type != 0u)
			text = coin_manual_page_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		text = coins_per_credit_texts[setting_coins_per_credit];
		print_text_list(&text, &ignored_3);
		text = bonus_adder_texts[setting_bonus_adder];
		print_text_list(&text, &ignored_3);
		value = setting_chute_1_multiplier;
		text_position = TEXT_CELL(30, 12);
		attribute = 0x1200u;
		draw_hex_2(value, attribute, text_position);
		text = chute_1_coin_text_ADDRESS;
		if (setting_chute_1_multiplier != 1u)
			text = chute_1_coins_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		coins_left = draw_coin_credit_table();
		if (setting_coin_chute_type != 0u)
			return coins_left;
		value = setting_chute_2_multiplier;
		text_position = TEXT_CELL(30, 18);
		attribute = 0x1200u;
		draw_hex_2(value, attribute, text_position);
		text = chute_2_coin_text_ADDRESS;
		if (setting_chute_2_multiplier != 1u)
			text = chute_2_coins_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		return coins_left;
	}
	cursor_positions = coin_manual_cursor_positions_ADDRESS;
	erase_test_cursor(cursor_positions);
	if ((0x1001u & inputs_pressed) != 0u)
	{
		test_cursor++;
		if (setting_coin_chute_type != 0u && test_cursor == 3u)
			test_cursor++;
		set_test_cursor();
	}
	blink_test_cursor();
	if ((0x2002u & inputs_pressed) == 0u)
		return color;
	switch (test_cursor)
	{
	case 0:
	case 5:
		setting_coin_number = -1;
		setting_coins_per_credit++;
		if ((int8_t)setting_coins_per_credit > 9)
			setting_coins_per_credit = 1u;
		text = coins_per_credit_texts[setting_coins_per_credit];
		print_text_list(&text, &ignored_3);
		text = coin_credit_rows_erase_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		coins_left = draw_coin_credit_table();
		return coins_left;
	case 1:
		setting_coin_number = -1;
		setting_bonus_adder++;
		if ((int8_t)setting_bonus_adder > 9)
			setting_bonus_adder = 0u;
		if (setting_bonus_adder == 1u)
			setting_bonus_adder = 2u;
		text = bonus_adder_texts[setting_bonus_adder];
		print_text_list(&text, &ignored_3);
		text = coin_credit_rows_erase_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		coins_left = draw_coin_credit_table();
		return coins_left;
	case 2:
		setting_coin_number = -1;
		setting_chute_1_multiplier++;
		if ((int8_t)setting_chute_1_multiplier > 9)
			setting_chute_1_multiplier = 1u;
		value = setting_chute_1_multiplier;
		text_position = TEXT_CELL(30, 12);
		attribute = 0x1200u;
		draw_hex_2(value, attribute, text_position);
		text = chute_1_coin_text_ADDRESS;
		if (setting_chute_1_multiplier != 1u)
			text = chute_1_coins_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		text = coin_credit_rows_erase_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		coins_left = draw_coin_credit_table();
		return coins_left;
	case 3:
		setting_coin_number = -1;
		setting_chute_2_multiplier++;
		if ((int8_t)setting_chute_2_multiplier > 9)
			setting_chute_2_multiplier = 1u;
		value = setting_chute_2_multiplier;
		text_position = TEXT_CELL(30, 18);
		attribute = 0x1200u;
		draw_hex_2(value, attribute, text_position);
		text = chute_2_coin_text_ADDRESS;
		if (setting_chute_2_multiplier != 1u)
			text = chute_2_coins_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		text = coin_credit_rows_erase_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		coins_left = draw_coin_credit_table();
		return coins_left;
	case 4:
		test_cursor = 3u;
		mode = 0u;
		set_screen_mode(mode);
		test_coin_assignments_draw_main_page();
		return color;
	default:
		invalid_jump(0x9E479u + test_cursor * 3u);
	}
}

/* Unused: a second return right after the end of the manual page's coins-per-credit item
   (test_coin_assignments_manual_page); nothing jumps to it. */
void test_coin_assignments_spare_return_2(void)
{
}

/* Keeps the setting byte at setting within 1-9. */
void clamp_coin_setting(uint32_t setting)
{
	if ((int8_t)mem_read8(setting) <= 0)
		mem_write8(setting, 1u);
	if ((int8_t)mem_read8(setting) > 9)
		mem_write8(setting, 9u);
}

/* The manual page's "1 COIN COUNTS AS" table: draw_coin_credit_cell for 9 down to 1 coins in chute 1
   and, with common chutes (setting_coin_chute_type 0), in chute 2. */
uint32_t draw_coin_credit_table(void)
{
	uint16_t multiplier = 0;
	uint32_t cell_position = 0, coins = 0;

	coins = 9u;
	cell_position = chute_1_credit_cell_positions_ADDRESS;
	do
	{
		multiplier = setting_chute_1_multiplier;
		cell_position = draw_coin_credit_cell(multiplier, (uint16_t)coins, cell_position);
		coins--;
	} while (coins != 0u);
	if (setting_coin_chute_type != 0u)
		return coins;
	coins = 9u;
	cell_position = chute_2_credit_cell_positions_ADDRESS;
	do
	{
		multiplier = setting_chute_2_multiplier;
		cell_position = draw_coin_credit_cell(multiplier, (uint16_t)coins, cell_position);
		coins--;
	} while (coins != 0u);
	return coins;
}

/* Prints the credits for coins (the count) in a chute with multiplier, by setting_coins_per_credit and
   setting_bonus_adder: the whole credits, 9 at most, and any part as a fraction over
   setting_coins_per_credit (draw_fraction), at the cell cell_position names; hands back the next cell. */
uint32_t draw_coin_credit_cell(uint16_t multiplier, uint16_t coins, uint32_t cell_position)
{
	uint8_t attribute = 0;
	uint16_t attribute_2 = 0;
	uint32_t adder_setting = 0, coin_count = 0, coins_per_credit = 0, column = 0, fraction_coins = 0, ignored = 0, text_position = 0, value = 0, whole_credits = 0;

	coin_count = coins;
	coins_per_credit = setting_coins_per_credit;
	adder_setting = setting_bonus_adder;
	if (adder_setting == 0u)
		coin_count = divide_s32((coin_count * multiplier) << 16, coins_per_credit);
	else
	{
		multiplier = (uint16_t)(multiplier * coin_count);
		coin_count = divide_s32((multiplier + multiplier * adder_setting) << 16, coins_per_credit * adder_setting);
		coins_per_credit = setting_coins_per_credit;
	}
	whole_credits = coin_count >> 16;
	coin_count &= 0xFFFFu;
	fraction_coins = divide_s32(0x10000u, coins_per_credit) & 0xFFFFu;
	if (fraction_coins == 0u || coin_count == 0u)
		fraction_coins = 0u;
	else
		fraction_coins = divide_s32(coin_count, fraction_coins);
	if ((int32_t)whole_credits >= 9)
	{
		whole_credits = 9u;
		fraction_coins = 0u;
	}
	column = TEXT_CELL_POSITION(cell_position)->column;
	cell_position++;
	text_position = TEXT_CELL(0, 0) + (column << 1) + ((uint32_t)TEXT_CELL_POSITION(cell_position - 0x1u)->row << 7);
	attribute = 2u;
	cell_position++;
	if (fraction_coins != 0u)
	{
		draw_fraction(attribute, &fraction_coins, &coins_per_credit, &text_position);
		text_position -= 4u;
	}
	value = whole_credits;
	text_position -= 2u;
	convert_to_bcd(&value, &ignored);
	attribute_2 = 0x200u;
	draw_hex_2(value, attribute_2, text_position);
	return cell_position;
}

/* draw_hex_4 of value at text_position with attribute 0x200. */
void draw_test_hex_4(uint32_t value, uint32_t text_position)
{
	uint32_t attribute = 0;

	attribute = 0x200u;
	draw_hex_4(&value, &attribute, &text_position);
}

/* draw_hex_2 of value at text_position with attribute 0x200. */
void draw_test_hex_2(uint32_t value, uint32_t text_position)
{
	uint16_t attribute = 0;

	attribute = 0x200u;
	draw_hex_2(value, attribute, text_position);
}

/* BOOKKEEPING, printed once from bookkeeping_coin_credits: the coins per chute (chute 3 only for
   individual chutes on a 3-player cabinet) and their total, coin, service and total credits, games, then
   the total, play, average, longest and shortest times (split_frame_count). Test leaves. */
void test_bookkeeping(void)
{
	uint32_t average_hours = 0, average_minutes = 0, average_seconds = 0, chute_3_coins = 0, days = 0, frames = 0, frames_high = 0, hours = 0, ignored = 0, ignored_2 = 0, ignored_3 = 0, ignored_4 = 0, minutes = 0, print_position = 0, seconds = 0, shortest_game_frames = 0, shortest_hours = 0, shortest_minutes = 0, shortest_seconds = 0, text = 0, text_position = 0, time_high = 0, total_coins = 0, value = 0;

	if (test_screen_ready == 0u)
	{
		test_screen_ready++;
		clear_text_layer(&ignored, &ignored_2, &print_position);
		text = bookkeeping_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		test_cursor_positions = bookkeeping_cursor_positions_ADDRESS;
		test_cursor = 0u;
		current_object = bookkeeping_coin_credits;
		if (setting_coin_chute_type == 0u)
			text = bookkeeping_chute_3_erase_text_ADDRESS;
		if (setting_cabinet_type == 0u)
			text = bookkeeping_chute_3_erase_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		value = mem_read16(current_object + 4u);
		convert_to_bcd(&value, &ignored_2);
		text_position = TEXT_CELL(26, 3);
		draw_test_hex_4(value, text_position);
		value = mem_read16(current_object + 6u);
		convert_to_bcd(&value, &ignored_2);
		text_position = TEXT_CELL(26, 4);
		draw_test_hex_4(value, text_position);
		if (setting_coin_chute_type != 0u && setting_cabinet_type != 0u)
		{
			value = mem_read16(current_object + 8u);
			convert_to_bcd(&value, &ignored_2);
			text_position = TEXT_CELL(26, 5);
			draw_test_hex_4(value, text_position);
		}
		chute_3_coins = mem_read16(current_object + 8u);
		total_coins = (uint32_t)mem_read16(current_object + 4u) + mem_read16(current_object + 6u);
		if (setting_coin_chute_type != 0u && setting_cabinet_type != 0u)
			total_coins += chute_3_coins;
		convert_to_bcd(&total_coins, &ignored_2);
		text_position = TEXT_CELL(26, 7);
		draw_test_hex_4(total_coins, text_position);
		value = mem_read16(current_object);
		convert_to_bcd(&value, &ignored_2);
		text_position = TEXT_CELL(26, 9);
		draw_test_hex_4(value, text_position);
		value = mem_read16(current_object + 2u);
		convert_to_bcd(&value, &ignored_2);
		text_position = TEXT_CELL(26, 10);
		draw_test_hex_4(value, text_position);
		value = (uint32_t)mem_read16(current_object) + mem_read16(current_object + 2u);
		convert_to_bcd(&value, &ignored_2);
		text_position = TEXT_CELL(26, 11);
		draw_test_hex_4(value, text_position);
		value = mem_read16(current_object + 0xAu);
		convert_to_bcd(&value, &ignored_2);
		text_position = TEXT_CELL(26, 13);
		draw_test_hex_4(value, text_position);
		frames = mem_read32(current_object + 0x10u);
		frames_high = mem_read16(current_object + 0x14u);
		split_frame_count(frames, frames_high, &seconds, &minutes, &hours, &days);
		value = days;
		convert_to_bcd(&value, &ignored_2);
		text_position = TEXT_CELL(21, 15);
		draw_test_hex_4(value, text_position);
		value = hours;
		convert_to_bcd(&value, &ignored_2);
		text_position = TEXT_CELL(26, 15);
		draw_test_hex_2(value, text_position);
		value = minutes;
		convert_to_bcd(&value, &ignored_2);
		text_position = TEXT_CELL(29, 15);
		draw_test_hex_2(value, text_position);
		value = seconds;
		convert_to_bcd(&value, &ignored_2);
		text_position = TEXT_CELL(32, 15);
		draw_test_hex_2(value, text_position);
		frames = mem_read32(current_object + 0x16u);
		frames_high = mem_read16(current_object + 0x1Au);
		split_frame_count(frames, frames_high, &seconds, &minutes, &hours, &days);
		value = days;
		convert_to_bcd(&value, &ignored_2);
		text_position = TEXT_CELL(21, 16);
		draw_test_hex_4(value, text_position);
		value = hours;
		convert_to_bcd(&value, &ignored_2);
		text_position = TEXT_CELL(26, 16);
		draw_test_hex_2(value, text_position);
		value = minutes;
		convert_to_bcd(&value, &ignored_2);
		text_position = TEXT_CELL(29, 16);
		draw_test_hex_2(value, text_position);
		value = seconds;
		convert_to_bcd(&value, &ignored_2);
		text_position = TEXT_CELL(32, 16);
		draw_test_hex_2(value, text_position);
		average_seconds = 0u;
		average_minutes = 0u;
		average_hours = 0u;
		if (mem_read16(current_object + 0xAu) != 0u)
		{
			frames = mem_read32(current_object + 0x1Cu);
			time_high = mem_read16(current_object + 0x20u);
			divx_pair(mem_read16(current_object + 0xAu), &frames, &time_high);
			frames_high = 0u;
			split_frame_count(frames, frames_high, &average_seconds, &average_minutes, &average_hours, &ignored_4);
		}
		value = average_hours;
		convert_to_bcd(&value, &ignored_2);
		text_position = TEXT_CELL(26, 17);
		draw_test_hex_2(value, text_position);
		value = average_minutes;
		convert_to_bcd(&value, &ignored_2);
		text_position = TEXT_CELL(29, 17);
		draw_test_hex_2(value, text_position);
		value = average_seconds;
		convert_to_bcd(&value, &ignored_2);
		text_position = TEXT_CELL(32, 17);
		draw_test_hex_2(value, text_position);
		frames = mem_read32(current_object + 0xCu);
		frames_high = 0u;
		split_frame_count(frames, frames_high, &seconds, &minutes, &hours, &ignored_4);
		value = hours;
		convert_to_bcd(&value, &ignored_2);
		text_position = TEXT_CELL(26, 18);
		draw_test_hex_2(value, text_position);
		value = minutes;
		convert_to_bcd(&value, &ignored_2);
		text_position = TEXT_CELL(29, 18);
		draw_test_hex_2(value, text_position);
		value = seconds;
		convert_to_bcd(&value, &ignored_2);
		text_position = TEXT_CELL(32, 18);
		draw_test_hex_2(value, text_position);
		shortest_seconds = 0u;
		shortest_minutes = 0u;
		shortest_hours = 0u;
		frames_high = 0u;
		shortest_game_frames = mem_read32(current_object + 0x22u);
		if (((int8_t)0xFFu & shortest_game_frames) + 1u != 0u)
			split_frame_count(shortest_game_frames, frames_high, &shortest_seconds, &shortest_minutes, &shortest_hours, &ignored_4);
		value = shortest_hours;
		convert_to_bcd(&value, &ignored_2);
		text_position = TEXT_CELL(26, 19);
		draw_test_hex_2(value, text_position);
		value = shortest_minutes;
		convert_to_bcd(&value, &ignored_2);
		text_position = TEXT_CELL(29, 19);
		draw_test_hex_2(value, text_position);
		value = shortest_seconds;
		convert_to_bcd(&value, &ignored_2);
		text_position = TEXT_CELL(32, 19);
		draw_test_hex_2(value, text_position);
	}
	if ((0x2002u & inputs_pressed) == 0u)
		return;
	if (test_continue_mode != 0u)
	{
		test_screen++;
		test_screen_ready = 0u;
		return;
	}
	test_cursor = test_menu_item;
	test_screen = 0u;
	test_screen_ready = 1u;
	draw_test_menu();
}

/* BACKUP DATA CLEAR: service moves the cursor between YES and NO; test on YES prints COMPLETED, clears
   the bookkeeping and the ranking (reset_bookkeeping, reset_ranking) and leaves 0x78 frames later, on NO
   at once. In continue mode it goes on to OUTPUT TEST only with moving_seat_dip, else to the menu on
   EXIT. */
uint32_t test_backup_clear(uint32_t polynomial)
{
	uint32_t crc_polynomial = 0, cursor_positions = 0, ignored = 0, ignored_2 = 0, ignored_3 = 0, text = 0;

	if (test_screen_ready == 0u)
	{
		test_screen_ready++;
		clear_text_layer(&ignored, &ignored_2, &ignored_3);
		text = backup_clear_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		test_cursor_positions = backup_clear_cursor_positions_ADDRESS;
		test_cursor = 1u;
		backup_clear_timer = 0u;
	}
	if (backup_clear_timer != 0u)
	{
		backup_clear_timer++;
		if ((int8_t)backup_clear_timer < 0x78)
			return polynomial;
		if (test_continue_mode == 0u)
		{
			test_cursor = test_menu_item;
			test_screen = 0u;
			test_screen_ready = 1u;
			draw_test_menu();
			return polynomial;
		}
		if (mem_read8(moving_seat_dip) != 0u)
		{
			test_screen++;
			test_screen_ready = 0u;
			return polynomial;
		}
		test_cursor = 0xAu;
		test_screen = 0u;
		test_screen_ready = 1u;
		draw_test_menu();
		return polynomial;
	}
	cursor_positions = backup_clear_cursor_positions_ADDRESS;
	erase_test_cursor(cursor_positions);
	if ((0x1001u & inputs_pressed) != 0u)
	{
		test_cursor++;
		set_test_cursor();
	}
	blink_test_cursor();
	if ((0x2002u & inputs_pressed) == 0u)
		return polynomial;
	switch (test_cursor)
	{
	case 0:
	case 2:
		text = backup_clear_completed_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		reset_bookkeeping();
		crc_polynomial = reset_ranking();
		text = backup_clear_done_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		backup_clear_timer++;
		return crc_polynomial;
	case 1:
		if (test_continue_mode == 0u)
		{
			test_cursor = test_menu_item;
			test_screen = 0u;
			test_screen_ready = 1u;
			draw_test_menu();
			return polynomial;
		}
		if (mem_read8(moving_seat_dip) != 0u)
		{
			test_screen++;
			test_screen_ready = 0u;
			return polynomial;
		}
		test_cursor = 0xAu;
		test_screen = 0u;
		test_screen_ready = 1u;
		draw_test_menu();
		return polynomial;
	default:
		invalid_jump(0x9EABDu + test_cursor * 3u);
	}
}

/* OUTPUT TEST: prints whether the moving seat is in use; every frame port D bits 2-3 are cleared and,
   with setting_moving_seat, held 1P and 2P attack (input_ports_active 0x10000, 0x1000000) set them (0x4,
   0x8) with ON/OFF texts. Test goes back to the menu (in continue mode on EXIT, with CONTINUE MODE
   printed). */
void test_outputs(void)
{
	uint32_t ignored = 0, ignored_2 = 0, ignored_3 = 0, setup_print_position = 0, text = 0;
	uint8_t port_d_bits = 0, port_d_bits_1p = 0, port_d_bits_2p = 0;

	if (test_screen_ready == 0u)
	{
		test_screen_ready++;
		clear_text_layer(&ignored, &ignored_2, &setup_print_position);
		text = output_test_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		if (setting_moving_seat != 0u)
		{
			text = output_test_seat_text_ADDRESS;
			print_text_list(&text, &ignored_3);
			return;
		}
		text = output_test_no_use_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		return;
	}
	port_d_bits = mem_read8(io_port_d);
	mem_write8(io_port_d, port_d_bits & ~0xCu);
	if ((0x2002u & inputs_pressed) != 0u)
	{
		if (test_continue_mode == 0u)
		{
			test_cursor = test_menu_item;
			test_screen = 0u;
			test_screen_ready = 1u;
			draw_test_menu();
			return;
		}
		test_cursor = 0xAu;
		test_screen = 0u;
		test_screen_ready = 1u;
		draw_test_menu();
		text = continue_mode_text_ADDRESS;
		print_text_list(&text, &ignored_3);
		return;
	}
	if (setting_moving_seat == 0u)
		return;
	text = output_test_1p_off_text_ADDRESS;
	if ((input_ports_active & 0x10000u) != 0u)
	{
		port_d_bits_1p = mem_read8(io_port_d);
		mem_write8(io_port_d, port_d_bits_1p | 4u);
		text = output_test_1p_on_text_ADDRESS;
	}
	print_text_list(&text, &ignored_3);
	text = output_test_2p_off_text_ADDRESS;
	if ((input_ports_active & 0x1000000u) != 0u)
	{
		port_d_bits_2p = mem_read8(io_port_d);
		mem_write8(io_port_d, port_d_bits_2p | 8u);
		text = output_test_2p_on_text_ADDRESS;
	}
	print_text_list(&text, &ignored_3);
}

/* EXIT: clears the three players' credits and bonus coins and test_mode, leaving the test mode; when
   eeprom_write_count is 10000 (0x2710) or more it first shows the EEPROM caution and waits for test. */
void exit_test_mode(void)
{
	uint32_t ignored = 0, ignored_2 = 0, ignored_3 = 0, print_position = 0, text = 0;

	if (test_screen_ready != 0u)
	{
		if ((0x2002u & inputs_pressed) == 0u)
			return;
		p1_bonus_coins = 0u;
		p1_credits = 0u;
		p2_bonus_coins = 0u;
		p2_credits = 0u;
		p3_bonus_coins = 0u;
		p3_credits = 0u;
		test_mode = 0u;
		return;
	}
	if ((int16_t)(uint16_t)eeprom_write_count < 0x2710)
	{
		p1_bonus_coins = 0u;
		p1_credits = 0u;
		p2_bonus_coins = 0u;
		p2_credits = 0u;
		p3_bonus_coins = 0u;
		p3_credits = 0u;
		test_mode = 0u;
		return;
	}
	clear_text_layer(&ignored, &ignored_2, &print_position);
	text = eeprom_caution_text_ADDRESS;
	print_text_list(&text, &ignored_3);
	test_screen_ready++;
	if ((0x2002u & inputs_pressed) == 0u)
		return;
	p1_bonus_coins = 0u;
	p1_credits = 0u;
	p2_bonus_coins = 0u;
	p2_credits = 0u;
	p3_bonus_coins = 0u;
	p3_credits = 0u;
	test_mode = 0u;
}

/* Mirrors the rectangle left, top, right, bottom about the screen's centre (0xA0, 0x70), for drawing on
   a flipped screen; the callers test the flip (video_control bit 9). */
void flip_test_coordinates(uint32_t *p_left, uint32_t *p_top, uint32_t *p_right, uint32_t *p_bottom)
{
	uint16_t left = (uint16_t)*p_left;
	uint16_t top = (uint16_t)*p_top;
	uint16_t right = (uint16_t)*p_right;
	uint16_t bottom = (uint16_t)*p_bottom;

	left = (uint16_t)(0u - (left - 0xA0u) + 0xA0u);
	right = (uint16_t)(0u - (right - 0xA0u) + 0xA0u);
	top = (uint16_t)(0u - (top - 0x70u) + 0x70u);
	bottom = (uint16_t)(0u - (bottom - 0x70u) + 0x70u);
	*p_left = (*p_left & 0xFFFF0000u) | left;
	*p_top = (*p_top & 0xFFFF0000u) | top;
	*p_right = (*p_right & 0xFFFF0000u) | right;
	*p_bottom = (*p_bottom & 0xFFFF0000u) | bottom;
}

/* Draws the blinking '>' (test_cursor_arrow) at test_cursor_column/test_cursor_row
   (draw_blinking_test_cursor). */
void blink_test_cursor(void)
{
	uint32_t cursor_char = 0;

	cursor_char = test_cursor_arrow_ADDRESS;
	draw_blinking_test_cursor(cursor_char);
}

/* Writes cursor_char's character with attribute 0x200 at test_cursor_column/test_cursor_row, or a blank
   (test_cursor_space) while test_cursor_blink, counted down at each call, is below 1; the count goes
   back to 0x1E at -0xF. */
void draw_blinking_test_cursor(uint32_t cursor_char)
{
	uint8_t blink_before = 0;

	blink_before = test_cursor_blink;
	test_cursor_blink = blink_before - 1u;
	if ((int8_t)blink_before < 1)
		cursor_char = test_cursor_space_ADDRESS;
	if (test_cursor_blink <= -0xF)
		test_cursor_blink = 0x1E;
	mem_write16(TEXT_CELL(0, 0) + (((uint32_t)test_cursor_row << 7) + ((uint32_t)test_cursor_column << 1)), 0x200u | TEST_CURSOR_CHARACTERS(cursor_char)[0]);
}

/* Writes a blank at test_cursor's place in cursor_positions (test_cursor_column/test_cursor_row loaded
   from it), through draw_blinking_test_cursor, which also counts test_cursor_blink down. */
void erase_test_cursor(uint32_t cursor_positions)
{
	uint32_t cursor_char = 0;

	cursor_positions += (uint32_t)test_cursor << 1;
	test_cursor_column = TEXT_CELL_POSITION(cursor_positions)->column;
	test_cursor_row = TEXT_CELL_POSITION(cursor_positions)->row;
	cursor_char = test_cursor_space_ADDRESS;
	draw_blinking_test_cursor(cursor_char);
}

/* Loads test_cursor's column and row from test_cursor_positions into test_cursor_column/test_cursor_row,
   test_cursor going back to 0 at the list's 0xFF end. */
void set_test_cursor(void)
{
	uint32_t cursor_entry = 0;

	for (;;)
	{
		cursor_entry = test_cursor_positions + ((uint32_t)test_cursor << 1);
		if ((int8_t)TEXT_CELL_POSITION(cursor_entry)->column >= 0)
			break;
		test_cursor = 0u;
	}
	test_cursor_column = TEXT_CELL_POSITION(cursor_entry)->column;
	test_cursor_row = TEXT_CELL_POSITION(cursor_entry)->row;
}

/* Reads the moving seat DIP switch (io_port_f bit 0, on when low); when it differs from moving_seat_dip
   it is stored there and in setting_moving_seat, an on switch also sets setting_cabinet_type 0
   (setting_cabinet_config too), and the EEPROM is saved (eeprom_save_and_lock). */
uint32_t check_moving_seat_dip(uint32_t words_left)
{
	uint8_t seat_dip_on = 0;
	uint32_t dip_flag_address = 0, ignored = 0;
	uint8_t port_f_bits = 0;

	port_f_bits = mem_read8(io_port_f);
	dip_flag_address = moving_seat_dip;
	seat_dip_on = (uint8_t)((port_f_bits & 1u) ^ 1u);
	if ((mem_read8(moving_seat_dip) & 1u) == seat_dip_on)
		return words_left;
	setting_moving_seat = seat_dip_on;
	mem_write8(dip_flag_address, seat_dip_on);
	if (seat_dip_on != 0u)
	{
		setting_cabinet_type = 0u;
		setting_cabinet_config = setting_cabinet_type | ((uint32_t)setting_coin_chute_type << 1);
	}
	eeprom_save_and_lock(&ignored, &words_left);
	return words_left;
}

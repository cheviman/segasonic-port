/**
 *    Name:        main.c
 *    Description: the sound program's frame: the reset, the timer interrupt (the players' tick), the main loop and sound_init; the two YM3438 and the RF5C68 set up and written; the tracks' and MIDI parts' start values; read_table_word, multiply_bytes, copy_sample_page
 *    Version:     0.1.0
 *    Created:     13-10-2026
 *    Modified:    21-10-2026
 *    Author:      Sebastian Toyos
 *
 *    SegaSonic The Hedgehog (arcade, Sega System 32): the Z80 sound program decompiled into C by
 *    tools/decompile_sound.py, with the names of sound_symbols.txt.
 */
#include "sound.h"

/* The sound CPU's start: interrupts off, the shared RAM 0xE000-0xFEFF cleared, the interrupt inputs
   mapped (ports 0xD0-0xD2), unmasked and their pending ones cleared, sound_init, then interrupts on. */
void sound_reset(void)
{
	uint8_t unused_c = 0;

	z80_block(14);
	interrupts_off();
	z80_block(201);
	midi_in_message_length = 0u;
	move_block(midi_in_message_length_ADDRESS, midi_in_bytes_left_ADDRESS, 0x1EFFu, 1u);
	unused_c = 0u;
	port_out(IRQ_VECTOR0_INPUT, 0u);
	port_out(IRQ_VECTOR1_INPUT, 1u);
	port_out(IRQ_VECTOR2_INPUT, 2u);
	port_out(IRQ_CONTROL, 0u);
	port_out(IRQ_ACKNOWLEDGE, 0u);
	port_out(IRQ_CONTROL_D8, 0xBu);
	port_out(IRQ_MASK, 0u);
	sound_init(unused_c);
	z80_block(4);
	interrupts_on_after_block();
}

/* The timer interrupt (the first YM3438's timer A): acknowledges every pending input, rearms the timer
   (register 0x27 = 0x15) and runs sequence_tick, the sequence players' tick. */
void sound_irq(void)
{
	z80_block(121);
	interrupts_off();
	port_out(IRQ_ACKNOWLEDGE, 0u);
	port_out(FM1_ADDRESS, 0x27u);
	port_out(FM1_DATA, 0x15u);
	sequence_tick();
	z80_block(54);
	interrupts_on();
}

/* One pass of the sound program's main loop: waits while sound_cpu_hold is set; when the second YM3438's
   timer A has run out, rearms it and refills a sample voice (refill_pcm_voice); step_fm_vibrato on
   vibrato_voice when its vibrato_depth is set, vibrato_voice moving on to the next of the 12 FM voices;
   poll_sound_command, handle_midi_message. */
void sound_main_loop(void)
{
	uint16_t fm_track = 0, next_fm_track = 0;
	uint16_t saved_fm_track = 0;

	do
	{
		z80_block(31);
		interrupts_off();
	} while (sound_cpu_hold != 0u);
	z80_block(29);
	interrupts_on();
	if (port_in(FM2_ADDRESS) & 1u)
	{
		z80_block(61);
		port_out(FM2_ADDRESS, 0x27u);
		port_out(FM2_DATA, 0x15u);
		refill_pcm_voice();
	}
	z80_block(53);
	fm_track = vibrato_voice;
	if (FM_VOICE(fm_track)->vibrato_depth != 0u)
	{
		z80_cycles(7);
		step_fm_vibrato(fm_track);
	}
	z80_block(87);
	saved_fm_track = fm_track;
	next_fm_track = saved_fm_track + 0x12u;
	vibrato_voice = next_fm_track;
	if (0x68u == (uint8_t)next_fm_track)
	{
		z80_block(26);
		vibrato_voice = FM_VOICE_ADDRESS(0);
	}
	z80_block(17);
	poll_sound_command();
	z80_block(17);
	handle_midi_message();
	z80_block(10);
}

/* Sets up the sound program: init_pcm_chip, init_fm_chips, the MIDI queue (midi_queue_read 0xFF,
   midi_queue_write 0, midi_status 0x80), tick_countdown 1, music_volume 0xFF, the voice cursors at their
   first voices, init_track_channels, init_pcm_tracks and init_midi_parts; then timer A started on both
   YM3438 (register 0x27 = 0x15). */
void sound_init(uint8_t unused_c)
{
	uint8_t fm_register = 0, leftover_c = 0, timer_control = 0;
	uint16_t leftover_bc = 0;

	z80_block(17);
	init_pcm_chip(unused_c);
	z80_block(17);
	leftover_bc = init_fm_chips();
	leftover_c = (uint8_t)leftover_bc;
	z80_block(278);
	midi_queue_write = 0u;
	midi_queue_read = 0xFFu;
	tick_countdown = 1u;
	unread_e010 = 0x8Cu;
	next_fm_voice = FM_VOICE_ADDRESS(0);
	fm_voices_to_end = 0xCu;
	music_volume = 0xFFu;
	midi_status = 0x80u;
	midi_in_message_length = 2u;
	refill_voices_left = 8u;
	refill_voice = SAMPLE_VOICE_ADDRESS(0);
	vibrato_voice = FM_VOICE_ADDRESS(0);
	unused_c = leftover_c;
	init_track_channels(unused_c);
	z80_block(17);
	init_pcm_tracks();
	z80_block(17);
	init_midi_parts();
	z80_block(31);
	timer_control = 0x15u;
	fm_register = 0x27u;
	write_fm1(fm_register, timer_control);
	z80_block(31);
	timer_control = 0x15u;
	fm_register = 0x27u;
	write_fm2(fm_register, timer_control);
	z80_block(10);
}

/* Sets up both YM3438: the 11 register/value pairs of fm_global_init to part I of each (LFO, timers,
   every key off, DAC off), the first YM3438's timer A to 0x3EF (registers 0x24 = 0xFB, 0x25 = 3), then
   the 36 pairs of fm_operator_init to all four parts (total level 0x7F, sustain level and release 0xFF,
   SSG-EG 0 on every operator). */
uint16_t init_fm_chips(void)
{
	uint8_t fm_register = 0, init_register = 0, pairs_left = 0, value = 0;
	uint16_t init_entry = 0, leftover_bc = 0;

	z80_block(17);
	init_entry = fm_global_init_ADDRESS;
	pairs_left = 0xBu;
	for (;;)
	{
		z80_block(43);
		init_register = REGISTER_VALUE_PAIR(init_entry)->register_number;
		init_entry++;
		value = REGISTER_VALUE_PAIR(init_entry - 0x1u)->value;
		init_entry++;
		fm_register = init_register;
		write_fm1(fm_register, value);
		z80_block(67);
		fm_register = init_register;
		write_fm2(fm_register, value);
		z80_block(8);
		pairs_left--;
		if (pairs_left == 0u)
			break;
		z80_cycles(5);
	}
	z80_block(31);
	value = 0xFBu;
	fm_register = 0x24u;
	write_fm1(fm_register, value);
	z80_block(81);
	value = 3u;
	fm_register = 0x25u;
	write_fm1(fm_register, value);
	z80_block(17);
	init_entry = fm_operator_init_ADDRESS;
	pairs_left = 0x24u;
	for (;;)
	{
		z80_block(43);
		init_register = REGISTER_VALUE_PAIR(init_entry)->register_number;
		init_entry++;
		value = REGISTER_VALUE_PAIR(init_entry - 0x1u)->value;
		init_entry++;
		fm_register = init_register;
		write_fm1(fm_register, value);
		z80_block(67);
		fm_register = init_register;
		write_fm2(fm_register, value);
		z80_block(67);
		fm_register = init_register;
		write_fm1_part2(fm_register, value);
		z80_block(67);
		fm_register = init_register;
		write_fm2_part2(fm_register, value);
		z80_block(8);
		pairs_left--;
		if (pairs_left == 0u)
			break;
		z80_cycles(5);
	}
	z80_block(10);
	leftover_bc = ((uint32_t)pairs_left << 8) | init_register;
	return leftover_bc;
}

/* Writes value to register fm_register on part I of the YM3438 that fm_channel's bit 3 picks (the second
   when set), for the registers of the whole chip such as key on/off (0x28); hands back value. */
uint8_t write_fm_global(uint8_t fm_register, uint8_t value)
{
	uint8_t chip_channel = 0;

	z80_block(39);
	chip_channel = fm_channel;
	if (((chip_channel >> 3) & 1u) == 0u)
	{
		write_fm1(fm_register, value);
		return value;
	}
	z80_block(10);
	write_fm2(fm_register, value);
	return value;
}

/* Writes value to register fm_register on the chip and part of fm_channel's channel: the second YM3438
   when its bit 3 is set, part II when its bit 2 is set. */
void write_fm_channel(uint8_t fm_register, uint8_t value)
{
	uint8_t channel_then_value = 0, saved_value = 0;
	uint8_t fm2_chip_channel = 0, fm1_chip_channel = 0;

	z80_block(35);
	saved_value = value;
	channel_then_value = fm_channel;
	if (((channel_then_value >> 3) & 1u) != 0u)
	{
		z80_block(22);
		fm2_chip_channel = channel_then_value;
		channel_then_value = saved_value;
		if (((fm2_chip_channel >> 2) & 1u) != 0u)
		{
			write_fm2_part2(fm_register, channel_then_value);
			return;
		}
		write_fm2(fm_register, channel_then_value);
		return;
	}
	z80_block(22);
	fm1_chip_channel = channel_then_value;
	channel_then_value = saved_value;
	if (((fm1_chip_channel >> 2) & 1u) != 0u)
	{
		write_fm1_part2(fm_register, channel_then_value);
		return;
	}
	write_fm1(fm_register, channel_then_value);
}

/* Writes value to register fm_register of the first YM3438's part I (ports 0x80 and 0x81), interrupts
   off between the two writes. */
void write_fm1(uint8_t fm_register, uint8_t value)
{
	uint8_t saved_value = 0;

	z80_block(73);
	saved_value = value;
	interrupts_off();
	port_out(FM1_ADDRESS, (uint8_t)fm_register);
	port_out(FM1_DATA, saved_value);
	interrupts_on();
}

/* Writes value to register fm_register of the first YM3438's part II (ports 0x82 and 0x83), interrupts
   off between the two writes. */
void write_fm1_part2(uint8_t fm_register, uint8_t value)
{
	uint8_t saved_value = 0;

	z80_block(73);
	saved_value = value;
	interrupts_off();
	port_out(FM1_PART2_ADDRESS, (uint8_t)fm_register);
	port_out(FM1_PART2_DATA, saved_value);
	interrupts_on();
}

/* Writes value to register fm_register of the second YM3438's part I (ports 0x90 and 0x91), interrupts
   off between the two writes. */
void write_fm2(uint8_t fm_register, uint8_t value)
{
	uint8_t saved_value = 0;

	z80_block(73);
	saved_value = value;
	interrupts_off();
	port_out(FM2_ADDRESS, (uint8_t)fm_register);
	port_out(FM2_DATA, saved_value);
	interrupts_on();
}

/* Writes value to register fm_register of the second YM3438's part II (ports 0x92 and 0x93), interrupts
   off between the two writes. */
void write_fm2_part2(uint8_t fm_register, uint8_t value)
{
	uint8_t saved_value = 0;

	z80_block(73);
	saved_value = value;
	interrupts_off();
	port_out(FM2_PART2_ADDRESS, (uint8_t)fm_register);
	port_out(FM2_PART2_DATA, saved_value);
	interrupts_on();
}

/* Clears the RF5C68's 16 wave banks through the window at 0xD000 (register 7 picks the bank, sound off):
   even banks to 0x80, odd ones to 0xFF but 0x2BC-0xABC to 0x80; each fill also writes the byte past the
   window (midi_in_message_length). Then the 41 register/value pairs of pcm_register_init. */
void init_pcm_chip(uint32_t unused_c)
{
	uint8_t bank_pairs_left = 0, init_register = 0, pairs_left = 0, pcm_register = 0, value = 0;
	uint16_t init_entry = 0;
	uint8_t saved_bank_pairs_left = 0;

	z80_block(14);
	value = 0u;
	bank_pairs_left = 8u;
	for (;;)
	{
		z80_block(35);
		saved_bank_pairs_left = bank_pairs_left;
		pcm_register = 7u;
		write_pcm_register(pcm_register, value);
		z80_block(84);
		z80_write8(PCM_WAVE_WINDOW, 0x80u);
		move_block(PCM_WAVE_WINDOW, 0xD001u, 0x1000u, 1u);
		value++;
		pcm_register = 7u;
		write_pcm_register(pcm_register, value);
		z80_block(134);
		z80_write8(PCM_WAVE_WINDOW, 0xFFu);
		move_block(PCM_WAVE_WINDOW, 0xD001u, 0x1000u, 1u);
		z80_write8(PCM_SILENT_LOOP, 0x80u);
		move_block(PCM_SILENT_LOOP, 0xD2BDu, 0x800u, 1u);
		value++;
		bank_pairs_left = saved_bank_pairs_left - 1u;
		if (bank_pairs_left == 0u)
			break;
		z80_cycles(5);
	}
	z80_block(17);
	init_entry = pcm_register_init_ADDRESS;
	pairs_left = 0x29u;
	for (;;)
	{
		z80_block(43);
		init_register = REGISTER_VALUE_PAIR(init_entry)->register_number;
		init_entry++;
		value = REGISTER_VALUE_PAIR(init_entry - 0x1u)->value;
		init_entry++;
		pcm_register = init_register;
		write_pcm_register(pcm_register, value);
		z80_block(8);
		pairs_left--;
		if (pairs_left == 0u)
			break;
		z80_cycles(5);
	}
	z80_block(10);
}

/* Writes register_value to RF5C68 register pcm_register (at 0xC000 + it). */
void write_pcm_register(uint8_t pcm_register, uint8_t register_value)
{
	z80_block(45);
	z80_write8((pcm_register & 0xFFu) | 0xC000u, register_value);
}

/* Sets the chip_channel of the 20 voices (the 8 sample voices, then the 12 FM voices after them) from
   voice_chip_channels: 0-7 the RF5C68's channels, 0x80-0x8E the YM3438's (bit 3 the chip, bit 2 the
   part). */
void init_track_channels(uint32_t unused_c)
{
	uint8_t voices_left = 0;
	uint16_t code_entry = 0, sample_voice = 0;
	uint8_t saved_voices_left = 0;

	z80_block(27);
	sample_voice = 0xE401u;
	code_entry = voice_chip_channels_ADDRESS;
	voices_left = 0x14u;
	for (;;)
	{
		z80_block(70);
		saved_voices_left = voices_left;
		SAMPLE_VOICE(sample_voice - 0x1u)->chip_channel = VOICE_CHIP_CODES(code_entry)[0];
		code_entry++;
		sample_voice += 0x12u;
		voices_left = saved_voices_left - 1u;
		if (voices_left == 0u)
			break;
		z80_cycles(5);
	}
	z80_block(10);
}

/* Starts the 8 sample voices: wave_page 0xD0 and stream_position 0x07FF each. */
void init_pcm_tracks(void)
{
	uint8_t voices_left = 0;
	uint16_t sample_voice = 0;

	z80_block(17);
	sample_voice = 0xE409u;
	voices_left = 8u;
	for (;;)
	{
		z80_block(98);
		SAMPLE_VOICE(sample_voice - 0x9u)->wave_page = 0xD0u;
		sample_voice += 7u;
		SAMPLE_VOICE(sample_voice - 0x10u)->stream_position_low = 0xFFu;
		sample_voice++;
		SAMPLE_VOICE(sample_voice - 0x11u)->stream_position_high = 7u;
		sample_voice += 0xAu;
		voices_left--;
		if (voices_left == 0u)
			break;
		z80_cycles(5);
	}
	z80_block(10);
}

/* Starts the 13 MIDI parts: part_templates[1]'s 8 parts (0x48 bytes) to MIDI part 0 and the first 0x2D
   bytes of part_templates[0] (5 parts) to part 8; then each part's effective_volume = volume *
   (music_volume, or 0xFF for parts with flags bit 0) >> 8, + 1 when not 0. */
void init_midi_parts(void)
{
	uint8_t factor = 0, value = 0;
	uint16_t count = 0, part = 0, template_then_product = 0;

	z80_block(34);
	value = 1u;
	template_then_product = part_templates_ADDRESS;
	template_then_product = read_table_word(template_then_product, value);
	z80_block(70);
	move_block(template_then_product, MIDI_PART_ADDRESS(0), 0x48u, 1u);
	value = 0u;
	template_then_product = part_templates_ADDRESS;
	template_then_product = read_table_word(template_then_product, value);
	z80_block(57);
	move_block(template_then_product, MIDI_PART_ADDRESS(8), 0x2Du, 1u);
	template_then_product += 0x2Du;
	part = MIDI_PART_ADDRESS(0);
	count = 0xD00u;
	for (;;)
	{
		z80_block(56);
		template_then_product = (template_then_product & 0xFFu) | ((uint32_t)MIDI_PART(part)->volume << 8);
		value = 0xFFu;
		if ((MIDI_PART(part)->flags & 1u) == 0u)
		{
			z80_block(13);
			value = music_volume;
		}
		z80_block(21);
		factor = value;
		template_then_product = multiply_bytes(factor, template_then_product);
		z80_block(18);
		value = (uint8_t)(template_then_product >> 8);
		if (value != 0u)
		{
			z80_block(4);
			value++;
		}
		z80_block(52);
		MIDI_PART(part)->effective_volume = value;
		part += 9u;
		count = (uint32_t)(uint8_t)((count >> 8) - 1u) << 8;
		if (count >> 8 == 0u)
			break;
		z80_cycles(5);
	}
	z80_block(10);
}

/* Hands back the 16-bit entry index of the table at table. */
uint16_t read_table_word(uint16_t table, uint8_t index)
{
	uint16_t offset = 0, word = 0;

	z80_block(88);
	offset = index;
	table = table + offset + offset;
	word = TABLE_WORDS(table)[0];
	return word;
}

/* Hands back value's high byte times factor's low byte, 8 by 8 bits by shift and add (the volume
   scalings keep the product's high byte). */
uint16_t multiply_bytes(uint16_t factor, uint16_t value)
{
	uint32_t carry = 0;

	z80_block(35);
	factor &= 0xFFu;
	value &= ~0xFFu;
	carry = (int16_t)value < 0;
	value = value * 2u;
	if (carry)
	{
		z80_block(11);
		value += factor;
	}
	z80_block(21);
	carry = (int16_t)value < 0;
	value = value * 2u;
	if (carry)
	{
		z80_block(11);
		value += factor;
	}
	z80_block(21);
	carry = (int16_t)value < 0;
	value = value * 2u;
	if (carry)
	{
		z80_block(11);
		value += factor;
	}
	z80_block(21);
	carry = (int16_t)value < 0;
	value = value * 2u;
	if (carry)
	{
		z80_block(11);
		value += factor;
	}
	z80_block(21);
	carry = (int16_t)value < 0;
	value = value * 2u;
	if (carry)
	{
		z80_block(11);
		value += factor;
	}
	z80_block(21);
	carry = (int16_t)value < 0;
	value = value * 2u;
	if (carry)
	{
		z80_block(11);
		value += factor;
	}
	z80_block(21);
	carry = (int16_t)value < 0;
	value = value * 2u;
	if (carry)
	{
		z80_block(11);
		value += factor;
	}
	z80_block(16);
	carry = (int16_t)value < 0;
	value = value * 2u;
	if (carry == 0u)
	{
		z80_cycles(6);
		return value;
	}
	z80_block(21);
	value += factor;
	return value;
}

/* Copies 256 bytes from sample_address to wave_address (a page of sample data into the RF5C68's wave
   memory window) and hands both back moved on by 256. */
void copy_sample_page(uint16_t *p_wave_address, uint16_t *p_sample_address)
{
	uint16_t wave_address = (uint16_t)*p_wave_address;
	uint16_t sample_address = (uint16_t)*p_sample_address;

	for (uint32_t repeat = 0; repeat < 255u; repeat++)
	{
		z80_block(16);
		z80_write8(wave_address, z80_read8(sample_address));
		sample_address++;
		wave_address++;
	}
	z80_block(26);
	z80_write8(wave_address, z80_read8(sample_address));
	sample_address++;
	wave_address++;
	*p_wave_address = wave_address;
	*p_sample_address = sample_address;
}

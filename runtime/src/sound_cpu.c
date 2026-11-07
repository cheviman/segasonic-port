/**
 *    Name:        sound_cpu.c
 *    Description: runs the decompiled sound program on the board's time: the Z80's timing model
 *    Version:     0.1.0
 *    Created:     13-10-2026
 *    Modified:    07-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    The sound program is C (src/sound/, tools/decompile_sound.py) and keeps the Z80's time itself: every
 *    basic block of the original starts with z80_block(cycles). The rules, the same tools/reference/sound
 *    keeps for MAME's Z80 so that the two compare sample for sample:
 *      - the Z80's time counts in board cycles (two a Z80 cycle) and moves on by each block's cycles
 *        (tools/z80/timing.py), the extra cycles of a condition's longer way, and 21 for every round of a
 *        block move but its last;
 *      - when a block starts, the YM3438 timers due by then run out, and an interrupt is taken if the
 *        line is up and the Z80 lets interrupts in (not right after an EI that ended the block before):
 *        13 cycles, then sound_irq(); after it returns the same is looked at again;
 *      - everything a block reads and writes happens at the time it starts;
 *      - the board runs the program whenever its time moves on (s32_board.time_passes): from its reset
 *        once the I/O chip lets the Z80 run, then the main loop again and again until the Z80's time is
 *        past the board's (the loop's top is where it waits, sound_symbols.txt's "loop" line); while the
 *        Z80 is held in reset, only the timers run.
 */
#include "sound/support.h"
#include "sound/chips.h"

#include <stdio.h>

#define INTERRUPT_CYCLES 13

/* the sound program's ways in (sound_symbols.txt) */
void sound_reset(void);
void sound_irq(void);
void sound_main_loop(void);

static struct
{
	uint64_t clock;         /* where the next block starts */
	bool interrupts;        /* IFF1 */
	bool after_ei;          /* EI ended the block before */
	bool line;              /* the board's interrupt line to the Z80 */
	bool held;              /* in reset */
	bool running;           /* past the reset code: the main loop runs */
} z80 = { 0, false, false, false, true, false };

void z80_block(int cycles)
{
	for (;;)
	{
		sound_chips_timers_due(z80.clock);
		if (!z80.interrupts || z80.after_ei || !z80.line)
			break;
		z80.interrupts = false;
		z80.clock += 2 * INTERRUPT_CYCLES;
		sound_irq();
	}
	z80.after_ei = false;
	s32_board.sound_cycles = z80.clock;
	z80.clock += 2 * (uint64_t)cycles;
}

void z80_cycles(int cycles)
{
	z80.clock += 2 * (uint64_t)cycles;
}

void interrupts_on(void)
{
	z80.interrupts = true;
}

void interrupts_on_after_block(void)
{
	z80.interrupts = true;
	z80.after_ei = true;
}

void interrupts_off(void)
{
	z80.interrupts = false;
}

uint8_t z80_read8(uint16_t address)
{
	return s32_z80_read(address);
}

uint16_t z80_read16(uint16_t address)
{
	const uint8_t low = s32_z80_read(address);
	return (uint16_t)(low | s32_z80_read((uint16_t)(address + 1)) << 8);
}

void z80_write8(uint16_t address, uint8_t value)
{
	s32_z80_write(address, value);
}

void z80_write16(uint16_t address, uint16_t value)
{
	s32_z80_write(address, (uint8_t)value);
	s32_z80_write((uint16_t)(address + 1), (uint8_t)(value >> 8));
}

uint8_t port_in(uint8_t port)
{
	return s32_z80_in(port);
}

void port_out(uint8_t port, uint8_t value)
{
	s32_z80_out(port, value);
}

void move_block(uint16_t source, uint16_t destination, uint16_t count, bool upward)
{
	const uint32_t rounds = count ? count : 0x10000;

	for (uint32_t round = 0; round < rounds; round++)
	{
		s32_z80_write(destination, s32_z80_read(source));
		source = (uint16_t)(upward ? source + 1 : source - 1);
		destination = (uint16_t)(upward ? destination + 1 : destination - 1);
	}
	z80.clock += 2 * 21 * (uint64_t)(rounds - 1);
}

void sound_invalid_jump(uint32_t address)
{
	if (s32_board.log)
	{
		char message[64];
		snprintf(message, sizeof(message), "sound program: a jump to %04X, past its table", (unsigned)address);
		s32_board.log(message);
	}
}

/* ---- the board's side ---- */

static void on_time_passes(uint64_t until)
{
	if (z80.held)
	{
		sound_chips_timers_due(until);
		if (z80.clock < until)
			z80.clock = until;
		return;
	}
	if (!z80.running)
	{
		z80.running = true;
		sound_reset();
	}
	while (z80.clock < until)
		sound_main_loop();
}

static void on_sound_irq(bool asserted, uint8_t vector)
{
	(void)vector;    /* interrupt mode 1: always 0038 */
	z80.line = asserted;
}

static void on_sound_reset(bool held)
{
	if (held)
	{
		z80.running = false;
		z80.interrupts = false;
		z80.after_ei = false;
	}
	z80.held = held;
}

void sound_cpu_power_on(void)
{
	z80.clock = 0;
	z80.interrupts = false;
	z80.after_ei = false;
	z80.line = false;
	z80.held = true;
	z80.running = false;
}

/* Hooks the sound program to the board; s32_board.sound_rom and the chips have to be set */
void sound_cpu_attach(void)
{
	z80.clock = s32_board.cycles;
	s32_board.time_passes = on_time_passes;
	s32_board.sound_irq_changed = on_sound_irq;
	s32_board.sound_reset_changed = on_sound_reset;
}

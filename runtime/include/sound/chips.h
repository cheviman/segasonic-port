/**
 *    Name:        chips.h
 *    Description: the sound board's chips and the Z80 program on the board's time (sound_chips.c, sound_cpu.c)
 *    Version:     0.1.0
 *    Created:     13-10-2026
 *    Modified:    07-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    A port sets s32_board.sound_rom, calls sound_chips_attach() and sound_cpu_attach(), and at the end of
 *    every frame sound_chips_end_frame() with the board's time: the frame's samples are then there to
 *    play, each chip at its own rate (a YM3438 every 288 board cycles, the RF5C68 every 384 x 16107950 /
 *    12500000).
 */
#ifndef SOUND_CHIPS_H
#define SOUND_CHIPS_H

#include "s32/machine.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The two YM3438 and the RF5C68 on s32_board.chips */
void sound_chips_attach(void);

/* The decompiled sound program on the board's time (sound_cpu.c) */
void sound_cpu_attach(void);

/* The sound CPU as at power-on (held in reset, its time at the board's), before the board is set up again
   (s32_init): a LAN game's start and end, the same on every device */
void sound_cpu_power_on(void);

/* The YM3438 timers due by 'time' run out (sound_cpu.c calls it where a block starts) */
void sound_chips_timers_due(uint64_t time);

/* The frame ends at 'time': the samples that start before it are the frame's */
void sound_chips_end_frame(uint64_t time);

/* The frame's samples of chip 0 or 1 (YM3438) or 2 (RF5C68), left and right interleaved */
const int32_t *sound_chips_frame_samples(int chip, size_t *count);

/* FNV-1a hashes, as tools/reference/s32ref writes them: the frame's samples of the three chips, and
   the sound board's memories (the shared RAM but the Z80's stack, FE00-FEFF, and the wave memory) */
uint64_t sound_chips_frame_hash(void);
uint64_t sound_board_state_hash(void);

#ifdef __cplusplus
}
#endif

#endif

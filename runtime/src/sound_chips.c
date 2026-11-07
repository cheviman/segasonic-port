/**
 *    Name:        sound_chips.c
 *    Description: the sound board's chips on the board's time: two YM3438 and the RF5C68, their samples
 *                 frame by frame
 *    Version:     0.1.0
 *    Created:     13-10-2026
 *    Modified:    07-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    Plugs runtime/src/ym3438.c and rf5c68.c into the Z80's side of the board (s32_board.chips). The chips
 *    keep no time: before every write they make the samples that start before it, at the time the Z80's
 *    block makes its accesses (s32_board.sound_cycles, sound_cpu.c), and the timers they ask for run out
 *    where sound_cpu.c says (sound_chips_timers_due). Sample n of a YM3438 starts at n x 288 board cycles
 *    (144 of its clocks), the RF5C68's at n x 384 x 16107950 / 12500000. tools/reference/sound/s32sound.cpp
 *    does the same with ymfm's and MAME's chips: the frame's hashes compare.
 */
#include "sound/chips.h"

#include "s32/rf5c68.h"
#include "s32/ym3438.h"

#include <stdlib.h>
#include <string.h>

#define FM_PERIOD           288u                                        /* board cycles a YM3438 sample takes */
#define PCM_NUMERATOR       ((uint64_t)384 * S32_CPU_CLOCK)             /* board cycles x S32_PCM_CLOCK a sample takes */
#define STACK_START         0xFE00u                                     /* the Z80's stack, FE00-FEFF: not in the hash */
#define STACK_END           0xFF00u
#define NO_TIMER            UINT64_MAX

/* The samples a chip has made and the frame has not taken yet: left and right, from sample 'first' on */
typedef struct sample_buffer
{
	int32_t *data;
	size_t count;           /* values (two a sample) */
	size_t capacity;
	uint64_t first;
} sample_buffer;

typedef struct fm_host
{
	s32_fm chip;
	int number;
	uint64_t samples;                   /* made so far */
	uint64_t deadline[2];               /* when its timers run out, board cycles */
	uint64_t busy_until;
	sample_buffer buffer;
} fm_host;

static struct
{
	fm_host fm[2];
	s32_pcm pcm;
	uint64_t pcm_samples;
	sample_buffer pcm_buffer;
	uint64_t override;                  /* while a timer runs out: its own time */
	uint64_t frame_hash;
	int32_t *frame[3];                  /* the frame's samples, for the mix */
	size_t frame_counts[3];
	size_t frame_capacity[3];
} sound = { .override = NO_TIMER };

static uint64_t now(void)
{
	return sound.override != NO_TIMER ? sound.override : s32_board.sound_cycles;
}

static void append(sample_buffer *buffer, const int32_t out[2])
{
	if (buffer->count + 2 > buffer->capacity)
	{
		buffer->capacity = buffer->capacity ? buffer->capacity * 2 : 4096;
		buffer->data = (int32_t *)realloc(buffer->data, buffer->capacity * sizeof(int32_t));
	}
	buffer->data[buffer->count++] = out[0];
	buffer->data[buffer->count++] = out[1];
}

/* ---- the YM3438s ---- */

static void fm_catch_up(fm_host *host, uint64_t time)
{
	const uint64_t due = (time + FM_PERIOD - 1) / FM_PERIOD;
	int32_t out[2];

	while (host->samples < due)
	{
		s32_fm_generate(&host->chip, out);
		append(&host->buffer, out);
		host->samples++;
	}
}

static void fm_set_timer(void *opaque, int timer, int32_t clocks)
{
	fm_host *host = (fm_host *)opaque;

	host->deadline[timer] = clocks < 0 ? NO_TIMER : now() + 2 * (uint64_t)clocks;
}

static void fm_update_irq(void *opaque, bool asserted)
{
	/* only the first chip's interrupt is wired */
	if (((fm_host *)opaque)->number != 0)
		return;
	if (asserted)
		s32_sound_signal_irq(S32_SOUND_IRQ_FM);
	else
		s32_sound_clear_irq(S32_SOUND_IRQ_FM);
}

static void fm_set_busy_end(void *opaque, uint32_t clocks)
{
	((fm_host *)opaque)->busy_until = now() + 2 * (uint64_t)clocks;
}

static bool fm_is_busy(void *opaque)
{
	return now() < ((fm_host *)opaque)->busy_until;
}

static uint64_t fm_next_timer(const fm_host *host)
{
	return host->deadline[0] < host->deadline[1] ? host->deadline[0] : host->deadline[1];
}

/* one chip's timers due by 'time', in the order they run out */
static void fm_fire(fm_host *host, uint64_t time)
{
	while (fm_next_timer(host) <= time)
	{
		const int timer = host->deadline[0] <= host->deadline[1] ? 0 : 1;
		const uint64_t when = host->deadline[timer];

		host->deadline[timer] = NO_TIMER;
		fm_catch_up(host, when);
		sound.override = when;
		s32_fm_timer_expired(&host->chip, timer);
		sound.override = NO_TIMER;
	}
}

void sound_chips_timers_due(uint64_t time)
{
	for (;;)
	{
		const uint64_t first = fm_next_timer(&sound.fm[0]);
		const uint64_t second = fm_next_timer(&sound.fm[1]);
		fm_host *host = first <= second ? &sound.fm[0] : &sound.fm[1];

		if (fm_next_timer(host) > time)
			return;
		fm_fire(host, fm_next_timer(host));
	}
}

static uint8_t fm_read(int chip, int offset)
{
	return s32_fm_read(&sound.fm[chip].chip, offset);
}

static void fm_write(int chip, int offset, uint8_t data)
{
	fm_catch_up(&sound.fm[chip], now());
	s32_fm_write(&sound.fm[chip].chip, offset, data);
}

/* ---- the RF5C68 ---- */

static void pcm_catch_up(uint64_t time)
{
	const uint64_t due = (time * S32_PCM_CLOCK + PCM_NUMERATOR - 1) / PCM_NUMERATOR;
	int32_t out[2];

	while (sound.pcm_samples < due)
	{
		s32_pcm_generate(&sound.pcm, out);
		append(&sound.pcm_buffer, out);
		sound.pcm_samples++;
	}
}

static void pcm_write(int offset, uint8_t data)
{
	pcm_catch_up(now());
	s32_pcm_write(&sound.pcm, offset, data);
}

static uint8_t pcm_memory_read(int offset)
{
	return s32_pcm_memory_read(&sound.pcm, offset);
}

static void pcm_memory_write(int offset, uint8_t data)
{
	pcm_catch_up(now());
	s32_pcm_memory_write(&sound.pcm, offset, data);
}

/* ---- frames ---- */

static uint64_t hash_bytes(const void *data, size_t size, uint64_t hash)
{
	const uint8_t *bytes = (const uint8_t *)data;

	for (size_t index = 0; index < size; index++)
		hash = (hash ^ bytes[index]) * 0x100000001b3ull;
	return hash;
}

/* the samples before sample number 'due': hashed and moved to the frame's list 'which' */
static uint64_t take(sample_buffer *buffer, uint64_t due, uint64_t hash, int which)
{
	size_t values = (size_t)(due - buffer->first) * 2;

	if (values > buffer->count)
		values = buffer->count;
	if (values > sound.frame_capacity[which])
	{
		sound.frame_capacity[which] = values;
		sound.frame[which] = (int32_t *)realloc(sound.frame[which], values * sizeof(int32_t));
	}
	memcpy(sound.frame[which], buffer->data, values * sizeof(int32_t));
	sound.frame_counts[which] = values;
	memmove(buffer->data, buffer->data + values, (buffer->count - values) * sizeof(int32_t));
	buffer->count -= values;
	buffer->first += values / 2;
	return hash_bytes(sound.frame[which], values * sizeof(int32_t), hash);
}

void sound_chips_end_frame(uint64_t time)
{
	const uint64_t fm_due = (time + FM_PERIOD - 1) / FM_PERIOD;
	const uint64_t pcm_due = (time * S32_PCM_CLOCK + PCM_NUMERATOR - 1) / PCM_NUMERATOR;
	uint64_t hash = 0xcbf29ce484222325ull;

	fm_catch_up(&sound.fm[0], time);
	fm_catch_up(&sound.fm[1], time);
	pcm_catch_up(time);
	hash = take(&sound.fm[0].buffer, fm_due, hash, 0);
	hash = take(&sound.fm[1].buffer, fm_due, hash, 1);
	sound.frame_hash = take(&sound.pcm_buffer, pcm_due, hash, 2);
}

uint64_t sound_chips_frame_hash(void)
{
	return sound.frame_hash;
}

uint64_t sound_board_state_hash(void)
{
	uint64_t hash = 0xcbf29ce484222325ull;

	hash = hash_bytes(s32_board.shared_ram, STACK_START - 0xE000u, hash);
	hash = hash_bytes(s32_board.shared_ram + (STACK_END - 0xE000u), 0x10000u - STACK_END, hash);
	return hash_bytes(s32_board.pcm_ram, sizeof(s32_board.pcm_ram), hash);
}

const int32_t *sound_chips_frame_samples(int chip, size_t *count)
{
	if (count)
		*count = sound.frame_counts[chip] / 2;
	return sound.frame[chip];
}

void sound_chips_attach(void)
{
	for (int number = 0; number < 2; number++)
	{
		fm_host *host = &sound.fm[number];

		memset(&host->chip, 0, sizeof(host->chip));
		host->number = number;
		host->samples = 0;    /* (a board set up again: its time from 0, as at power-on; a LAN game's start) */
		host->busy_until = 0;
		host->buffer.count = 0;
		host->buffer.first = 0;
		host->deadline[0] = host->deadline[1] = NO_TIMER;
		host->chip.host = host;
		host->chip.set_timer = fm_set_timer;
		host->chip.update_irq = fm_update_irq;
		host->chip.set_busy_end = fm_set_busy_end;
		host->chip.is_busy = fm_is_busy;
		s32_fm_init(&host->chip);
	}
	sound.pcm_samples = 0;
	sound.pcm_buffer.count = 0;
	sound.pcm_buffer.first = 0;
	sound.override = NO_TIMER;
	for (int chip = 0; chip < 3; chip++)
		sound.frame_counts[chip] = 0;
	s32_pcm_reset(&sound.pcm, s32_board.pcm_ram);
	s32_board.chips.fm_read = fm_read;
	s32_board.chips.fm_write = fm_write;
	s32_board.chips.pcm_write = pcm_write;
	s32_board.chips.pcm_memory_read = pcm_memory_read;
	s32_board.chips.pcm_memory_write = pcm_memory_write;
}

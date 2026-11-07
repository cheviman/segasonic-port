/**
 *    Name:        main.c
 *    Description: runs the game with no screen or sound, with the controls a script drives, and writes a
 *                 hash of the board state (and of the frame the video draws) after every frame, in the
 *                 format tools/reference/s32ref uses
 *    Version:     0.1.0
 *    Created:     11-10-2026
 *    Modified:    07-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    Usage: sonic_headless [--frames N] [--hashes FILE] [--script FILE]
 *                          [--dump-frame N --dump-dir DIR] [--video-hashes FILE] [--video-dump N ... --video-dir DIR]
 *                          [--sound-hashes FILE] [--sound-levels FILE] [--wide] [--assets FILE] [--extract ROM_SET]
 *                          [--actions FILE]
 *    The game's data is its assets pack (game/data.h): --assets, or segasonic.assets beside the executable.
 *    --extract first writes that pack from the player's ROM set (a .zip or a folder), as the first run does
 *    (runtime/src/extract.c). The scaffold (SEGASONIC_SCAFFOLD) runs the original code, so it reads the ROM
 *    set from rom/ instead.
 *    --dump-frame saves the board's memories at the end of that frame, in the files tools/reference/s32ref
 *    writes, so the two compare byte by byte. --video-hashes and --video-dump draw every frame with the
 *    board's video (runtime/src/video.c): the hash of each frame, and frames as BMP. --sound-hashes runs the
 *    sound board too (the decompiled sound program and the chips, runtime/src/sound_cpu.c and
 *    sound_chips.c), and writes for each frame the hash of its samples and of the sound board's memories.
 *    --sound-levels runs it too, and writes for each frame how loud each chip and the window port's mix of
 *    them are, and how many of the mix's samples pass 16 bits (write_levels). SEGASONIC_VIDEO_WORKERS=N in
 *    the environment draws the video's rows on N threads beside this one (runtime/src/workers.c), as the
 *    ports do: the frames are the same, which the checks show.
 *    --actions (the console build) presses the console's actions in the frames a file names, as the window
 *    port's keys do: the menus, OPTIONS, the map (tools/make_screenshots.py); and section+ and section-, the
 *    window's F8 and F7, move the camera to the stage's section after or before.
 */
#include "s32/machine.h"
#include "s32/program.h"
#include "s32/script.h"
#include "s32/video.h"
#include "s32/workers.h"
#include "sound/chips.h"
#include "game/data.h"
#include "game/extract.h"
#include "s32/bus.h"
#ifdef SEGASONIC_CONSOLE
#include "console.h"
#endif

#include <inttypes.h>
#include <setjmp.h>
#include <time.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>    /* (Sleep: --pace) */
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef SEGASONIC_CONSOLE
#define USAGE_ACTIONS " [--actions FILE]"
#else
#define USAGE_ACTIONS ""
#endif
#define USAGE "Usage: sonic_headless [--frames N] [--hashes FILE] [--script FILE]" \
	" [--dump-frame N --dump-dir DIR] [--video-hashes FILE] [--video-dump N ... --video-dir DIR]" \
	" [--sound-hashes FILE] [--sound-levels FILE] [--sprite-census FILE] [--tile-census FILE] [--memory-census FILE] [--wide]" \
	" [--assets FILE] [--extract ROM_SET]" USAGE_ACTIONS "\n"
#define ASSETS_FILE "segasonic.assets"
#define MAX_VIDEO_DUMPS 64
#define CENSUS_SLOTS (1u << 20)

static jmp_buf finished;
static uint64_t frames_wanted = 600;
/* The frames run, a LAN game's resets (the board's and the program's frames from 0 again) counted in: the actions',
   the dumps', the hashes' and --frames' (the same as the program's where nothing resets the board) */
static uint64_t frames_before_reset, vblanks_before_reset;
#define FRAMES_RUN   (frames_before_reset + program_frames())
#define VBLANKS_RUN  (vblanks_before_reset + s32_board.frame)
static bool pace;          /* --pace: 60 frames a second, as a window's (a LAN game's rooms are found in seconds) */
static bool wide_video;    /* --wide: the video's wide picture (s32_video_set_wide) */
static bool lan_sound;     /* --lan-name or --lan-hashes: the sound board runs, as the window port's always does */
static FILE *hashes;
static s32_script *script;
static uint64_t dump_frame = UINT64_MAX;
static const char *dump_dir;
static FILE *video_hashes;
static uint64_t video_dumps[MAX_VIDEO_DUMPS];
static int video_dump_count;
static const char *video_dir;
static FILE *sound_hashes;
static FILE *sound_levels;
static FILE *sprite_census;
static uint64_t *census_seen;    /* hashes of the sprite and color sets written, open addressing (0 = free) */

/* --sprite-census: every sprite of the ROM the list draws, with the colors its pens show, once per set of
   colors and way of drawing it: "ADDRESS WORDS ROWS BITS OPAQUE COLOR..." (hex; OPAQUE 1 when it draws the
   end code and goes on past it, else 0; colors RGB555, one a pen) */
static void on_sprite_drawn(uint32_t address, int words, int rows, bool bits8, bool opaque, const uint16_t *pixels)
{
	const int pens = bits8 ? 256 : 16;
	uint16_t colors[256];
	uint64_t hash = 0xcbf29ce484222325ull;
	uint32_t slot;

	for (int pen = 0; pen < pens; pen++)
		colors[pen] = s32_video_sprite_pixel_color(pixels[pen]);
	hash = (hash ^ address) * 0x100000001b3ull;
	hash = (hash ^ (uint64_t)((uint32_t)words << 16 | (uint32_t)rows << 2 | (opaque ? 2u : 0u) | (bits8 ? 1u : 0u)))
		* 0x100000001b3ull;
	for (int pen = 0; pen < pens; pen++)
		hash = (hash ^ colors[pen]) * 0x100000001b3ull;
	if (hash == 0)
		hash = 1;
	for (slot = (uint32_t)hash & (CENSUS_SLOTS - 1); census_seen[slot] != 0; slot = (slot + 1) & (CENSUS_SLOTS - 1))
		if (census_seen[slot] == hash)
			return;
	census_seen[slot] = hash;
	fprintf(sprite_census, "%06X %d %d %d %d", address, words, rows, bits8 ? 8 : 4, opaque ? 1 : 0);
	for (int pen = 0; pen < pens; pen++)
		fprintf(sprite_census, " %04X", colors[pen]);
	fputc('\n', sprite_census);
}

/* --tile-census: every tile the tilemaps that are on show (every TILE_CENSUS_EVERY frames), with the colors
   its pens show, once per set of colors: "TILE COLOR TILEMAP COLOR..." (hex; colors RGB555, one a pen) */
#define TILE_CENSUS_EVERY 30
static FILE *tile_census;
static uint64_t *tile_census_seen;

static void on_tile_shown(int tilemap, uint32_t tile, uint16_t color)
{
	uint16_t colors[16];
	uint64_t hash = 0xcbf29ce484222325ull;
	uint32_t slot;

	if (program_frames() % TILE_CENSUS_EVERY != 0)
		return;
	for (int pen = 0; pen < 16; pen++)
		colors[pen] = s32_video_tile_pen_color(tilemap, color, pen);
	hash = (hash ^ tile) * 0x100000001b3ull;
	for (int pen = 0; pen < 16; pen++)
		hash = (hash ^ colors[pen]) * 0x100000001b3ull;
	if (hash == 0)
		hash = 1;
	for (slot = (uint32_t)hash & (CENSUS_SLOTS - 1); tile_census_seen[slot] != 0; slot = (slot + 1) & (CENSUS_SLOTS - 1))
		if (tile_census_seen[slot] == hash)
			return;
	tile_census_seen[slot] = hash;
	fprintf(tile_census, "%04X %03X %d", tile, color, tilemap);
	for (int pen = 0; pen < 16; pen++)
		fprintf(tile_census, " %04X", colors[pen]);
	fputc('\n', tile_census);
}

static void write_file(const char *name, const void *data, size_t size)
{
	char path[1024];
	FILE *output;

	snprintf(path, sizeof(path), "%s/%s", dump_dir, name);
	output = fopen(path, "wb");
	if (!output)
	{
		fprintf(stderr, "%s: cannot write\n", path);
		return;
	}
	fwrite(data, 1, size, output);
	fclose(output);
}

static void dump_state(void)
{
	write_file("work_ram.bin", s32_board.work_ram, sizeof(s32_board.work_ram));
	write_file("video_ram.bin", s32_board.video_ram, sizeof(s32_board.video_ram));
	write_file("sprite_ram.bin", s32_board.sprite_ram, sizeof(s32_board.sprite_ram));
	write_file("palette.bin", s32_board.palette, sizeof(s32_board.palette));
	write_file("mixer.bin", s32_board.mixer, sizeof(s32_board.mixer));
	write_file("shared_ram.bin", s32_board.shared_ram, sizeof(s32_board.shared_ram));
	write_file("eeprom.bin", s32_board.eeprom.data, sizeof(s32_board.eeprom.data));
}

/* The last frame the video drew, as a 24-bit BMP */
static void write_bmp(const char *path)
{
	int width;
	const uint32_t *pixels = s32_video_frame(&width);
	const uint32_t row = ((uint32_t)width * 3 + 3) & ~3u;
	const uint32_t size = 54 + row * S32_VIDEO_HEIGHT;
	uint8_t *file = (uint8_t *)calloc(size, 1);
	FILE *output;

	if (!file)
		return;
	for (int byte = 0; byte < 4; byte++)
	{
		file[2 + byte] = (uint8_t)(size >> (8 * byte));
		file[18 + byte] = (uint8_t)((uint32_t)width >> (8 * byte));
		file[22 + byte] = (uint8_t)((uint32_t)S32_VIDEO_HEIGHT >> (8 * byte));
	}
	file[0] = 'B';
	file[1] = 'M';
	file[10] = 54;
	file[14] = 40;
	file[26] = 1;
	file[28] = 24;
	for (int y = 0; y < S32_VIDEO_HEIGHT; y++)
		for (int x = 0; x < width; x++)
		{
			const uint32_t pixel = pixels[y * S32_VIDEO_MAX_WIDTH + x];
			uint8_t *out = &file[54 + (uint32_t)(S32_VIDEO_HEIGHT - 1 - y) * row + (uint32_t)x * 3];

			out[0] = (uint8_t)pixel;
			out[1] = (uint8_t)(pixel >> 8);
			out[2] = (uint8_t)(pixel >> 16);
		}
	output = fopen(path, "wb");
	if (output)
	{
		fwrite(file, 1, size, output);
		fclose(output);
	}
	else
		fprintf(stderr, "%s: cannot write\n", path);
	free(file);
}

static void on_irq_changed(bool asserted)
{
	program_irq_line(asserted);
}

static void on_log(const char *message)
{
	fprintf(stderr, "[s32] %s\n", message);
}

/* --sound-levels: the frame's loudest sample of each chip (FM0, FM1: the YM3438s; PCM: the RF5C68) and of
   their mix as the window port makes it (each YM3438 at 0.30, the RF5C68 at 0.40, volume 1; the RF5C68's
   samples taken at the YM3438s' times), how many of the mix's samples pass 16 bits, and how many of the
   RF5C68's are at its own limit (it sums its channels into 16 bits): "FRAME FM0 FM1 PCM MIX CLIPPED PCM_LIMIT" */
static void write_levels(void)
{
	const int32_t *samples[3];
	size_t counts[3];
	int32_t peaks[3] = { 0 };
	double mix_peak = 0.0;
	unsigned clipped = 0, pcm_limit = 0;

	for (int chip = 0; chip < 3; chip++)
	{
		samples[chip] = sound_chips_frame_samples(chip, &counts[chip]);
		for (size_t at = 0; at < counts[chip] * 2; at++)
		{
			if (abs(samples[chip][at]) > peaks[chip])
				peaks[chip] = abs(samples[chip][at]);
			if (chip == 2 && (samples[chip][at] >= 32767 || samples[chip][at] <= -32767))
				pcm_limit++;
		}
	}
	for (size_t at = 0; counts[1] && counts[2] && at < counts[0]; at++)
	{
		const size_t fm1 = at < counts[1] ? at : counts[1] - 1, pcm = at * counts[2] / counts[0];

		for (int side = 0; side < 2; side++)
		{
			const double value = 0.30 * (samples[0][at * 2 + side] + samples[1][fm1 * 2 + side])
				+ 0.40 * samples[2][pcm * 2 + side];

			if (value > mix_peak || -value > mix_peak)
				mix_peak = value < 0 ? -value : value;
			if (value > 32767.0 || value < -32768.0)
				clipped++;
		}
	}
	fprintf(sound_levels, "%" PRIu64 " %d %d %d %.0f %u %u\n", FRAMES_RUN - 1, peaks[0], peaks[1], peaks[2],
		mix_peak, clipped, pcm_limit);
}

static void sleep_ms(int milliseconds)
{
#ifdef _WIN32
	Sleep((unsigned long)milliseconds);
#else
	const struct timespec wait = { 0, (long)milliseconds * 1000000L };

	nanosleep(&wait, NULL);
#endif
}

/* (the system's steady clock: Android's bionic has no timespec_get before API 29) */
static double seconds_now(void)
{
#ifdef _WIN32
	LARGE_INTEGER now, frequency;

	QueryPerformanceCounter(&now);
	QueryPerformanceFrequency(&frequency);
	return (double)now.QuadPart / (double)frequency.QuadPart;
#else
	struct timespec now;

	clock_gettime(CLOCK_MONOTONIC, &now);
	return (double)now.tv_sec + (double)now.tv_nsec / 1e9;
#endif
}

/* --pace: the frame's sixtieth of a second, as a window's (TMNT's headless') */
static void pace_frame(void)
{
	static double next;
	double now = seconds_now();

	if (next == 0.0 || now > next + 0.1)
		next = now;
	next += 1.0 / 60.0;
	while ((now = seconds_now()) < next)
		sleep_ms(1);
}

#ifdef SEGASONIC_CONSOLE
static bool lan_restart_wanted;    /* program_run left for a LAN game's start or end (console_lan_reset_wanted) */
#endif

static void on_frame_done(void)
{
	if (hashes)
		fprintf(hashes, "%" PRIu64 " %016" PRIx64 "\n", FRAMES_RUN - 1, s32_state_hash());
	if (dump_dir && FRAMES_RUN - 1 == dump_frame)
		dump_state();
	if (video_hashes)
		fprintf(video_hashes, "%" PRIu64 " %016" PRIx64 "\n", FRAMES_RUN - 1, s32_video_frame_hash());
	if (sound_hashes || sound_levels)
	{
		s32_board.time_passes(s32_board.cycles);
		sound_chips_end_frame(s32_board.cycles);
		if (sound_hashes)
			fprintf(sound_hashes, "%" PRIu64 " %016" PRIx64 " %016" PRIx64 "\n", FRAMES_RUN - 1,
				sound_chips_frame_hash(), sound_board_state_hash());
		if (sound_levels)
			write_levels();
	}
	for (int index = 0; index < video_dump_count && video_dir; index++)
		if (video_dumps[index] == FRAMES_RUN - 1)
		{
			char path[1024];

			snprintf(path, sizeof(path), "%s/frame_%" PRIu64 ".bmp", video_dir, video_dumps[index]);
			write_bmp(path);
		}
	if (FRAMES_RUN >= frames_wanted)
		longjmp(finished, 1);
	if (pace)
		pace_frame();
#ifdef SEGASONIC_CONSOLE
	if (console_lan_reset_wanted())
	{
		lan_restart_wanted = true;    /* (a LAN game's start or end: the board reset, main's) */
		longjmp(finished, 1);
	}
#endif
}

#ifdef SEGASONIC_CONSOLE
/* --actions: the console's actions (console_actions.h) a file presses, as the window port's keys do, one a
   line: "FRAME ACTION [FRAMES]", FRAME a frame number or +N for N frames after the previous line, held FRAMES
   frames (1 by default) and pressed on the first; '#' starts a comment */
#define MAX_ACTION_EVENTS 1024

static struct
{
	uint64_t frame, frames;
	console_action action;
	int section;          /* section+ (1) or section- (-1) instead of an action: the window's F8 and F7 */
} action_events[MAX_ACTION_EVENTS];
static int action_event_count;

static const char *const action_names[ACTIONS] = { "up", "down", "left", "right", "accept", "back", "start", "jump" };

static bool load_actions(const char *path)
{
	FILE *input = fopen(path, "r");
	char line[256];
	uint64_t previous = 0;
	int number = 0;

	if (!input)
	{
		fprintf(stderr, "%s: cannot be read\n", path);
		return false;
	}
	while (fgets(line, sizeof(line), input))
	{
		char when[32], name[32];
		unsigned long long frames = 1;
		char *comment = strchr(line, '#');
		int fields, action = 0, section;

		number++;
		if (comment)
			*comment = '\0';
		fields = sscanf(line, "%31s %31s %llu", when, name, &frames);
		if (fields <= 0)
			continue;
		while (fields >= 2 && action < ACTIONS && strcmp(name, action_names[action]) != 0)
			action++;
		section = fields < 2 ? 0 : (strcmp(name, "section+") == 0 ? 1 : (strcmp(name, "section-") == 0 ? -1 : 0));
		if (fields < 2 || (action == ACTIONS && !section) || frames == 0 || action_event_count == MAX_ACTION_EVENTS)
		{
			fprintf(stderr, "%s:%d: not an action line\n", path, number);
			fclose(input);
			return false;
		}
		previous = when[0] == '+' ? previous + strtoull(when + 1, NULL, 10) : strtoull(when, NULL, 10);
		action_events[action_event_count].frame = previous;
		action_events[action_event_count].frames = frames;
		action_events[action_event_count].action = (console_action)(section ? 0 : action);
		action_events[action_event_count].section = section;
		action_event_count++;
	}
	fclose(input);
	return true;
}

static bool actions_hold_start, actions_hold_button;    /* player 1's start and button, held by the actions */

/* The frame's actions, as the window port hands them over (ports/sdl2/main.c): start is player 1's start
   button too outside a game (or with their start prompt up), jump their button, the directions roll their
   trackball */
static void apply_actions(uint64_t frame)
{
	uint32_t pressed = 0, held = 0;
	bool start, button;
	int roll_x, roll_y;

	for (int index = 0; index < action_event_count; index++)
		if (action_events[index].section)
		{
			if (frame == action_events[index].frame)
				console_debug_section(action_events[index].section);
		}
		else if (frame >= action_events[index].frame && frame - action_events[index].frame < action_events[index].frames)
		{
			held |= ACTION_BIT(action_events[index].action);
			if (frame == action_events[index].frame)
				pressed |= ACTION_BIT(action_events[index].action);
		}
	start = (held & ACTION_BIT(ACTION_START))
		&& (console_lan_playing() || !console_in_game() || console_start_prompted(0));    /* (a LAN game's: lan_controls') */
	button = (held & ACTION_BIT(ACTION_JUMP)) != 0;
	/* (the ports are active low; a button the actions held is released when they let go of it) */
	if (start)
		s32_board.input.port_e &= (uint8_t)~0x10u;
	else if (actions_hold_start)
		s32_board.input.port_e |= 0x10u;
	if (button)
		s32_board.input.port_a &= (uint8_t)~0x01u;
	else if (actions_hold_button)
		s32_board.input.port_a |= 0x01u;
	actions_hold_start = start;
	actions_hold_button = button;
	/* up, down, left and right roll player 1's trackball, TRACKBALL's KEYS counts a frame (the board's x counts
	   the other way) */
	roll_x = ((held & ACTION_BIT(ACTION_LEFT)) != 0u) - ((held & ACTION_BIT(ACTION_RIGHT)) != 0u);
	roll_y = ((held & ACTION_BIT(ACTION_DOWN)) != 0u) - ((held & ACTION_BIT(ACTION_UP)) != 0u);
	s32_board.input.trackball_x[0] = (uint16_t)(s32_board.input.trackball_x[0] + roll_x * console_window_settings.key_roll);
	s32_board.input.trackball_y[0] = (uint16_t)(s32_board.input.trackball_y[0] + roll_y * console_window_settings.key_roll);
	if (console_lan_playing())
	{
		if (pressed & ACTION_BIT(ACTION_BACK))
			console_lan_leave_game();    /* (a LAN game's: out of it, as the window's BACK) */
		console_actions(0, 0);
		return;
	}
	if (!console_in_game() && (pressed & ACTION_BIT(ACTION_BACK)))
		console_back();
	console_actions(pressed, held);
}

/* --lan-hashes: each LAN frame's number and the state's hash before it, as the host compares them */
static FILE *lan_hashes;
static bool lan_given;
static s32_inputs lan_own;     /* player 1's controls as the script and the actions make them, apart from the board's */
static s32_inputs lan_board;   /* the board's, every slot's, while the script and the actions make player 1's */

/* A LAN game's frame (the window port's lan_controls): player 1's controls, the script's and the actions', to the
   network (the trackball's turn since the last frame, its button and START), every slot's back onto the board's
   players; held a frame at a time till they come */
static void lan_controls(void)
{
	uint8_t own[CONSOLE_LAN_INPUT], slots[CONSOLE_LAN_SLOTS][CONSOLE_LAN_INPUT];
	const s32_inputs mine = s32_board.input;
	const int turn_x = (int16_t)(mine.trackball_x[0] - lan_own.trackball_x[0]);
	const int turn_y = (int16_t)(mine.trackball_y[0] - lan_own.trackball_y[0]);
	uint8_t port_a = 0xFFu, port_b = 0xFFu, port_e = 0xFFu;
	int held = 0;

	own[0] = (uint8_t)(int8_t)(turn_x < -127 ? -127 : (turn_x > 127 ? 127 : turn_x));
	own[1] = (uint8_t)(int8_t)(turn_y < -127 ? -127 : (turn_y > 127 ? 127 : turn_y));
	own[2] = (uint8_t)(((mine.port_a & 0x01u) == 0u ? CONSOLE_LAN_BUTTON : 0u)
		| ((mine.port_e & 0x10u) == 0u ? CONSOLE_LAN_START : 0u));
	lan_own = mine;
	if (lan_given && lan_hashes)
		fprintf(lan_hashes, "%" PRIu32 " %016" PRIx64 "\n", console_lan_frame_number(), console_lan_state_hash());
	while (!console_lan_frame(own, slots))
	{
		sleep_ms(16);
		if (++held > 60 * 30)
		{
			fprintf(stderr, "the LAN game held 30 seconds\n");
			exit(4);
		}
	}
	lan_given = console_lan_playing();
	s32_board.input = lan_board;
	for (int slot = 0; slot < CONSOLE_LAN_SLOTS; slot++)
	{
		static const uint8_t start_bits[CONSOLE_LAN_SLOTS] = { 0x10u, 0x20u, 0x80u };
		const bool button = (slots[slot][2] & CONSOLE_LAN_BUTTON) != 0u;
		const bool start = (slots[slot][2] & CONSOLE_LAN_START) != 0u && (!console_in_game() || console_start_prompted(slot));

		s32_board.input.trackball_x[slot] = (uint16_t)(s32_board.input.trackball_x[slot] + (int8_t)slots[slot][0]);
		s32_board.input.trackball_y[slot] = (uint16_t)(s32_board.input.trackball_y[slot] + (int8_t)slots[slot][1]);
		if (button && slot == 1)
			port_b &= (uint8_t)~0x01u;
		else if (button)
			port_a &= (uint8_t)~(slot == 0 ? 0x01u : 0x04u);
		if (start)
			port_e &= (uint8_t)~start_bits[slot];
	}
	s32_board.input.port_a = port_a;
	s32_board.input.port_b = port_b;
	s32_board.input.port_e = port_e;
}
#endif

static void on_controls_update(void)
{
	/* VBLANK of the board's frame is starting */
#ifdef SEGASONIC_CONSOLE
	const bool lan = console_lan_playing();

	if (lan)
	{
		lan_board = s32_board.input;    /* (the script and the actions make player 1's apart: lan_controls) */
		s32_board.input = lan_own;
	}
#endif
	s32_script_apply(script, VBLANKS_RUN);
#ifdef SEGASONIC_CONSOLE
	apply_actions(VBLANKS_RUN);
	if (lan && console_lan_playing())
		lan_controls();
	else if (lan)
		s32_board.input = lan_board;    /* (BACK left the game: the board's own, its frame with nothing pressed) */
	if (wide_video)
		s32_video_set_wide(console_wide_now());    /* (as the console's port does: the stages wider) */
#endif
}

int main(int argc, char **argv)
{
	const char *hashes_path = NULL;
	const char *script_path = NULL;
	const char *video_hashes_path = NULL;
	static const char *sound_hashes_path;    /* (static: read after a LAN restart's longjmp) */
	static const char *sound_levels_path;
	const char *sprite_census_path = NULL;
	const char *tile_census_path = NULL;
	static const char *memory_census_path;
	const char *assets_path = NULL;
	const char *extract_source = NULL;
	char beside[1024];
	bool frames_given = false;

	for (int index = 1; index < argc; index++)
	{
		if (strcmp(argv[index], "--frames") == 0 && index + 1 < argc)
		{
			frames_wanted = strtoull(argv[++index], NULL, 10);
			frames_given = true;
		}
		else if (strcmp(argv[index], "--hashes") == 0 && index + 1 < argc)
			hashes_path = argv[++index];
		else if (strcmp(argv[index], "--script") == 0 && index + 1 < argc)
			script_path = argv[++index];
		else if (strcmp(argv[index], "--dump-frame") == 0 && index + 1 < argc)
			dump_frame = strtoull(argv[++index], NULL, 10);
		else if (strcmp(argv[index], "--dump-dir") == 0 && index + 1 < argc)
			dump_dir = argv[++index];
		else if (strcmp(argv[index], "--video-hashes") == 0 && index + 1 < argc)
			video_hashes_path = argv[++index];
		else if (strcmp(argv[index], "--video-dump") == 0 && index + 1 < argc && video_dump_count < MAX_VIDEO_DUMPS)
			video_dumps[video_dump_count++] = strtoull(argv[++index], NULL, 10);
		else if (strcmp(argv[index], "--wide") == 0)
			wide_video = true;
#ifdef SEGASONIC_CONSOLE
		else if (strcmp(argv[index], "--actions") == 0 && index + 1 < argc)
		{
			if (!load_actions(argv[++index]))
				return 1;
		}
		else if (strcmp(argv[index], "--lan-name") == 0 && index + 1 < argc)
		{
			snprintf(console_window_settings.name, sizeof(console_window_settings.name), "%s", argv[++index]);
			lan_sound = true;    /* (a LAN game's state is the sound board's too: the window port's) */
		}
		else if (strcmp(argv[index], "--lan-hashes") == 0 && index + 1 < argc)
		{
			lan_hashes = fopen(argv[++index], "w");
			lan_sound = true;
		}
#endif
		else if (strcmp(argv[index], "--pace") == 0)
			pace = true;
		else if (strcmp(argv[index], "--video-dir") == 0 && index + 1 < argc)
			video_dir = argv[++index];
		else if (strcmp(argv[index], "--sound-hashes") == 0 && index + 1 < argc)
			sound_hashes_path = argv[++index];
		else if (strcmp(argv[index], "--sound-levels") == 0 && index + 1 < argc)
			sound_levels_path = argv[++index];
		else if (strcmp(argv[index], "--sprite-census") == 0 && index + 1 < argc)
			sprite_census_path = argv[++index];
		else if (strcmp(argv[index], "--tile-census") == 0 && index + 1 < argc)
			tile_census_path = argv[++index];
		else if (strcmp(argv[index], "--memory-census") == 0 && index + 1 < argc)
			memory_census_path = argv[++index];
		else if (strcmp(argv[index], "--assets") == 0 && index + 1 < argc)
			assets_path = argv[++index];
		else if (strcmp(argv[index], "--extract") == 0 && index + 1 < argc)
			extract_source = argv[++index];
		else
		{
			fprintf(stderr, USAGE);
			return 1;
		}
	}
#ifdef SEGASONIC_SCAFFOLD
	(void)assets_path;
	(void)beside;
	{
		const char *problem = game_rom_set_load("rom");

		if (problem)
		{
			fprintf(stderr, "rom: %s\n", problem);
			return 1;
		}
	}
#else
	if (!assets_path)
	{
		/* (the executable's folder, as argv[0] gives it) */
		const char *slash = strrchr(argv[0], '/'), *backslash = strrchr(argv[0], '\\');
		const char *last = slash > backslash ? slash : backslash;

		snprintf(beside, sizeof(beside), "%.*s" ASSETS_FILE, last ? (int)(last - argv[0] + 1) : 0, argv[0]);
		assets_path = beside;
	}
	if (extract_source)
	{
		const char *problem = game_data_extract(extract_source, assets_path);

		if (problem)
		{
			fprintf(stderr, "%s: %s\n", extract_source, problem);
			return 1;
		}
	}
	switch (game_data_load(assets_path))
	{
	case GAME_DATA_LOADED:
		break;
	case GAME_DATA_MISSING:
		fprintf(stderr, "%s: no game assets\n", assets_path);
		return 1;
	default:
		fprintf(stderr, "%s: the game assets cannot be read\n", assets_path);
		return 1;
	}
#endif
	if (script_path)
	{
		char error[256];

		script = s32_script_load(script_path, error, sizeof(error));
		if (!script)
		{
			fprintf(stderr, "%s\n", error);
			return 1;
		}
		if (!frames_given && s32_script_frames(script))
			frames_wanted = s32_script_frames(script);
	}
	if (hashes_path)
		hashes = fopen(hashes_path, "w");
	if (video_hashes_path || video_dump_count || sprite_census_path || tile_census_path)
	{
#ifdef SEGASONIC_CONSOLE
		console_prepare_sprites(game_sprites, GAME_SPRITES_SIZE, game_program);
		s32_video_init(game_tiles, GAME_TILES_SIZE, game_sprites, GAME_SPRITES_SIZE);
		s32_board.frame_shown = console_draw_frame;    /* (WIDE's sides of a stage's map too) */
#else
		s32_video_init(game_tiles, GAME_TILES_SIZE, game_sprites, GAME_SPRITES_SIZE);
#endif
#ifdef SEGASONIC_CONSOLE
		console_window_settings.wide = wide_video ? CONSOLE_WIDE_16_9 : CONSOLE_WIDE_OFF;
#else
		s32_video_set_wide(wide_video);
#endif
		if (video_hashes_path)
			video_hashes = fopen(video_hashes_path, "w");
	}
	if (sprite_census_path)
	{
		sprite_census = fopen(sprite_census_path, "w");
		census_seen = calloc(CENSUS_SLOTS, sizeof(*census_seen));
		if (!sprite_census || !census_seen)
		{
			fprintf(stderr, "%s: cannot be written\n", sprite_census_path);
			return 1;
		}
		s32_video_sprite_observer = on_sprite_drawn;
	}
	if (tile_census_path)
	{
		tile_census = fopen(tile_census_path, "w");
		tile_census_seen = calloc(CENSUS_SLOTS, sizeof(*tile_census_seen));
		if (!tile_census || !tile_census_seen)
		{
			fprintf(stderr, "%s: cannot be written\n", tile_census_path);
			return 1;
		}
		s32_video_tile_observer = on_tile_shown;
	}
	if (sound_hashes_path || sound_levels_path || lan_sound)
	{
		s32_board.sound_rom = game_sound;
		sound_chips_attach();
		sound_cpu_attach();
		if (sound_hashes_path)
			sound_hashes = fopen(sound_hashes_path, "w");
		if (sound_levels_path)
			sound_levels = fopen(sound_levels_path, "w");
	}

	s32_board.irq_changed = on_irq_changed;
	s32_board.controls_update = on_controls_update;
	s32_board.log = on_log;
	s32_board.frame_timing = true;
	/* (the video's rows on that many threads beside this one: the same frames, which the checks show) */
	if (getenv("SEGASONIC_VIDEO_WORKERS"))
		s32_workers_start(atoi(getenv("SEGASONIC_VIDEO_WORKERS")));
	s32_init(game_program);
	program_reset();
	program_frame_done = on_frame_done;

	for (;;)
	{
		if (!setjmp(finished))
			program_run();
#ifdef SEGASONIC_CONSOLE
		if (lan_restart_wanted && FRAMES_RUN < frames_wanted)
		{
			/* (a LAN game's start or end: the sound board, the board and the program as at power-on, as the window
			   port's lan_restart, the console as at power-on with the host's EEPROM, the program from its start) */
			lan_restart_wanted = false;
			frames_before_reset += program_frames();
			vblanks_before_reset += s32_board.frame;
			if (lan_sound || sound_hashes_path || sound_levels_path)
			{
				sound_chips_attach();
				sound_cpu_power_on();
			}
			s32_init(game_program);
			console_lan_power_on();
			program_reset();
			memset(&lan_own, 0, sizeof(lan_own));
			lan_given = false;
			continue;
		}
#endif
		break;
	}
#ifdef SEGASONIC_CONSOLE
	if (lan_hashes)
		fclose(lan_hashes);
#endif

	if (hashes)
		fclose(hashes);
	if (video_hashes)
		fclose(video_hashes);
	if (sound_hashes)
		fclose(sound_hashes);
	if (sound_levels)
		fclose(sound_levels);
	if (sprite_census)
		fclose(sprite_census);
	free(census_seen);
	if (tile_census)
		fclose(tile_census);
	free(tile_census_seen);
	if (memory_census_path)
	{
#ifdef S32_MEMORY_CENSUS
		FILE *census = fopen(memory_census_path, "w");
		if (census)
		{
			s32_memory_census_write(census);
			fclose(census);
		}
		else
			fprintf(stderr, "%s: cannot be written\n", memory_census_path);
#else
		fprintf(stderr, "--memory-census: this build does not count memory accesses\n");
#endif
	}
	s32_script_free(script);
	printf("frames:            %" PRIu64 "\n", program_frames());
	printf("interrupts taken:  %u\n", program_interrupts());
	printf("sprite lists:      %u\n", s32_board.sprite_lists);
	printf("sound requests:    %u\n", s32_board.sound_requests);
	printf("unmapped reads:    %u\n", s32_board.unmapped_reads);
	printf("unmapped writes:   %u\n", s32_board.unmapped_writes);
	printf("eeprom written:    %s\n", s32_board.eeprom.dirty ? "yes" : "no");
	return 0;
}

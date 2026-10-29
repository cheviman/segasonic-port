/**
 *    Name:        console.c
 *    Description: the console build's changes to the game: no licence screen, the attract mode's own order and
 *                 sound, no coins, PRESS START on the title and a menu over it
 *    Version:     0.1.0
 *    Created:     19-10-2026
 *    Modified:    29-10-2026
 *    Author:      Sebastian Toyos
 *
 *    The game's C (src/) stays as the decompilation wrote it. A function listed in console/replaced.txt is
 *    original_NAME in this build, and the function here takes its place (and may call it).
 *
 *    The menu is drawn over the title, which keeps drawing SegaSonic's logo under it. It reads the game's
 *    own controls: player 1's trackball up and down moves the cursor (the arrow keys roll it), the
 *    start button or the button picks.
 */
#include "game.h"
#include "console.h"
#include "s32/video.h"

#include <math.h>
#include <string.h>

/* System modes (run_frame) */
#define MODE_OPTIONS   1u    /* never in the arcade's attract sequence: the console's OPTIONS */
#define MODE_RANKING   2u
#define MODE_TITLE     3u
#define MODE_DEMO_1    4u
#define MODE_DEMO_2    5u
#define MODE_GAME      6u

/* inputs_pressed: player 1's start and button */
#define PRESSED_START_1    0x10u
#define PRESSED_BUTTON_1   0x10000u
#define PRESSED_STARTS_2_3 0xA0u        /* players 2 and 3's starts, */
#define PRESSED_BUTTONS    0x1050000u   /* and the three players' buttons */

#define PRESS_START_ROW    25u      /* under the emblem's ribbon */
#define MENU_ROW           23u      /* the menu's first line, under the logo */
#define COPYRIGHT_ROW      27u      /* the title's (C) SEGA 1992 line */
#define MENU_TRAVEL        48       /* the trackball's y travel that moves the cursor one line, */
#define MENU_PAUSE         16       /* and the frames before it moves again */
#define SEGA_LOGO_LAST     0x3C     /* sega_logo_timer at the end of its last step (run_sega_logo, step 6) */
#define BLINK_FRAMES       32u
#define TITLE_HOLD         0x240    /* title_timer the title keeps under the menu */
#define MENU_TEXT          0x00u    /* text attributes (even ones: the odd ones are clear): white, */
#define MENU_CHOSEN        0x0Au    /* and yellow for the line the cursor is on */

/* The attract mode's screens: the title (the SEGA logo, then SegaSonic's), then the ranking, then a demo */
static const uint8_t attract_order[8] = {
	MODE_TITLE, MODE_RANKING, MODE_DEMO_1, MODE_TITLE, MODE_RANKING, MODE_DEMO_2, MODE_TITLE, MODE_RANKING
};

/* (where the device's system leaves games, SEGASONIC_SYSTEM_EXIT: the Switch's HOME button, there is no EXIT) */
#ifdef SEGASONIC_SYSTEM_EXIT
static const char *const menu_items[] = { "START GAME", "TIME ATTACK", "OPTIONS" };
#define MENU_ITEMS  3
#else
static const char *const menu_items[] = { "START GAME", "TIME ATTACK", "OPTIONS", "EXIT" };
#define MENU_ITEMS  4
#endif

static uint32_t title_frames;     /* frames the title has shown its logo (PRESS START blinks by them) */
static bool sega_logo_shown;      /* the SEGA logo has been shown (once, at power-on) */
static bool title_music_started;  /* this title's music has been asked for */
static bool menu_open;            /* the menu is over the title */
static bool options_open;         /* OPTIONS is on screen (system mode 1) */
static bool menu_after_title;     /* back from OPTIONS: the title opens its menu */
static bool menu_opening;         /* its first frame */
static int menu_choice;
static int menu_travel;
static int menu_wait;             /* frames before the cursor moves again */
static bool quit_requested;
static bool game_sees_start;      /* the menu started a game: its first frame sees player 1's start */
static uint32_t actions;          /* the port's actions newly pressed this frame, */
static uint32_t actions_held;     /* and held */

/* ---- text in the game's own font (the text layer: a cell is attribute << 8 | character) ---- */

static uint8_t text_attribute(void)
{
	return ROM8(credits_label_text_ADDRESS);    /* the credit line's colours */
}

static void write_text(uint32_t column, uint32_t row, const char *text, uint8_t attribute)
{
	for (; *text; text++, column++)
		mem_write16(TEXT_CELL(column, row), (uint16_t)((attribute << 8) | (uint8_t)*text));
}

static void erase_line(uint32_t row)
{
	uint32_t column;

	for (column = 0; column < 40u; column++)
		mem_write16(TEXT_CELL(column, row), (uint16_t)((text_attribute() << 8) | ' '));
}

static void write_centered(uint32_t row, const char *text, uint8_t attribute)
{
	uint32_t length = 0;

	while (text[length])
		length++;
	write_text(20u - (length + 1u) / 2u, row, text, attribute);
}

static void run_menu(void);
static void run_options(void);
static void open_menu(void);
static void give_game_credits(void);
static void start_time_attack(void);
static void time_attack_frame(void);
static void run_times(void);
static void close_times(void);
static void open_times(void);
static bool time_attack;           /* this game is TIME ATTACK's */
static bool times_open;            /* the BEST TIMES page is on screen (system mode 1, OPTIONS' place) */
static bool times_pending;         /* the run is over: BEST TIMES once its results begin, or the game ends */
static bool clock_running;
static uint8_t time_attack_players; /* the players in play when its clock started (a bit each) */
static int time_attack_level = -1; /* the level the map picked: 0, Eggman's, the whole game */
static void end_game_credits(void);
static void wide_how_to_play(void);
static void debug_sections(void);

/* ---- the SEGA logo's jingle: two chords through the sound driver's own MIDI queue ---- */

/* The sound driver plays its sequences by putting their MIDI messages in a queue in the sound RAM the main
   CPU shares (E100-E1FF there, 0x700100 here), for its MIDI parts to play with the game's instruments: two
   chords put there play like a song of the game's. MIDI channel B plays them: one of the effects' parts,
   at full volume whatever music_volume says (fixed_volume) and with no transpose, with the program the
   port's settings name; its own program comes back after them. */
#define JINGLE_CHANNEL     0x0Bu
#define LAYER_CHANNEL      0x0Cu    /* the layer under it: an octave down, another instrument */
#define MIDI_QUEUE         (SOUND_RAM + 0x100u)
#define MIDI_QUEUE_WRITE   (SOUND_RAM + 0x758u)
#define MIDI_QUEUE_COUNT   (SOUND_RAM + 0x759u)
#define JINGLE_STEP        4u       /* sega_logo_step when the logo has come together */
#define JINGLE_FIRST_FRAMES 36      /* "SE": 0.6 seconds, */
#define JINGLE_END_FRAMES  108      /* "GA": 1.2 more */
#define CHORD_NOTES        3

/* "SE-GA": E flat major, then C major (our own two chords) */
static const uint8_t chord_se[CHORD_NOTES] = { 63u, 67u, 75u };
static const uint8_t chord_ga[CHORD_NOTES] = { 60u, 64u, 72u };

static int jingle_frame = -1;     /* frames since the jingle began, -1: none */
static int jingle_program = 13;   /* the instrument (the port's settings: sega_jingle_program) */
static int jingle_layer = 2;      /* the layer's (sega_jingle_layer; -1: no layer) */

static void midi_message(uint8_t status, uint8_t first, uint8_t second, int bytes)
{
	uint8_t write = mem_read8(MIDI_QUEUE_WRITE);
	const uint8_t message[3] = { status, first, second };
	int i;

	for (i = 0; i < bytes; i++)
	{
		mem_write8(MIDI_QUEUE + write, message[i]);
		write++;
	}
	mem_write8(MIDI_QUEUE_WRITE, write);
	mem_write8(MIDI_QUEUE_COUNT, (uint8_t)(mem_read8(MIDI_QUEUE_COUNT) + 1u));
}

/* One voice of the jingle on 'channel', 'shift' semitones away: at this frame of it, its chords on or off */
static void jingle_voice(uint8_t channel, int program, int shift)
{
	int note;

	if (program < 0)
		return;
	if (jingle_frame == 0)
	{
		midi_message(0xB0u | channel, 7u, 0x7Fu, 3);    /* CC 7, the volume: all of it */
		midi_message(0xC0u | channel, (uint8_t)program, 0u, 2);
		for (note = 0; note < CHORD_NOTES; note++)
			midi_message(0x90u | channel, (uint8_t)(chord_se[note] + shift), 0x7Fu, 3);
	}
	else if (jingle_frame == JINGLE_FIRST_FRAMES)
	{
		for (note = 0; note < CHORD_NOTES; note++)
		{
			midi_message(0x80u | channel, (uint8_t)(chord_se[note] + shift), 0u, 3);
			midi_message(0x90u | channel, (uint8_t)(chord_ga[note] + shift), 0x7Fu, 3);
		}
	}
	else if (jingle_frame == JINGLE_END_FRAMES)
	{
		for (note = 0; note < CHORD_NOTES; note++)
			midi_message(0x80u | channel, (uint8_t)(chord_ga[note] + shift), 0u, 3);
		midi_message(0xC0u | channel, 0u, 0u, 2);    /* the part's own program again */
	}
}

static void play_jingle(void)
{
	if (jingle_frame < 0)
		return;
	jingle_voice(JINGLE_CHANNEL, jingle_program, 0);
	jingle_voice(LAYER_CHANNEL, jingle_layer, -12);
	if (jingle_frame == JINGLE_END_FRAMES)
	{
		jingle_frame = -2;    /* (done) */
		return;
	}
	jingle_frame++;
}

void console_jingle_program(int program, int layer)
{
	jingle_program = program;
	jingle_layer = layer;
}

/* ---- the attract mode ---- */

/* The title shows SegaSonic's emblem: its timer runs (title_emblem_animate), the SEGA logo is over */
static bool title_logo_up(void)
{
	return title_scene_built != 0u && title_timer > 0;
}

/* System mode 0 at power-on (and after the test mode): instead of the licence screen, the title; and no
   coins: the game's own free play (COIN/CREDIT SETTING 26), set over what apply_settings just read */
void attract_warning(uint32_t *p_pixels, uint32_t *p_text)
{
	(void)p_pixels;
	(void)p_text;
	free_play = 1u;
	/* the 3-player cabinet (the test mode's cabinet type): with the 2-player one the game looks player 3's
	   start prompt up past start_prompt_positions_2p's two, and draws it in the middle of the stage */
	setting_cabinet_type = 1u;
	setting_cabinet_config = (uint8_t)(1u | ((uint32_t)setting_coin_chute_type << 1));
	cabinet_config = setting_cabinet_config;
	attract_step = 0u;
	system_mode = attract_order[0];
	system_mode_step = 0u;
}

/* The next attract screen, in the console's order */
void advance_attract_screen(void)
{
	uint32_t next_step = 0;

	if (game_in_progress == 0u)
		end_game_credits();    /* (game over, or the ending, goes on to the attract mode) */
	if (times_pending)
	{
		times_pending = false;    /* (a TIME ATTACK run that reached the ending: its times) */
		open_times();
		return;
	}
	if (menu_open)
		return;    /* (the title stays under the menu) */

	next_step = (uint8_t)(attract_step + 1u) & 7u;
	system_mode_step = 0u;
	system_mode = attract_order[next_step];
	attract_step = next_step;
}

/* A sound: in the attract mode too, every one and the music, as at home (the arcade lets some effects
   through there, and only with ADVERTISE SOUND on) */
void play_sound(uint8_t sound_id, uint32_t *p_write_index, uint32_t *p_write_index_address)
{
	uint32_t write_index = *p_write_index;
	uint32_t write_index_address = *p_write_index_address;

	queue_sound(sound_id, &write_index, &write_index_address);
	*p_write_index = write_index;
	*p_write_index_address = write_index_address;
}

/* System mode 3, the title: with music from its first frame, and PRESS START blinking once the logo is up */
void attract_title(uint32_t *p_color, uint32_t *p_row_pairs_left, uint32_t *p_blocks_per_row, uint32_t *p_text)
{
	uint32_t write_index = 0, write_index_address = 0;

	if (system_mode_step == 0u)
	{
		title_frames = 0;
		title_music_started = false;
		title_timer = 0;    /* (left from the last title until this one builds its emblem) */
		jingle_frame = -1;
	}
	if (system_mode == MODE_TITLE && !sega_logo_shown && sega_logo_step >= JINGLE_STEP && jingle_frame == -1)
		jingle_frame = 0;    /* the SEGA logo is up: its jingle */
	play_jingle();
	if (menu_open && title_timer > TITLE_HOLD)
		title_timer = TITLE_HOLD;    /* (at 0x258 the title frees its logo and ends: not under the menu) */
	original_attract_title(p_color, p_row_pairs_left, p_blocks_per_row, p_text);
	if (system_mode == MODE_TITLE && sega_logo_shown && sega_logo_step == 1u)
	{
		/* the SEGA logo only once, at power-on: then on to its last step, which clears what it drew and
		   lets the title build SegaSonic's emblem */
		sega_logo_step = 6u;
		sega_logo_timer = SEGA_LOGO_LAST;
	}
	if (system_mode == MODE_TITLE && !title_logo_up())
		erase_line(COPYRIGHT_ROW);    /* the (C) SEGA 1992 line: on the title and its menu, not the SEGA logo */
	if (system_mode != MODE_TITLE || !title_logo_up())
		return;
	if (!title_music_started)
	{
		title_music_started = true;    /* (once: asked again, the song starts over every frame) */
		/* the emblem: its music (the arcade's title has none; segasonic.ini's title_music, stage 3's at first) */
		queue_sound((uint8_t)console_window_settings.title_music, &write_index, &write_index_address);
	}
	sega_logo_shown = true;
	if (menu_after_title && !menu_open)
	{
		menu_after_title = false;    /* (back from OPTIONS: the menu again, on OPTIONS) */
		open_menu();
		menu_choice = 1;
	}
	if (menu_open)
	{
		run_menu();
		return;
	}
	title_frames++;
	if ((title_frames / BLINK_FRAMES) & 1u)
		erase_line(PRESS_START_ROW);
	else
		write_centered(PRESS_START_ROW, "PRESS START", text_attribute());
}

/* The credit line: with free play (always, in the console) nothing; the game's otherwise */
void draw_credit_count_now(uint32_t credit_block, uint32_t *p_credits, uint32_t *p_coins_per_credit, uint32_t *p_text,
	uint32_t *p_text_position)
{
	if (free_play != 0u)
	{
		credit_text_position = *p_text_position;
		return;
	}
	original_draw_credit_count_now(credit_block, p_credits, p_coins_per_credit, p_text, p_text_position);
}

/* A start prompt, every 16th frame: the console has no coins, so with fewer credits than a start takes
   (CONTINUES' spent) it shows none, not "Insert coin", and the count beside it says 0 (the "Push start
   button" it showed before is erased); none either for a player TIME ATTACK keeps out (not in its run) */
void blink_start_prompt(uint32_t credit_block, uint32_t prompt_texts, uint32_t *p_blink_bit, uint32_t *p_text,
	uint32_t *p_text_position)
{
	uint32_t text = 0;
	const uint32_t player = OBJECT(current_object)->player;
	const bool kept_out = time_attack && clock_running && player < 3u && (time_attack_players & (1u << player)) == 0u;

	if (kept_out || (free_play == 0u && CREDIT_BLOCK(credit_block)->credits < credits_per_start))
	{
		if ((frame_counter & 0xFu) != 0u)
			return;
		text = prompt_texts + ROM16(prompt_texts);
		erase_text_lines(&text, p_text_position);
		*p_text = text;
		return;
	}
	original_blink_start_prompt(credit_block, prompt_texts, p_blink_bit, p_text, p_text_position);
}

/* ---- the menu ---- */

static void draw_menu(void)
{
	int item;

	for (item = 0; item < MENU_ITEMS; item++)
	{
		erase_line(MENU_ROW + (uint32_t)item);
		write_centered(MENU_ROW + (uint32_t)item, menu_items[item], item == menu_choice ? MENU_CHOSEN : MENU_TEXT);
	}
}

static void erase_menu(void)
{
	int item;

	for (item = 0; item < MENU_ITEMS; item++)
		erase_line(MENU_ROW + (uint32_t)item);
}

static void open_menu(void)
{
	erase_line(PRESS_START_ROW);
	menu_open = true;
	menu_opening = true;
	menu_choice = 0;
	menu_travel = 0;
}

/* A start button outside a game: on the title, the menu (the frame goes on with the title and the menu,
   which reads the button itself); on the ranking or a demo, back to the title. The game starts from the
   menu. */
void run_frame_start_game(uint32_t credits, uint32_t blink_bit, uint32_t credits_needed, uint32_t count,
	uint32_t message_text, uint32_t text_position, uint32_t *p_scale, uint32_t *p_cursor, uint32_t *p_height)
{
	uint32_t sprite_entry = 0, string_cursor = 0;

	(void)credits;
	(void)blink_bit;
	(void)credits_needed;
	(void)text_position;
	if (system_mode == MODE_TITLE)
	{
		if (title_logo_up() && !menu_open)
			open_menu();
		attract_title(p_scale, &count, p_height, &message_text);    /* the title, and the menu over it */
	}
	else if (system_mode == MODE_OPTIONS)
		system_mode_idle();    /* (OPTIONS reads the button itself) */
	else
	{
		attract_step = 0u;
		system_mode = MODE_TITLE;
		system_mode_step = 0u;
	}
	/* the rest of the frame, as the game's own does after starting a game */
	run_objects();
	build_sprite_list(p_scale, &count, p_height, &message_text, &sprite_entry);
	draw_queued_strings((uint16_t)count, (uint16_t)message_text, sprite_entry, p_scale, &string_cursor, p_height);
	*p_cursor = string_cursor;
}

/* A move of a cursor: -1, +1 or none, from player 1's trackball along one axis ('trackball', travelled up to
   MENU_TRAVEL, then MENU_PAUSE frames before the next) or the actions 'minus' and 'plus' */
static int cursor_step(int *travel, int *wait, int16_t trackball, console_action minus, console_action plus)
{
	const uint32_t both = ACTION_BIT(minus) | ACTION_BIT(plus);

	if (*wait > 0)
	{
		(*wait)--;
		*travel = 0;
	}
	else if (actions_held & both)
		/* held, it repeats; the trackball does not count then (an arrow key held rolls it too) */
		*travel = (actions_held & ACTION_BIT(plus)) ? MENU_TRAVEL : -MENU_TRAVEL;
	else
		*travel += trackball;
	if (actions & ACTION_BIT(minus))
		*travel = -MENU_TRAVEL;
	if (actions & ACTION_BIT(plus))
		*travel = MENU_TRAVEL;
	if (*travel > -MENU_TRAVEL && *travel < MENU_TRAVEL)
		return 0;
	*wait = MENU_PAUSE;
	if (*travel > 0)
	{
		*travel = 0;
		return 1;
	}
	*travel = 0;
	return -1;
}

/* A pick: player 1's start or button, or the actions accept and start */
static bool picked(void)
{
	return (inputs_pressed & (PRESSED_START_1 | PRESSED_BUTTON_1)) != 0u
		|| (actions & (ACTION_BIT(ACTION_ACCEPT) | ACTION_BIT(ACTION_START))) != 0u;
}

static void open_options(void);

/* The menu over the title: the cursor follows player 1's trackball up and down (or the actions up and
   down), the start button, the button or the actions accept and start pick */
static void run_menu(void)
{
	bool opening = menu_opening;
	int step = 0;

	menu_opening = false;
	step = cursor_step(&menu_travel, &menu_wait, (int16_t)trackball_1_y, ACTION_UP, ACTION_DOWN);
	menu_choice = (menu_choice + step + MENU_ITEMS) % MENU_ITEMS;
	draw_menu();
	if (opening || !picked())
		return;    /* (the press that opened the menu picks nothing) */
	switch (menu_choice)
	{
	case 0:    /* START GAME: as the game's own start does */
		menu_open = false;
		erase_menu();
		give_game_credits();
		system_mode = MODE_GAME;
		game_mode = 0u;
		system_mode_step = 0u;
		game_in_progress = 0xFFu;
		game_sees_start = true;
		break;
	case 1:    /* TIME ATTACK: the map, then the level picked, timed */
		start_time_attack();
		break;
	case 2:    /* OPTIONS */
		open_options();
		break;
	default:   /* EXIT */
		quit_requested = true;
		break;
	}
}

/* ---- OPTIONS: a page of its own (system mode 1) over the ranking's drifting Sonic faces ---- */

enum
{
	OPTION_DIFFICULTY,
	OPTION_VITALITY,
	OPTION_CONTINUES,
	OPTION_VIDEO,
	OPTION_WINDOW,
	OPTION_FULL_SCREEN,
	OPTION_PIXELS,
	OPTION_WIDE,
	OPTION_VOLUME,
	OPTION_CONTROLS,
	OPTION_TRACKBALL,
	OPTION_SOUND_TEST,
	OPTION_STAGE_SELECT,
	OPTION_BACK,
	OPTION_COUNT
};

static const char *const option_names[OPTION_COUNT] = {
	"DIFFICULTY", "VITALITY", "CONTINUES", "VIDEO", "WINDOW", "FULL SCREEN", "PIXELS", "WIDE", "VOLUME", "CONTROLS",
	"TRACKBALL", "SOUND TEST", "STAGE SELECT", "BACK"
};

/* The pages: OPTIONS, and VIDEO under it (its BACK goes back to OPTIONS). A game device's build
   (SEGASONIC_HANDHELD) has no VIDEO: its picture is fixed; nor CONTROLS where the device's controls are fixed
   (the Switch), but where they may need putting right (SEGASONIC_CONTROLS_PAGE: a handheld's own buttons).
   TRACKBALL (a game of its own, on the how to play's floor) is the computers' alone, as VIDEO is */
static const int main_options[] = {
#if defined(SEGASONIC_HANDHELD) && defined(SEGASONIC_CONTROLS_PAGE)
	OPTION_DIFFICULTY, OPTION_VITALITY, OPTION_CONTINUES, OPTION_CONTROLS, OPTION_VOLUME,
#elif defined(SEGASONIC_HANDHELD)
	OPTION_DIFFICULTY, OPTION_VITALITY, OPTION_CONTINUES, OPTION_VOLUME,
#else
	OPTION_DIFFICULTY, OPTION_VITALITY, OPTION_CONTINUES, OPTION_VIDEO, OPTION_CONTROLS, OPTION_TRACKBALL,
	OPTION_VOLUME,
#endif
	OPTION_SOUND_TEST, OPTION_STAGE_SELECT, OPTION_BACK
};
static const int video_options[] = { OPTION_WINDOW, OPTION_FULL_SCREEN, OPTION_PIXELS, OPTION_WIDE, OPTION_BACK };

#define MAIN_OPTIONS  ((int)(sizeof(main_options) / sizeof(main_options[0])))
#define VIDEO_OPTIONS ((int)(sizeof(video_options) / sizeof(video_options[0])))

static bool video_open;    /* the VIDEO page is on screen, in OPTIONS' place */

#define OPTIONS_TITLE_ROW   4u
#define OPTIONS_FIRST_ROW   8u      /* then a line every two rows */
#define OPTION_NAME_COLUMN  8u
#define OPTION_VALUE_COLUMN 24u
#define WINDOW_SCALE_MOST   6
#define VOLUME_MOST         10

console_window console_window_settings = { 3, false, false, false, false, false, 5, 0x01, 3, 30, 30, 0.35, true, 0.98,
	0.3, false };

static int option_choice;    /* the line chosen on the page shown */
static int option_travel, option_wait, value_travel, value_wait;
static bool game_settings_changed;

/* SOUND TEST: the game's songs, by the sound id that plays each (sound_commands, group 81) */
static const struct
{
	uint8_t sound;
	const char *name;
} songs[] = {
	{ 0x03u, "MAP" }, { 0x01u, "STAGE 3" }, { 0x02u, "STAGES 4 8" }, { 0x04u, "STAGES 5 6" },
	{ 0x81u, "STAGES 9 10" }, { 0x7Du, "STAGE A" }, { 0x00u, "STAGES B-F" }, { 0x7Cu, "ESCAPE" },
	{ 0x7Bu, "STAGE CLEAR" }, { 0x82u, "ENDING" },
};
#define SONGS ((int)(sizeof(songs) / sizeof(songs[0])))

static int song_choice;
static bool stage_select_pending;    /* STAGE SELECT: the stage select (game mode 8), then player select */
static int stage_select_phase;       /* 0 before the stage select, 1 in it, 2 in player select after it */
static uint16_t stage_select_stage;  /* the stage the map picked, started once player select is over */

/* The difficulties' names (the test mode shows them as 1-4, its bar longer the harder) */
static const char *const difficulty_names[4] = { "VERY EASY", "EASY", "NORMAL", "HARD" };

/* The value of an option as its line shows it: "< 2 >", "< ON  >"; nothing for the ones that are picked */
static void option_value(int option, char *text)
{
	int value = 0;

	switch (option)
	{
	case OPTION_DIFFICULTY:
		strcpy(text, "< ");
		strcat(text, difficulty_names[setting_difficulty & 3u]);
		strcat(text, " >");
		return;
	case OPTION_VITALITY:
		value = setting_initial_vitality + 1;
		break;
	case OPTION_CONTINUES:
		if (console_window_settings.continues < 0)
		{
			strcpy(text, "< FREE >");
			return;
		}
		value = console_window_settings.continues;
		break;
	case OPTION_WINDOW:
		value = console_window_settings.window_scale;
		break;
	case OPTION_VOLUME:
		value = console_window_settings.volume;
		break;
	case OPTION_FULL_SCREEN:
		strcpy(text, console_window_settings.full_screen ? "< ON  >" : "< OFF >");
		return;
	case OPTION_WIDE:
		strcpy(text, console_window_settings.wide ? "< 16:9 >" : "< OFF >");
		return;
	case OPTION_PIXELS:
		strcpy(text, console_window_settings.crt ? "< CRT >" : console_window_settings.bilinear ? "< BILINEAR >"
			: console_window_settings.smooth ? "< SMOOTH >" : "< SHARP >");
		return;
	case OPTION_SOUND_TEST:
		strcpy(text, "< ");
		strcat(text, songs[song_choice].name);
		strcat(text, " >");
		return;
	default:
		text[0] = 0;
		return;
	}
	text[0] = '<';
	text[1] = ' ';
	text[2] = (char)(value >= 10 ? '1' : ' ');
	text[3] = (char)('0' + value % 10);
	text[4] = option == OPTION_WINDOW ? 'X' : ' ';
	text[5] = ' ';
	text[6] = '>';
	text[7] = 0;
}

/* The page shown: its options, and how many */
static const int *page_options(int *count)
{
	*count = video_open ? VIDEO_OPTIONS : MAIN_OPTIONS;
	return video_open ? video_options : main_options;
}

static void draw_options(void)
{
	char value[24];
	int count = 0, line;
	const int *options = page_options(&count);

	write_centered(OPTIONS_TITLE_ROW, video_open ? "VIDEO" : "OPTIONS", MENU_CHOSEN);
	for (line = 0; line < count; line++)
	{
		const uint32_t row = OPTIONS_FIRST_ROW + 2u * (uint32_t)line;
		const uint8_t attribute = line == option_choice ? MENU_CHOSEN : MENU_TEXT;

		erase_line(row);
		write_text(OPTION_NAME_COLUMN, row, option_names[options[line]], attribute);
		option_value(options[line], value);
		write_text(OPTION_VALUE_COLUMN, row, value, attribute);
	}
}

/* Either page off the screen (OPTIONS has the more lines) */
static void erase_options(void)
{
	uint32_t row;

	for (row = OPTIONS_TITLE_ROW; row < OPTIONS_FIRST_ROW + 2u * (uint32_t)MAIN_OPTIONS; row++)
		erase_line(row);
}

/* VIDEO in OPTIONS' place, and back: OPTIONS with VIDEO chosen */
static void open_video(void)
{
	erase_options();
	video_open = true;
	option_choice = 0;
	option_wait = MENU_PAUSE;
}

static void close_video(void)
{
	int line;

	erase_options();
	video_open = false;
	for (line = 0; line < MAIN_OPTIONS; line++)
		if (main_options[line] == OPTION_VIDEO)
			option_choice = line;
	option_wait = MENU_PAUSE;
}

static int clamp(int value, int least, int most)
{
	return value < least ? least : value > most ? most : value;
}

#define CONTINUES_MOST 9

/* A game the menu starts: CONTINUES' FREE is the game's own free play; a number, free play off and the
   shared credits (cabinet_config 1) the start's and the continues', which joining and continuing take
   (pay_and_join: a player who joins later takes them too) */
static void give_game_credits(void)
{
	if (console_window_settings.continues < 0)
		return;
	free_play = 0u;
	p1_credits = (uint8_t)(credits_per_start + (uint32_t)console_window_settings.continues);
}

/* Out of a game: free play again (the console has no coins) and no credits left over */
static void end_game_credits(void)
{
	free_play = 1u;
	p1_credits = 0u;
}

static void change_option(int option, int step)
{
	int pixels = 0;

	switch (option)
	{
	case OPTION_DIFFICULTY:
		setting_difficulty = (uint8_t)clamp(setting_difficulty + step, 0, 3);
		game_settings_changed = true;
		break;
	case OPTION_VITALITY:
		setting_initial_vitality = (uint8_t)clamp(setting_initial_vitality + step, 0, 3);
		game_settings_changed = true;
		break;
	case OPTION_CONTINUES:
		console_window_settings.continues = clamp(console_window_settings.continues + step, -1, CONTINUES_MOST);
		console_window_settings.changed = true;
		break;
	case OPTION_WINDOW:
		console_window_settings.window_scale = clamp(console_window_settings.window_scale + step, 1,
			WINDOW_SCALE_MOST);
		console_window_settings.changed = true;
		break;
	case OPTION_FULL_SCREEN:
		console_window_settings.full_screen = !console_window_settings.full_screen;
		console_window_settings.changed = true;
		break;
	case OPTION_WIDE:
		console_window_settings.wide = !console_window_settings.wide;
		console_window_settings.changed = true;
		break;
	case OPTION_PIXELS:
		/* SHARP, SMOOTH, BILINEAR, CRT, round */
		pixels = console_window_settings.crt ? 3 : console_window_settings.bilinear ? 2
			: console_window_settings.smooth ? 1 : 0;
		pixels = (pixels + step + 4) % 4;
		console_window_settings.smooth = pixels == 1;
		console_window_settings.bilinear = pixels == 2;
		console_window_settings.crt = pixels == 3;
		console_window_settings.changed = true;
		break;
	case OPTION_VOLUME:
		console_window_settings.volume = clamp(console_window_settings.volume + step, 0, VOLUME_MOST);
		console_window_settings.changed = true;
		break;
	case OPTION_SOUND_TEST:
		song_choice = (song_choice + step + SONGS) % SONGS;
		break;
	default:
		break;
	}
}

static uint32_t options_backdrop;    /* the object that drifts the faces (ranking_display_hold) */

static void open_options(void)
{
	int song;

	for (song = 0; song < SONGS; song++)    /* (SOUND TEST shows the title's song) */
		if (songs[song].sound == console_window_settings.title_music)
			song_choice = song;
	menu_open = false;
	options_open = true;
	video_open = false;
	option_choice = 0;
	option_travel = value_travel = 0;
	option_wait = value_wait = MENU_PAUSE;
	game_settings_changed = false;
	system_mode = MODE_OPTIONS;
	system_mode_step = 0u;
}

/* The page's first frame: as the ranking sets itself up (attract_ranking, start_ranking_display), but with
   no table: the NBG0 picture of Sonic faces, and the object that drifts it */
static void build_options_page(void)
{
	/* (what the calls hand back and nothing reads: a variable for each pointer, as the game's C has them,
	   since a function reads and writes through its pointers: one variable for two would mix them up) */
	uint32_t ignored = 0, ignored_2 = 0, ignored_3 = 0, display = 0, rows = 0, source = 0;
	uint8_t mode = 0u;

	set_screen_mode(mode);
	clear_text_layer(&ignored, &ignored_2, &ignored_3);
	display = clear_page_0_and_scroll();
	reset_objects(&ignored, &ignored_2);
	current_screen = 2u;
	display = allocate_object_or_start_list(display);
	OBJECT(display)->parent = object_cursor;
	OBJECT(display)->child = 0xFFFFu;
	init_object(display);
	show_nbg0_picture(&ignored, &rows, &source, &ignored_2);
	OBJECT(display)->handler = ranking_display_hold_ADDRESS;
	options_backdrop = display;
}

/* Back to the title and its menu: the game's own settings (difficulty, vitality) worked out again and kept
   in the EEPROM as the test mode keeps them */
static void close_options(void)
{
	uint32_t command = 0, words_left = 0;

	erase_options();
	options_open = false;
	menu_after_title = true;
	attract_step = 0u;
	system_mode = MODE_TITLE;
	system_mode_step = 0u;
	if (!game_settings_changed)
		return;
	apply_settings();
	free_play = 1u;
	eeprom_save_and_lock(&command, &words_left);
}

/* System mode 1, OPTIONS: its page on the first frame, then its lines; the faces never stop drifting (the
   backdrop's hold timer stays at 0) */
void system_mode_idle(void)
{
	if (system_mode_step == 0u)
	{
		system_mode_step = 1u;
		build_options_page();
	}
	OBJECT(options_backdrop)->ranking_display.hold_timer = 0;
	if (times_open)
		run_times();
	else
		run_options();
}

/* ---- CONTROLS: the actions, their keys and buttons (the port draws them, Kenney's Input Prompts) ---- */

#define CONTROLS_TITLE_ROW  2u
#define CONTROLS_FIRST_ROW  5u      /* then a line every two rows */
#define CONTROLS_NAME_COLUMN 4u
#define CONTROLS_VALUE_COLUMN 15u    /* the player's number (CONSOLE_CONTROLS_KEYS_X) */
#define CONTROLS_PLAYER     0                /* the first line, then the actions' */
#define CONTROLS_DEFAULTS   (ACTIONS + 1)    /* the lines after the actions' */
#define CONTROLS_BACK       (ACTIONS + 2)
#define CONTROLS_LINES      (ACTIONS + 3)
#define CONTROLS_MESSAGE_ROW 27u
#define CONTROLS_PLAYERS    3

static const char *const action_names[ACTIONS] = {
	"UP", "DOWN", "LEFT", "RIGHT", "ACCEPT", "BACK", "START", "JUMP"
};

static bool controls_open;
static int controls_choice;
static int controls_player;            /* whose controls are shown (0-2) */
static int start_prompt_frames[CONTROLS_PLAYERS];    /* a player's start prompt up: 2 frames from its drawing */
static int controls_waiting = -1;      /* the action waiting for its new key or button */
static bool controls_defaults_asked;

static uint32_t controls_row(int line)
{
	return CONTROLS_FIRST_ROW + 2u * (uint32_t)line;
}

static void draw_controls(void)
{
	int line;

	write_centered(CONTROLS_TITLE_ROW, "CONTROLS", MENU_CHOSEN);
	for (line = 0; line < CONTROLS_LINES; line++)
	{
		const uint8_t attribute = line == controls_choice ? MENU_CHOSEN : MENU_TEXT;

		erase_line(controls_row(line));
		if (line == CONTROLS_PLAYER)
		{
			char value[] = "< 1 >";

			value[2] = (char)('1' + controls_player);
			write_text(CONTROLS_NAME_COLUMN, controls_row(line), "PLAYER", attribute);
			write_text(CONTROLS_VALUE_COLUMN, controls_row(line), value, attribute);
		}
		else if (line <= ACTIONS)
			write_text(CONTROLS_NAME_COLUMN, controls_row(line), action_names[line - 1], attribute);
		else
			write_text(CONTROLS_NAME_COLUMN, controls_row(line), line == CONTROLS_DEFAULTS ? "DEFAULTS" : "BACK",
				attribute);
	}
	erase_line(CONTROLS_MESSAGE_ROW);
	if (controls_waiting >= 0)
		write_centered(CONTROLS_MESSAGE_ROW, "PRESS A KEY OR BUTTON", MENU_CHOSEN);
}

static void erase_controls(void)
{
	uint32_t row;

	for (row = CONTROLS_TITLE_ROW; row <= CONTROLS_MESSAGE_ROW; row++)
		erase_line(row);
}

static void open_controls(void)
{
	erase_options();
	controls_open = true;
	controls_choice = 0;
	controls_waiting = -1;
	option_wait = MENU_PAUSE;
}

static void close_controls(void)
{
	erase_controls();
	controls_open = false;
	option_wait = MENU_PAUSE;
}

/* CONTROLS: up and down choose a line; on PLAYER, left and right choose whose controls are shown; on an
   action's, accept waits for the key or button that takes its place (the port takes it, Escape leaves it as
   it was); DEFAULTS puts that player's back; BACK or the action back return to OPTIONS */
static void run_controls(void)
{
	int step = 0;

	if (controls_waiting >= 0)
	{
		draw_controls();    /* (the port is waiting for the key) */
		return;
	}
	step = cursor_step(&option_travel, &option_wait, (int16_t)trackball_1_y, ACTION_UP, ACTION_DOWN);
	controls_choice = (controls_choice + step + CONTROLS_LINES) % CONTROLS_LINES;
	if (controls_choice == CONTROLS_PLAYER)
	{
		step = cursor_step(&value_travel, &value_wait, (int16_t)trackball_1_x, ACTION_LEFT, ACTION_RIGHT);
		controls_player = (controls_player + step + CONTROLS_PLAYERS) % CONTROLS_PLAYERS;
	}
	draw_controls();
	if ((actions & ACTION_BIT(ACTION_BACK)) != 0u)
	{
		close_controls();
		return;
	}
	if (!picked())
		return;
	if (controls_choice == CONTROLS_PLAYER)
		controls_player = (controls_player + 1) % CONTROLS_PLAYERS;
	else if (controls_choice <= ACTIONS)
		controls_waiting = controls_choice - 1;
	else if (controls_choice == CONTROLS_DEFAULTS)
		controls_defaults_asked = true;
	else
		close_controls();
}

/* ---- TRACKBALL: a game on the how to play's floor (stage 7), where player 1 moves Sonic as the trackball's
   settings change (the port rolls the trackball by them, and keeps them in segasonic.ini) ---- */

enum
{
	TRACKBALL_STICK,
	TRACKBALL_KEYS,
	TRACKBALL_MOUSE,
	TRACKBALL_ROLLS_ON,
	TRACKBALL_DRAG,
	TRACKBALL_FRICTION,
	TRACKBALL_DEFAULTS,
	TRACKBALL_BACK,
	TRACKBALL_ITEMS
};

static const char *const trackball_names[TRACKBALL_ITEMS] = {
	"STICK", "KEYS", "MOUSE", "ROLLS ON", "DRAG", "FRICTION", "DEFAULTS", "BACK"
};

/* The page's lines, two a row */
static const int trackball_lines[] = {
	TRACKBALL_STICK, TRACKBALL_KEYS, TRACKBALL_MOUSE, TRACKBALL_ROLLS_ON, TRACKBALL_DRAG, TRACKBALL_FRICTION,
	TRACKBALL_DEFAULTS, TRACKBALL_BACK
};

#define TRACKBALL_LINES       ((int)(sizeof(trackball_lines) / sizeof(trackball_lines[0])))
#define TRACKBALL_TITLE_ROW   1u
#define TRACKBALL_FIRST_ROW   3u      /* then a row every two lines, */
#define TRACKBALL_HINT_ROW    (TRACKBALL_FIRST_ROW + (uint32_t)TRACKBALL_LINES / 2u + 1u)
#define TRACKBALL_NAME_COLUMN 2u      /* the left line's name and value, */
#define TRACKBALL_VALUE_COLUMN 11u
#define TRACKBALL_HALF        19u     /* and the right one's this much further */
#define ROLL_STEP             5       /* STICK and KEYS: 0 to 60 (the player is at its fastest from about 36 counts a */
#define ROLL_MOST             60      /* frame along an axis: set_speed_from_trackball) */
#define MOUSE_LEAST           0.05    /* MOUSE: as F5 and F6 change it, a fifth slower or a quarter faster */
#define MOUSE_MOST            4.0
#define DRAG_LEAST            90      /* DRAG: 0.90 to 1.00, in hundredths */
#define DRAG_MOST             100
#define FRICTION_MOST         20      /* FRICTION: 0 to 2, in tenths */
#define TRACKBALL_TIMER       0x100   /* game_mode_timer the how to play is held at: past its captions (0xA, 0xC8),
                                         short of its end (0x21C) */
#define HAND_UNLIT            0x8530u /* the trackball hand's palettes (how_to_play_trackball_hand_update), lit */
#define HAND_LIT              0x8760u /* while the ball turns */
#define HAND_RESTS            0x31    /* the game_mode_timer its update sees: short of its rub (0x32), */
#define HAND_RUBS             0x40    /* or past it */
#define BUTTON_HAND_UP        0xFFu   /* the button hand's animation frame before its step: at rest, or pressing */
#define BUTTON_HAND_DOWN      0x01u   /* (how_to_play_button_presses) */

static bool trackball_open;      /* TRACKBALL's game is on: the how to play, held */
static bool trackball_trying;    /* player 1 moves Sonic; else the lines are chosen and changed, Sonic still */
static int trackball_choice;

/* "< 30 >", "< 35% >", "< 0.98 >": 'value' with 'decimals' of its digits after a point, at least 'width'
   characters wide (spaces before), 'unit' after it */
static void arrow_number(char *text, int value, int decimals, int width, const char *unit)
{
	char digits[8];
	int count = 0, length = 0;

	do
	{
		digits[count++] = (char)('0' + value % 10);
		value /= 10;
	} while (count < 7 && (value > 0 || count <= decimals));
	text[length++] = '<';
	text[length++] = ' ';
	for (; width > count + (decimals ? 1 : 0); width--)
		text[length++] = ' ';
	while (count > 0)
	{
		if (count == decimals)
			text[length++] = '.';
		text[length++] = digits[--count];
	}
	while (*unit)
		text[length++] = *unit++;
	text[length++] = ' ';
	text[length++] = '>';
	text[length] = 0;
}

/* A line's value as the page shows it; nothing for DEFAULTS and BACK */
static void trackball_value(int line, char *text)
{
	switch (line)
	{
	case TRACKBALL_STICK:
		arrow_number(text, console_window_settings.stick_roll, 0, 2, "");
		return;
	case TRACKBALL_KEYS:
		arrow_number(text, console_window_settings.key_roll, 0, 2, "");
		return;
	case TRACKBALL_MOUSE:
		arrow_number(text, (int)lround(console_window_settings.mouse_speed * 100.0), 0, 3, "%");
		return;
	case TRACKBALL_ROLLS_ON:
		strcpy(text, console_window_settings.ball_rolls_on ? "< ON  >" : "< OFF >");
		return;
	case TRACKBALL_DRAG:
		arrow_number(text, (int)lround(console_window_settings.ball_drag * 100.0), 2, 0, "");
		return;
	case TRACKBALL_FRICTION:
		arrow_number(text, (int)lround(console_window_settings.ball_friction * 10.0), 1, 0, "");
		return;
	default:
		text[0] = 0;
		return;
	}
}

static uint32_t trackball_row(int line)
{
	return TRACKBALL_FIRST_ROW + (uint32_t)line / 2u;
}

/* The lines, the chosen one yellow (none while Sonic is tried), and what accept or back does */
static void draw_trackball(void)
{
	char value[16];
	int line;

	write_centered(TRACKBALL_TITLE_ROW, "TRACKBALL", MENU_CHOSEN);
	for (line = 0; line < TRACKBALL_LINES; line++)
	{
		const uint32_t row = trackball_row(line);
		const uint32_t half = (uint32_t)(line % 2) * TRACKBALL_HALF;
		const uint8_t attribute = !trackball_trying && line == trackball_choice ? MENU_CHOSEN : MENU_TEXT;

		if (line % 2 == 0)
			erase_line(row);
		write_text(TRACKBALL_NAME_COLUMN + half, row, trackball_names[trackball_lines[line]], attribute);
		trackball_value(trackball_lines[line], value);
		write_text(TRACKBALL_VALUE_COLUMN + half, row, value, attribute);
	}
	erase_line(TRACKBALL_HINT_ROW);
	write_centered(TRACKBALL_HINT_ROW, trackball_trying ? "BACK TO CHANGE" : "ACCEPT TO TRY", MENU_TEXT);
}

static void erase_trackball(void)
{
	uint32_t row;

	for (row = TRACKBALL_TITLE_ROW; row <= TRACKBALL_HINT_ROW; row++)
		erase_line(row);
}

/* STICK and KEYS a step of ROLL_STEP, from the multiple of it at or under the value (segasonic.ini may hold
   any) */
static int roll_step(int roll, int step)
{
	const int under = roll - roll % ROLL_STEP;

	if (step > 0)
		return clamp(under + ROLL_STEP, 0, ROLL_MOST);
	return clamp(under == roll ? under - ROLL_STEP : under, 0, ROLL_MOST);
}

static void change_trackball(int line, int step)
{
	console_window *window = &console_window_settings;

	switch (line)
	{
	case TRACKBALL_STICK:
		window->stick_roll = roll_step(window->stick_roll, step);
		break;
	case TRACKBALL_KEYS:
		window->key_roll = roll_step(window->key_roll, step);
		break;
	case TRACKBALL_MOUSE:
		window->mouse_speed *= step > 0 ? 1.25 : 0.8;
		window->mouse_speed = window->mouse_speed < MOUSE_LEAST ? MOUSE_LEAST
			: window->mouse_speed > MOUSE_MOST ? MOUSE_MOST : window->mouse_speed;
		break;
	case TRACKBALL_ROLLS_ON:
		window->ball_rolls_on = !window->ball_rolls_on;
		break;
	case TRACKBALL_DRAG:
		window->ball_drag = clamp((int)lround(window->ball_drag * 100.0) + step, DRAG_LEAST, DRAG_MOST) / 100.0;
		break;
	case TRACKBALL_FRICTION:
		window->ball_friction = clamp((int)lround(window->ball_friction * 10.0) + step, 0, FRICTION_MOST) / 10.0;
		break;
	default:
		return;
	}
	window->changed = true;
}

/* DEFAULTS: the port's own settings (settings.c) */
static void trackball_defaults(void)
{
	console_window_settings.stick_roll = 30;
	console_window_settings.key_roll = 30;
	console_window_settings.mouse_speed = 0.35;
	console_window_settings.ball_rolls_on = true;
	console_window_settings.ball_drag = 0.98;
	console_window_settings.ball_friction = 0.3;
	console_window_settings.changed = true;
}

/* A game as START GAME's, which player select gives Sonic at once (character_chooser_wait) */
static void open_trackball(void)
{
	close_options();
	menu_after_title = false;
	trackball_open = true;
	trackball_trying = false;
	trackball_choice = 0;
	option_travel = value_travel = 0;
	option_wait = value_wait = MENU_PAUSE;
	system_mode = MODE_GAME;
	game_mode = 0u;
	system_mode_step = 0u;
	game_in_progress = 0xFFu;
	game_sees_start = true;
}

/* The game ends before it began (as leave_map ends STAGE SELECT's), back to OPTIONS on TRACKBALL, with the
   title's song */
static void close_trackball(void)
{
	uint32_t write_index = 0, write_index_address = 0;
	int line;

	erase_trackball();
	trackball_open = false;
	trackball_trying = false;
	game_in_progress = 0u;
	end_game_credits();
	open_options();
	for (line = 0; line < MAIN_OPTIONS; line++)
		if (main_options[line] == OPTION_TRACKBALL)
			option_choice = line;
	queue_sound((uint8_t)console_window_settings.title_music, &write_index, &write_index_address);
}

/* TRACKBALL's frame, before the game's: no one joins; on the how to play's floor no scroll, and the lines.
   Up and down choose one, left and right change it, a pick tries them (Sonic moves: back or start stops);
   DEFAULTS puts them back, BACK (or the action back) ends the game. Sonic stays still while they change: player
   1's trackball and button are the page's then */
static void run_trackball(void)
{
	const bool was_trying = trackball_trying;
	int step = 0, line = 0;

	inputs_pressed &= ~PRESSED_STARTS_2_3;
	if (game_mode != 1u)
		return;
	scroll_mode = 0u;
	scroll_speed = 0u;
	/* (nor the how to play's walk, path 0x14: player_state_intro_turn asks for it once, the autopilot walks
	   it whatever the trackball says) */
	PLAYER(PLAYER_BLOCK(1))->scene_step_seen = 0xFFu;
	PLAYER(PLAYER_BLOCK(1))->path_request = 0u;
	PLAYER(PLAYER_BLOCK(1))->path = 0u;
	if (trackball_trying)
	{
		if ((actions & (ACTION_BIT(ACTION_BACK) | ACTION_BIT(ACTION_START))) != 0u)
		{
			trackball_trying = false;
			option_wait = value_wait = MENU_PAUSE;
		}
	}
	else
	{
		step = cursor_step(&option_travel, &option_wait, (int16_t)trackball_1_y, ACTION_UP, ACTION_DOWN);
		trackball_choice = (trackball_choice + step + TRACKBALL_LINES) % TRACKBALL_LINES;
		line = trackball_lines[trackball_choice];
		step = cursor_step(&value_travel, &value_wait, (int16_t)trackball_1_x, ACTION_LEFT, ACTION_RIGHT);
		if (step != 0)
			change_trackball(line, step);
		if ((actions & ACTION_BIT(ACTION_BACK)) != 0u || (picked() && line == TRACKBALL_BACK))
		{
			close_trackball();
			return;
		}
		if (picked() && line == TRACKBALL_DEFAULTS)
			trackball_defaults();
		else if (picked())
			trackball_trying = true;
	}
	draw_trackball();
	if (was_trying && trackball_trying)
		return;
	trackball_1_x = 0;
	trackball_1_y = 0;
	inputs_pressed &= ~PRESSED_BUTTON_1;
}

/* The how to play's hand on the button: in TRACKBALL's game it presses the button while player 1 holds theirs
   (and opens no caption) */
void how_to_play_button_hand_update(void)
{
	uint32_t depth = 0xEEu, ignored = 0;

	if (!trackball_open || stage != 7u)
	{
		original_how_to_play_button_hand_update();
		return;
	}
	OBJECT(current_object)->animation_frame = trackball_trying && (input_ports_active & PRESSED_BUTTON_1) != 0u
		? BUTTON_HAND_DOWN : BUTTON_HAND_UP;
	animate_part_list_step();
	queue_sprite_at_depth(&ignored, &depth);
}

/* The how to play's hand on the trackball: in TRACKBALL's game it rubs the ball, lit, while player 1's
   trackball turns, and opens no caption; the scroll it starts stays stopped */
void how_to_play_trackball_hand_update(void)
{
	const int16_t timer = (int16_t)game_mode_timer;
	const bool turning = trackball_1_x != 0 || trackball_1_y != 0;

	if (!trackball_open || stage != 7u)
	{
		original_how_to_play_trackball_hand_update();
		return;
	}
	OBJECT(current_object)->sprite_palette = turning ? HAND_LIT : HAND_UNLIT;
	game_mode_timer = turning ? HAND_RUBS : HAND_RESTS;
	original_how_to_play_trackball_hand_update();
	game_mode_timer = timer;
	scroll_mode = 0u;
	scroll_speed = 0u;
}

/* The options: up and down choose a line, left and right change its value; VIDEO and CONTROLS (their
   pages), TRACKBALL (its game), SOUND TEST (it plays the song, which the title plays from then on), STAGE
   SELECT and BACK are picked; the action back is BACK */
static void run_options(void)
{
	int count = 0, option, step = 0;
	const int *options = page_options(&count);

	if (controls_open)
	{
		run_controls();
		return;
	}
	step = cursor_step(&option_travel, &option_wait, (int16_t)trackball_1_y, ACTION_UP, ACTION_DOWN);
	option_choice = (option_choice + step + count) % count;
	option = options[option_choice];
	step = cursor_step(&value_travel, &value_wait, (int16_t)trackball_1_x, ACTION_LEFT, ACTION_RIGHT);
	if (step != 0)
		change_option(option, step);
	draw_options();
	if (!picked())
		return;
	if (option == OPTION_VIDEO)
		open_video();
	else if (option == OPTION_CONTROLS)
		open_controls();
	else if (option == OPTION_TRACKBALL)
		open_trackball();
	else if (option == OPTION_SOUND_TEST)
	{
		uint32_t write_index = 0, write_index_address = 0;

		queue_sound(songs[song_choice].sound, &write_index, &write_index_address);    /* (it plays the song, */
		console_window_settings.title_music = songs[song_choice].sound;    /* the title's from now on) */
		console_window_settings.changed = true;
	}
	else if (option == OPTION_BACK && video_open)
		close_video();
	else if (option == OPTION_BACK)
		close_options();
	else if (option == OPTION_STAGE_SELECT)
	{
		close_options();
		give_game_credits();
		menu_after_title = false;
		stage_select_pending = true;
		stage_select_phase = 0;
		system_mode = MODE_GAME;
		game_mode = 0u;
		system_mode_step = 0u;
		game_in_progress = 0xFFu;
		game_sees_start = true;
	}
}

/* ---- STAGE SELECT: the island map (game mode 8), a level a marker, the ones cleared this session marked ---- */

#define LEVELS            8          /* the opening (Eggman's trap: it has no marker), then the 7 markers' */
#define MARKERS           7
#define MAP_LAYOUT_SIZE   12u        /* map_marker_layouts: the markers, then their number badges */
#define MAP_NAMES         0x096C6Fu  /* map_marker_blink records: a level's name beside its marker */
#define MAP_NAME_SIZE     10u
#define MAP_SPRITE_BLOCKS 0x209102u  /* the blocks the map's markers are queued from (draw_map_stage_markers) */
#define MAP_SPRITE_BLOCK  0x30u
#define MAP_BLINK         8u         /* the chosen level's number and name blink every 8 frames */
#define MAP_EGGMAN        0x5760u    /* Eggman from behind (map_eggman_update): the opening's picture */
#define MAP_EGGMAN_X      0xB7
#define MAP_EGGMAN_Y      0xDF
#define MAP_EGGMAN_PALETTE 0x680u
#define FADED_IN          0x3Fu      /* color_offset_red at the end of the map's fade in */
#define ALL_GAME_FRAME    0x9FD0u    /* the unused ROBOTNIK'S TOWER label: "ALL GAME" (console_prepare_sprites) */
#define ALL_GAME_X        0x80       /* TIME ATTACK's first place, the whole game, over Eggman's place */
#define ALL_GAME_Y        0xA6
#define MAP_NAME_PALETTE  0x670u     /* the labels' (map_marker_blink) */

/* Each level's first stage, in the game's order (stage_order): the opening (6, Eggman's tower at night, then
   5, the cage), then the markers' (the map comes after a level, so stage_map_markers gives each stage the
   markers cleared with it). Stage 7, first in stage_order, is a test corridor: a game starts with 6 */
static const uint8_t level_first_stage[LEVELS] = { 0x06u, 0x08u, 0x09u, 0x0Au, 0x03u, 0x04u, 0x10u, 0x0Cu };

/* Each stage's level (stages 0-0x11): -1 for the ones that are not a level's (0, stage 5's switch scene, 1,
   2 and the test corridor, 7) */
static const int8_t stage_level[] = { -1, -1, -1, 4, 5, 0, 0, -1, 1, 2, 3, 7, 7, 7, 7, 7, 6, 7 };

#define ENDING_LEVEL_STAGE 0x11u  /* the ending: Eggman's tower is over */

static uint8_t levels_cleared;    /* the levels this session's games played to their end (1 << level) */
static int level_in_play = -1;    /* the level of the stage this game plays (-1: none) */

static bool map_open;
static int map_step;        /* 0 its first frame, 1 its layers on, 2 fading in, 3 choosing */
static int map_choice;
static int map_travel_x, map_wait_x, map_travel_y, map_wait_y;
static uint8_t map_blink;

/* Each frame of a game: a game goes on to another level once one is over, so the level it played before
   counts as cleared, and the ending clears Eggman's tower; a TIME ATTACK game clears none (its map starts
   clean) */
static void note_levels_cleared(void)
{
	const int level = stage < sizeof(stage_level) ? stage_level[stage] : -1;

	if (system_mode != MODE_GAME || (int8_t)game_in_progress >= 0)
	{
		level_in_play = -1;
		return;
	}
	if (level < 0)
		return;
	if (level_in_play >= 0 && (level != level_in_play || stage == ENDING_LEVEL_STAGE) && !time_attack)
		levels_cleared |= (uint8_t)(1u << level_in_play);
	level_in_play = stage == ENDING_LEVEL_STAGE ? -1 : level;
}

/* The map's first frame, as the map screen loads it (map_intro_update, step 4): the island and Eggman's
   head, then the fade in */
static void build_map(void)
{
	uint32_t dest = 0, ignored = 0, palette_list = tile_bank_0_palette_list_ADDRESS, script = 0;
	uint8_t io_cnt_value = mem_read8(io_cnt);

	mem_write8(io_cnt, io_cnt_value & ~2u);
	display_off_frames = 2u;
	nbg0_scroll_x_int = 0u;
	nbg0_scroll_y_int = 0u;
	mem_write16(video_layer_disable, mem_read16(video_layer_disable) | 0x2Fu);
	mem_write16(backdrop_color, 0u);
	mem_write32(video_control, mem_read32(video_control) & ~0x400u);
	vblank_copies_pending = 0u;
	vblank_copy_count = 0u;
	load_palette_list(palette_list, &ignored, &dest);
	script = map_monitor_script_ADDRESS;
	run_vram_script(&ignored, &script, &dest);
	script = map_eggman_head_script_ADDRESS;
	nbg2_scroll_x = 0u;
	nbg2_scroll_y = 0u;
	run_vram_script(&ignored, &script, &dest);
	nbg0_pages = 0x1010101u;
	nbg2_pages = 0u;
	fade_kind = 2;
	fade_speed = 1u;
	fade_step = 0u;
	mixer_nbg0_blend |= 0x4000u;
	mixer_nbg3_blend |= 0x4000u;
	update_fade(ignored);
}

/* A sprite of the map from a block of its own */
static void queue_map_sprite(uint32_t block, uint16_t flags, uint16_t layer, int16_t x, int16_t y, uint16_t frame,
	uint16_t palette)
{
	current_object = block;
	OBJECT(block)->sprite_flags = flags;
	OBJECT(block)->sprite_scale = 0x100u;
	OBJECT(block)->sprite_layer = layer;
	OBJECT(block)->screen_x = x;
	OBJECT(block)->screen_y = y;
	OBJECT(block)->sprite_frame = frame;
	OBJECT(block)->sprite_palette = palette;
	queue_sprite_in_layer();
}

/* The player's head (Sonic's, the ranking's portrait: about 23 x 20 pixels from its top left) where a
   level's name points on the map, past its arrow's tip: right and down of it, left and down (3), left and
   up (5 and 7) */
static const int16_t map_heads[LEVELS][2] = {
	{ 0, 0 }, { 248, 80 }, { 161, 80 }, { 171, 107 }, { 182, 65 }, { 90, 28 }, { 129, 95 }, { 51, 29 }
};

/* The markers and their numbers, a level cleared this session in its cleared colours (not in TIME ATTACK's
   map); the chosen
   level's number and name blink, or Eggman for the opening (it has neither) */
static void draw_map(void)
{
	const uint32_t object = current_object;
	uint32_t layout = map_marker_layouts_ADDRESS, block = MAP_SPRITE_BLOCKS;
	int group, marker;

	map_blink++;
	for (group = 0; group < 2; group++)
		for (marker = 0; marker < MARKERS; marker++, layout += MAP_LAYOUT_SIZE, block += MAP_SPRITE_BLOCK)
		{
			const struct map_marker_layout *entry = MAP_MARKER_LAYOUT(layout);

			if (group == 1 && marker + 1 == map_choice && (map_blink & MAP_BLINK) != 0u)
				continue;
			queue_map_sprite(block, 0xAu, entry->sprite_layer, entry->screen_x, entry->screen_y, entry->sprite_frame,
				!time_attack && (levels_cleared & (1u << (marker + 1))) != 0u ? entry->cleared_palette
					: entry->sprite_palette);
		}
	if ((map_blink & MAP_BLINK) != 0u)
		;
	else if (map_choice > 0)
	{
		const struct map_marker_blink *name =
			MAP_MARKER_BLINK(MAP_NAMES + (uint32_t)(map_choice - 1) * MAP_NAME_SIZE);

		queue_map_sprite(block, 0xAu, name->sprite_layer, name->screen_x, name->screen_y, name->sprite_frame,
			name->sprite_palette);
	}
	if (map_choice > 0)    /* (the player's head where the name's arrow points, as the map screen shows its icon) */
		queue_map_sprite(block + MAP_SPRITE_BLOCK, 0xAu, 0u, map_heads[map_choice][0], map_heads[map_choice][1],
			ROM16(ranking_portrait_frames[0]), ranking_portrait_palettes[0]);
	else if (time_attack)
		queue_map_sprite(block, 0xAu, 0u, ALL_GAME_X, ALL_GAME_Y, ALL_GAME_FRAME, MAP_NAME_PALETTE);    /* (the whole game) */
	else
		queue_map_sprite(block, 0x107u, 1u, MAP_EGGMAN_X, MAP_EGGMAN_Y, MAP_EGGMAN, MAP_EGGMAN_PALETTE);
	current_object = object;
}

/* Where the map shows each choice, in the picture's pixels: the opening's (and TIME ATTACK's whole game)
   at Eggman's console, then the 7 markers (map_marker_layouts' places are not where they show) */
static const int16_t map_places[LEVELS][2] = {
	{ 160, 170 }, { 266, 45 }, { 147, 98 }, { 194, 129 }, { 202, 37 }, { 131, 26 }, { 82, 130 }, { 51, 41 }
};

static void map_place(int choice, int *x, int *y)
{
	*x = map_places[choice][0];
	*y = map_places[choice][1];
}

/* Where the cursor goes from map_choice one way (a step across or down, -1 or 1): the nearest place that
   way on the map, a place off to the side counting three times; it stays when there is none (the levels'
   order runs right to left on the island: 1 is the volcano's) */
static int map_choice_toward(int step_x, int step_y)
{
	int from_x = 0, from_y = 0, choice, best = map_choice, best_score = 0;

	map_place(map_choice, &from_x, &from_y);
	for (choice = 0; choice < LEVELS; choice++)
	{
		int x = 0, y = 0, along, aside;

		map_place(choice, &x, &y);
		along = (x - from_x) * step_x + (y - from_y) * step_y;
		aside = step_x != 0 ? y - from_y : x - from_x;
		aside = aside < 0 ? -aside : aside;
		if (choice == map_choice || along <= 0)
			continue;
		if (best == map_choice || along + 3 * aside < best_score)
		{
			best = choice;
			best_score = along + 3 * aside;
		}
	}
	return best;
}

/* Back on the map: the game ends before it began (as game over ends one), back to OPTIONS */
static void leave_map(void)
{
	map_open = false;
	stage_select_pending = false;
	game_in_progress = 0u;
	end_game_credits();
	stage_init_flags &= ~0x8040u;
	mixer_text_blend &= 0xBFFFu;
	mixer_nbg0_blend &= 0xBFFFu;
	mixer_nbg1_blend &= 0xBFFFu;
	mixer_nbg2_blend &= 0xBFFFu;
	mixer_nbg3_blend &= 0xBFFFu;
	mixer_bitmap_blend &= 0xBFFFu;
	if (!time_attack)
	{
		open_options();
		return;
	}
	time_attack = false;    /* (TIME ATTACK's map: back to the title and its menu) */
	menu_after_title = true;
	attract_step = 0u;
	system_mode = MODE_TITLE;
	system_mode_step = 0u;
}

/* Game mode 8, STAGE SELECT: the island map (as the map screen fades it in), then left and right (or up and
   down) go through every level (the cursor starts on the last one this session reached), and a pick starts the chosen one's first stage as the original's
   stage select does (cleared_stages from stage_select_cleared_values, game mode 2, stage_start_kind 1) */
void game_mode_stage_select(uint32_t *p_count, uint32_t *p_list_cursor)
{
	int step = 0;

	(void)p_count;
	(void)p_list_cursor;
	if ((stage_init_flags & 0x40u) == 0u)
	{
		stage_init_flags |= 0x40u;
		map_open = true;
		map_step = 0;
		map_choice = 0;    /* (the level after the last one cleared; TIME ATTACK's map from Eggman's place) */
		for (int level = LEVELS - 1; level >= 0 && !time_attack; level--)
			if ((levels_cleared & (1u << level)) != 0u)
			{
				map_choice = clamp(level + 1, 0, LEVELS - 1);
				break;
			}
		map_travel_x = map_travel_y = 0;
		map_wait_x = map_wait_y = MENU_PAUSE;
	}
	switch (map_step)
	{
	case 0:
		build_map();
		map_step++;
		return;
	case 1:
		/* (NBG0, the island and the console; not NBG2, Eggman's head, which the map screen slides in with a
		   row scroll: one a game left drew it across the console) */
		mem_write16(video_layer_disable, mem_read16(video_layer_disable) | 0xEu);
		mem_write16(video_layer_disable, mem_read16(video_layer_disable) & ~1u);
		map_step++;
		return;
	case 2:
		update_fade(0u);
		draw_map();
		if (color_offset_red != FADED_IN)
			return;
		mixer_nbg0_blend &= 0xBFFFu;
		mixer_nbg1_blend &= 0xBFFFu;
		map_step++;
		return;
	default:
		break;
	}
	step = cursor_step(&map_travel_x, &map_wait_x, (int16_t)trackball_1_x, ACTION_LEFT, ACTION_RIGHT);
	if (step != 0)
		map_choice = map_choice_toward(step, 0);
	step = cursor_step(&map_travel_y, &map_wait_y, (int16_t)trackball_1_y, ACTION_UP, ACTION_DOWN);
	if (step != 0)
		map_choice = map_choice_toward(0, step);
	draw_map();
	if (!picked())
		return;
	map_open = false;
	stage_select_stage = level_first_stage[map_choice];
	if (time_attack)
		time_attack_level = map_choice;
	stage = 7u;    /* (player select's, as in a game the menu starts: the chosen stage's objects would run there) */
	stage_init_flags &= ~0x40u;
	game_mode = 0x1Bu;
	stage_select_phase = 2;
}

/* ---- WIDE: the stage's map at the sides, where the board's camera never looks ---- */

#define VIDEO_REGISTERS    (VIDEO_RAM + 0x1FF00u)
#define BOARD_WIDTH        320
#define BOARD_HEIGHT       224
#define CELL_PIXELS        16
#define LAYER_CELLS_X      64       /* a tilemap: 2 x 2 pages of 32 x 16 cells, 1024 x 512 pixels */
#define LAYER_CELLS_Y      32
#define PAGE_CELLS_X       32
#define PAGE_CELLS_Y       16
#define MAP_LAYERS         4        /* NBG0 and NBG1 draw the stage's map; NBG2 and NBG3 its backgrounds, or
                                       some of it (the ice stage's cliffs) */
#define NORMAL_ZOOM        0x200
#define PERIOD_MOST        8        /* the widest repeat of a row's cells a side goes on with */
#define NO_MIRROR          (-0x7FFF)
#define TILE_SET_SPAN      0x40     /* how near two tiles' numbers are in one set */
#define REPEAT_EVIDENCE    2        /* the fewest cells a row's repeat holds for, to be taken */
#define CELL_FLIP_X        0x4000u  /* a tilemap cell's bit: its tile drawn flipped across */
#define REPEATS            32       /* the repeats tried: 1 to 8 cells across, along the slopes, and up or down */
#define STRIP_ROWS         16       /* a side's cells: the picture's rows (15 at most) */
#define STRIP_CELLS        4        /* and the columns its 48 pixels touch */
#define FILLED_MOST        1024
#define BLOCK_PIXELS_X     512      /* stage_block_maps' blocks */
#define BLOCK_PIXELS_Y     256
#define SCROLL_TRAIL       64       /* the most a map layer's scroll is from the camera */
#define SECTIONS_END       0xFFFFu  /* stage_section_ends past a stage's last section */
#define SECTIONS_MOST      64u
#define BEYOND_WORDS       16       /* the most words beyond_word counts */
#define WINDOW_CELLS_X     (2 * BLOCK_PIXELS_X / CELL_PIXELS)    /* the map's blocks the pages hold */
#define WINDOW_CELLS_Y     (2 * BLOCK_PIXELS_Y / CELL_PIXELS)

typedef struct
{
	int pages[4];
	int base_x, base_y;    /* the layer's pixel at the board's picture's top left, unwrapped (from 1024 on) */
	int map_x, map_y;      /* the map's there: the camera's */
} map_layer;

static uint16_t own_words[S32_VIDEO_CELLS];   /* the cells the frame draws its own way (s32_video_set_cells) */
static uint8_t own_marks[S32_VIDEO_CELLS];
static uint32_t own_cells[FILLED_MOST];       /* which, to unmark after it */
static int filled_count;

static uint32_t cell_index(uint32_t cell)
{
	return (cell - VIDEO_RAM) / 2u;
}

/* A cell's word as the frame draws it: its own, else video RAM's */
static uint16_t shown_word(uint32_t cell)
{
	const uint32_t index = cell_index(cell);

	return own_marks[index] ? own_words[index] : mem_read16(cell);
}

/* A cell that draws nothing (the stages leave some blank with a tile of no pens) */
static bool cell_blank(uint32_t cell)
{
	return s32_video_cell_blank(shown_word(cell));
}

static uint16_t video_register_word(uint32_t offset)
{
	return mem_read16(VIDEO_REGISTERS + offset);
}

/* A cell of the layer by its unwrapped column and row */
static uint32_t layer_cell(const map_layer *layer, int column, int row)
{
	const unsigned x = (unsigned)column & (LAYER_CELLS_X - 1u), y = (unsigned)row & (LAYER_CELLS_Y - 1u);
	const unsigned page = (unsigned)layer->pages[(y / PAGE_CELLS_Y) * 2u + x / PAGE_CELLS_X];

	return VIDEO_RAM + ((page * PAGE_CELLS_X * PAGE_CELLS_Y) + (y % PAGE_CELLS_Y) * PAGE_CELLS_X + x % PAGE_CELLS_X) * 2u;
}

/* Narrows [low, high] to the t with a + d t < k; false when none is left */
static bool narrow(double a, double d, double k, double *low, double *high)
{
	if (d == 0.0)
		return a < k;
	if (d > 0.0)
		*high = (k - a) / d < *high ? (k - a) / d : *high;
	else
		*low = (k - a) / d > *low ? (k - a) / d : *low;
	return *low < *high;
}

/* The board's picture, from the camera somewhere on the straight run from a to a + d (high 1) or past it
   (high HUGE_VAL), shows some of the map's cell at x, y */
static bool run_shows(double ax, double ay, double dx, double dy, double high, int x, int y)
{
	double low = 0.0;

	return narrow(ax, dx, x + CELL_PIXELS, &low, &high) && narrow(-ax, -dx, BOARD_WIDTH - x, &low, &high)
		&& narrow(ay, dy, y + CELL_PIXELS, &low, &high) && narrow(-ay, -dy, BOARD_HEIGHT - y, &low, &high);
}

/* The board's picture shows some of the map's cell at x, y from somewhere on the stage's camera path: each
   section from its start (stage_section_limits_x/_y, from camera_origin_x/_y, as load_camera_start_section
   puts the camera) straight to the next's; the last (the next section's end, stage_section_ends, is
   SECTIONS_END) has no limit: its direction (stage_section_directions_x/_y) on and on */
static bool cell_ever_shown(int x, int y)
{
	const uint32_t xs = stage_section_limits_x_ADDRESS + stage_section_limits_x[stage];
	const uint32_t ys = stage_section_limits_y_ADDRESS + stage_section_limits_y[stage];
	const uint32_t ends = stage_section_ends_ADDRESS + stage_section_ends[stage];
	uint32_t section;

	for (section = 0; section < SECTIONS_MOST; section++)
	{
		const double ax = (uint16_t)(ROM16(xs + 2u * section) + camera_origin_x);
		const double ay = (uint16_t)(ROM16(ys + 2u * section) + camera_origin_y);

		if (ROM16(ends + 2u * section + 2u) == SECTIONS_END)
			return run_shows(ax, ay,
				(int16_t)ROM16(stage_section_directions_x_ADDRESS + stage_section_directions_x[stage] + 2u * section),
				(int16_t)ROM16(stage_section_directions_y_ADDRESS + stage_section_directions_y[stage] + 2u * section),
				HUGE_VAL, x, y);
		if (run_shows(ax, ay, (uint16_t)(ROM16(xs + 2u * section + 2u) + camera_origin_x) - ax,
				(uint16_t)(ROM16(ys + 2u * section + 2u) + camera_origin_y) - ay, 1.0, x, y))
			return true;
	}
	return false;
}

/* A cell of the map's 2 x 2 blocks the game has in the pages (update_stage_tile_layers, from the camera:
   2 blocks of 512 pixels across from the wide picture's left edge, 2 of 256 down from its top's): the
   pages hold nothing else of the map, past them other blocks are in their place */
static int loaded_left(const map_layer *layer)
{
	return (layer->map_x >= S32_VIDEO_WIDE_SIDE ? layer->map_x - S32_VIDEO_WIDE_SIDE : layer->map_x) & ~(BLOCK_PIXELS_X - 1);
}

static int loaded_top(const map_layer *layer)
{
	return layer->map_y & ~(BLOCK_PIXELS_Y - 1);
}

static bool cell_loaded(const map_layer *layer, int column, int row)
{
	const int x = column * CELL_PIXELS - layer->base_x + layer->map_x;
	const int y = row * CELL_PIXELS - layer->base_y + layer->map_y;
	const int left = loaded_left(layer), top = loaded_top(layer);

	return x >= left && x < left + 2 * BLOCK_PIXELS_X && y >= top && y < top + 2 * BLOCK_PIXELS_Y;
}

/* A cell within the map's width (stage_block_map_width blocks of 512 pixels from x 0): past it the pages
   hold another end of the map, by their wrap */
static bool cell_in_map(const map_layer *layer, int column)
{
	const int x = column * CELL_PIXELS - layer->base_x + layer->map_x;

	return x >= 0 && x < (int)stage_block_map_width * BLOCK_PIXELS_X;
}

/* A cell the board's picture never shows (nor this frame's) */
static bool cell_hidden(const map_layer *layer, int column, int row)
{
	const int x = column * CELL_PIXELS - layer->base_x + layer->map_x;
	const int y = row * CELL_PIXELS - layer->base_y + layer->map_y;

	return (x >= layer->map_x + BOARD_WIDTH || x + CELL_PIXELS <= layer->map_x) && !cell_ever_shown(x, y);
}

/* The picture's sides on a layer: each side's cells (its first column, and the step outward: right 1, left
   -1), hidden or not, and which hidden ones the frame has drawn its own way so far; and, once a cell past
   the map's width needs them (beyond_word), the loaded blocks' cells as the map holds them, the
   columns within the map's width from 'first' to 'end' */
enum { CELL_SHOWN, CELL_HIDDEN, CELL_DRAWN };

typedef struct
{
	const map_layer *layer;
	int top_row, rows;
	int first[2], step[2];
	uint8_t state[STRIP_ROWS][2][STRIP_CELLS];
	bool window_read;
	int window_first, window_end;
	uint16_t window[WINDOW_CELLS_Y][WINDOW_CELLS_X];
} side_strips;

/* A repeat: a cell takes the one 'across' cells toward the picture, and up (-1) or down (1) half as many
   along the isometric slopes; or the one so many rows up or down (a wall's bricks) */
static const int8_t repeats[REPEATS][2] = {
	{ 1, 0 }, { 2, 0 }, { 2, -1 }, { 2, 1 }, { 3, 0 }, { 4, 0 }, { 4, -2 }, { 4, 2 },
	{ 5, 0 }, { 6, 0 }, { 6, -3 }, { 6, 3 }, { 7, 0 }, { 8, 0 }, { 8, -4 }, { 8, 4 },
	{ 0, -1 }, { 0, 1 }, { 0, -2 }, { 0, 2 }, { 0, -3 }, { 0, 3 }, { 0, -4 }, { 0, 4 },
	{ 0, -5 }, { 0, 5 }, { 0, -6 }, { 0, 6 }, { 0, -7 }, { 0, 7 }, { 0, -8 }, { 0, 8 }
};

static uint8_t *strip_state(side_strips *strips, int column, int row)
{
	int side, cell;

	if (row < strips->top_row || row >= strips->top_row + strips->rows)
		return NULL;
	for (side = 0; side < 2; side++)
	{
		cell = (column - strips->first[side]) * strips->step[side];
		if (cell >= 0 && cell < STRIP_CELLS)
			return &strips->state[row - strips->top_row][side][cell];
	}
	return NULL;
}

/* A cell whose word is what the frame shows: not hidden, or drawn already */
static bool cell_known(side_strips *strips, int column, int row)
{
	const uint8_t *state = strip_state(strips, column, row);

	if (!cell_loaded(strips->layer, column, row))
		return false;
	if (state ? *state != CELL_HIDDEN : !cell_hidden(strips->layer, column, row))
		return true;
	return cell_in_map(strips->layer, column)
		&& !s32_video_cell_blank(mem_read16(layer_cell(strips->layer, column, row)));    /* (the map's own there) */
}

/* Two cells look alike: the same word, or both blank */
static bool cells_alike(uint32_t one, uint32_t other)
{
	return shown_word(one) == shown_word(other) || (cell_blank(one) && cell_blank(other));
}

/* The repeats of a row's side, best first, and their scores: how many drawn cells each holds for, where
   the row meets the side (from the cell at 'from' toward the picture, with the rows above and below) */
static void rank_repeats(const side_strips *strips, int row, int from, int step, int *ranked, int *scores)
{
	const map_layer *layer = strips->layer;
	int repeat, line, cell, index;

	for (repeat = 0; repeat < REPEATS; repeat++)
	{
		scores[repeat] = 0;
		for (line = row - 1; line <= row + 1; line++)
			for (cell = 0; cell < PERIOD_MOST; cell++)
			{
				if (!cell_loaded(layer, from - step * cell, line)
					|| !cell_loaded(layer, from - step * (cell + repeats[repeat][0]), line + repeats[repeat][1]))
					continue;
				const uint32_t here = layer_cell(layer, from - step * cell, line);

				scores[repeat] += !cell_blank(here) && shown_word(here)
					== shown_word(layer_cell(layer, from - step * (cell + repeats[repeat][0]), line + repeats[repeat][1]));
			}
		for (index = repeat; index > 0 && scores[ranked[index - 1]] < scores[repeat]; index--)
			ranked[index] = ranked[index - 1];
		ranked[index] = repeat;
	}
}

/* Two cells' tiles from the same set (numbers near each other): a structure's own, not a piece of
   somewhere else */
static bool same_tile_set(uint16_t one, uint16_t other)
{
	const int distance = (int)(one & 0x1FFFu) - (int)(other & 0x1FFFu);

	return distance > -TILE_SET_SPAN && distance < TILE_SET_SPAN;
}

/* What a hidden cell shows, by its repeats (repeat_word, false when none holds): its own when it draws a
   tile of the same set as the cell beside it toward the picture; else the cell its row's best repeat takes that holds for the cell beside it toward
   the picture, and for the ones above it and under the one beside it when those are known; else its own when it draws, blank beside a
   blank one, an even fill beside or above it, or the texture beside it mirrored (about the row's *mirror,
   NO_MIRROR until a cell of the row takes it) */
static bool repeat_word(side_strips *strips, int column, int row, int step, const int *ranked, const int *scores,
	uint16_t *word)
{
	const map_layer *layer = strips->layer;
	const int inner = column - step;
	const bool above_known = cell_known(strips, column, row - 1);
	const uint16_t own = mem_read16(layer_cell(layer, column, row));
	const uint16_t beside = shown_word(layer_cell(layer, inner, row));
	int index;

	*word = own;
	if (cell_in_map(layer, column) && !s32_video_cell_blank(own) && !s32_video_cell_blank(beside)
		&& same_tile_set(own, beside))
		return true;    /* (what the map holds goes on with the cell beside it: the designer's own) */
	if (!cell_in_map(layer, inner) && !cell_known(strips, inner, row))
		return false;    /* (the cell beside it, past the map's width, isn't drawn yet: nothing to match) */
	for (index = 0; index < REPEATS && scores[ranked[index]] >= REPEAT_EVIDENCE; index++)
	{
		const int across = -step * repeats[ranked[index]][0];
		const int down = repeats[ranked[index]][1];

		if (!cell_known(strips, column + across, row + down) || !cell_known(strips, inner + across, row + down)
			|| !cells_alike(layer_cell(layer, inner, row), layer_cell(layer, inner + across, row + down)))
			continue;
		if (above_known && (!cell_known(strips, column + across, row - 1 + down)
				|| !cells_alike(layer_cell(layer, column, row - 1), layer_cell(layer, column + across, row - 1 + down))))
			continue;
		if (!cell_hidden(layer, inner, row + 1) && cell_loaded(layer, inner, row + 1)
			&& (!cell_known(strips, inner + across, row + 1 + down)
				|| !cells_alike(layer_cell(layer, inner, row + 1), layer_cell(layer, inner + across, row + 1 + down))))
			continue;    /* (and the one under the cell beside it, shown: a floor's corner ends there) */
		*word = shown_word(layer_cell(layer, column + across, row + down));
		return true;
	}
	return false;
}

/* The loaded blocks' cells within the map's width, as the map holds them (once a layer's frame) */
static void read_window(side_strips *strips)
{
	const map_layer *layer = strips->layer;
	const int left = loaded_left(layer), top = loaded_top(layer);
	const int column = (left + layer->base_x - layer->map_x) / CELL_PIXELS;
	const int row = (top + layer->base_y - layer->map_y) / CELL_PIXELS;
	const int end = ((int)stage_block_map_width * BLOCK_PIXELS_X - left) / CELL_PIXELS;
	int line, cell;

	if (strips->window_read)
		return;
	strips->window_read = true;
	strips->window_first = left < 0 ? -left / CELL_PIXELS : 0;
	strips->window_end = end < WINDOW_CELLS_X ? (end > 0 ? end : 0) : WINDOW_CELLS_X;
	for (line = 0; line < WINDOW_CELLS_Y; line++)
		for (cell = strips->window_first; cell < strips->window_end; cell++)
			strips->window[line][cell] = mem_read16(layer_cell(layer, column + cell, row + line));
}

/* A cell past the map's width (the pages hold another end of it there): what the map draws next where it
   holds the same run as the row's cells toward the picture, up to the map's edge (the most often so, of the
   ones that draw); else the same for a shorter run of them; nothing beside a blank one, or when the map never
   holds even the cell beside it: a wall's bricks go on as the designer laid them */
static uint16_t beyond_word(side_strips *strips, int column, int row, int step)
{
	const map_layer *layer = strips->layer;
	const int x = column * CELL_PIXELS - layer->base_x + layer->map_x;
	const int beyond = (step < 0 ? -x : x - (int)stage_block_map_width * BLOCK_PIXELS_X) / CELL_PIXELS + (step > 0);
	uint16_t keys[STRIP_CELLS + 1];
	uint16_t words[BEYOND_WORDS];
	int counts[BEYOND_WORDS];
	int run, found, best, line, cell, index;

	if (beyond < 1 || cell_blank(layer_cell(layer, column - step, row)))
		return 0u;    /* (a structure ends there) */
	read_window(strips);
	for (index = 1; index <= beyond && index <= STRIP_CELLS; index++)
		keys[index] = shown_word(layer_cell(layer, column - step * index, row));
	for (run = beyond < STRIP_CELLS ? beyond : STRIP_CELLS; run > 0; run--)
	{
		found = 0;
		best = -1;
		for (line = 0; line < WINDOW_CELLS_Y; line++)
			for (cell = strips->window_first; cell < strips->window_end; cell++)
			{
				for (index = 1; index <= run; index++)
				{
					const int other = cell - step * index;

					if (other < strips->window_first || other >= strips->window_end
						|| strips->window[line][other] != keys[index])
						break;
				}
				if (index <= run || s32_video_cell_blank(strips->window[line][cell]))
					continue;    /* (where the run ends in the map, it ends: past the map it goes on) */
				for (index = 0; index < found && words[index] != strips->window[line][cell]; index++)
					;
				if (index == found && found == BEYOND_WORDS)
					continue;
				if (index == found)
				{
					words[found] = strips->window[line][cell];
					counts[found++] = 0;
				}
				counts[index]++;
				if (best < 0 || counts[index] > counts[best])
					best = index;
			}
		if (best >= 0)
			return words[best];
	}
	return 0u;
}

static uint16_t hidden_word(side_strips *strips, int column, int row, int step, const int *ranked, const int *scores,
	int *mirror)
{
	const map_layer *layer = strips->layer;
	const int inner = column - step;
	const bool above_known = cell_known(strips, column, row - 1);
	const uint16_t own = mem_read16(layer_cell(layer, column, row));
	uint16_t word = 0;

	if (repeat_word(strips, column, row, step, ranked, scores, &word))
		return word;
	if (!cell_in_map(layer, column))
		return beyond_word(strips, column, row, step);    /* (past the map's width, a repeat or what it draws so) */
	/* (none: the row is one of a kind there, a picture, a texture with no repeat or a structure's end the
	   board shows. What the map holds there, when it draws: the picture goes on. Else nothing where the
	   cell beside it is blank (structures end); an even fill beside or above it goes on (the cell beside
	   it as the one over that, or two alike above: lava past a floor's end); else, where the map is cut
	   straight across (the cell beside it drawing above and under too), the texture beside it mirrored,
	   about the row's first cell so taken (flipped across: rocks at the map's edge, whose top goes on with
	   no step); else nothing (a structure's corner ends there)) */
	if (!s32_video_cell_blank(mem_read16(layer_cell(layer, column, row))))
		return mem_read16(layer_cell(layer, column, row));
	if (cell_blank(layer_cell(layer, inner, row)))
		return shown_word(layer_cell(layer, inner, row));
	if (cell_known(strips, inner, row - 1)
		&& shown_word(layer_cell(layer, inner, row - 1)) == shown_word(layer_cell(layer, inner, row)))
		return shown_word(layer_cell(layer, inner, row));
	if (above_known && !cell_blank(layer_cell(layer, column, row - 1)) && (!cell_known(strips, column, row - 2)
			|| shown_word(layer_cell(layer, column, row - 2)) == shown_word(layer_cell(layer, column, row - 1))))
		return shown_word(layer_cell(layer, column, row - 1));
	if (cell_blank(layer_cell(layer, inner, row - 1)) || cell_blank(layer_cell(layer, inner, row + 1)))
		return own;    /* (a structure's edge or corner, not the map cut across it: it ends there) */
	if (*mirror == NO_MIRROR)
		*mirror = column;
	{
		const int source = *mirror - step * ((column - *mirror) * step + 1);

		if (cell_known(strips, source, row))
			return (uint16_t)(shown_word(layer_cell(layer, source, row)) ^ CELL_FLIP_X);
	}
	return shown_word(layer_cell(layer, inner, row));
}

/* A layer's sides, twice row by row from the top: each side's hidden cells outward take their repeats
   (repeat_word), then the ones left hidden_word, as the frame's own cells (kept, to unmark after it) */
static void fill_sides(const map_layer *layer)
{
	side_strips strips;
	int ranked[REPEATS], scores[REPEATS];
	int pass, row, side, cell, first_hidden, mirror;

	memset(&strips, 0, sizeof(strips));
	strips.layer = layer;
	strips.top_row = layer->base_y / CELL_PIXELS;
	strips.rows = (layer->base_y + BOARD_HEIGHT - 1) / CELL_PIXELS - strips.top_row + 1;
	strips.first[0] = (layer->base_x + BOARD_WIDTH) / CELL_PIXELS;
	strips.step[0] = 1;
	strips.first[1] = (layer->base_x - 1) / CELL_PIXELS;
	strips.step[1] = -1;
	for (row = 0; row < strips.rows; row++)
		for (side = 0; side < 2; side++)
			for (cell = 0; cell < STRIP_CELLS; cell++)
				strips.state[row][side][cell] = cell_hidden(layer, strips.first[side] + strips.step[side] * cell,
					strips.top_row + row) ? CELL_HIDDEN : CELL_SHOWN;
	for (pass = 0; pass < 2; pass++)
	for (row = 0; row < strips.rows; row++)
		for (side = 0; side < 2; side++)
		{
			const int step = strips.step[side];

			for (first_hidden = 0; first_hidden < STRIP_CELLS && strips.state[row][side][first_hidden] != CELL_HIDDEN;
				first_hidden++)
				;
			if (first_hidden == STRIP_CELLS)
				continue;
			rank_repeats(&strips, strips.top_row + row, strips.first[side] + step * (first_hidden - 1), step, ranked,
				scores);
			mirror = NO_MIRROR;
			for (cell = first_hidden; cell < STRIP_CELLS && filled_count < FILLED_MOST; cell++)
			{
				const int column = strips.first[side] + step * cell;
				const uint32_t address = layer_cell(layer, column, strips.top_row + row);
				uint16_t word = 0;

				if (strips.state[row][side][cell] != CELL_HIDDEN)
					continue;
				if (pass == 0 && !repeat_word(&strips, column, strips.top_row + row, step, ranked, scores, &word))
					continue;    /* (the first pass: repeats alone, so a row can take from the rows under it) */
				if (pass == 1)
					word = hidden_word(&strips, column, strips.top_row + row, step, ranked, scores, &mirror);
				if (s32_video_cell_blank(word) && !s32_video_cell_blank(mem_read16(address)) && cell_in_map(layer, column))
					word = mem_read16(address);    /* (never a hole where the map draws: a background's) */
				own_words[cell_index(address)] = word;
				own_marks[cell_index(address)] = 1u;
				own_cells[filled_count++] = cell_index(address);
				strips.state[row][side][cell] = CELL_DRAWN;
			}
		}
}

/* Tilemap 'number' is on (registers 0x02 and 0x8E; 0x00 bits 12-13 for NBG2-3) */
static bool tilemap_on(int number)
{
	return (video_register_word(0x02u) & (1u << number)) == 0u && (video_register_word(0x8Eu) & (2u << number)) == 0u
		&& (number < 2 || (video_register_word(0x00u) & (1u << (10 + number))) == 0u);
}

/* Tilemap 'number' shows the stage's map: on, unflipped, scrolling with the camera; NBG0 and NBG1
   unzoomed, NBG2 and NBG3 (backgrounds, or parts of the map) with no line table and as the map layer
   'map' does exactly */
static bool map_layer_of(int number, map_layer *layer, const map_layer *map)
{
	const uint16_t control = video_register_word(0x00u);
	const uint16_t upper = video_register_word(0x40u + 4u * (uint32_t)number);
	const uint16_t lower = video_register_word(0x42u + 4u * (uint32_t)number);
	const uint16_t line_control = video_register_word(0x04u);
	const int scroll_x = video_register_word(0x12u + 8u * (uint32_t)number) & 0x3FF;
	const int scroll_y = video_register_word(0x16u + 8u * (uint32_t)number) & 0x1FF;
	const uint16_t center_x_word = video_register_word(0x30u + 4u * (uint32_t)number);
	int center_x = (int)((center_x_word & 0x1FFu) ^ 0x100u) - 0x100;
	int center_y = (int)((video_register_word(0x32u + 4u * (uint32_t)number) & 0x1FFu) ^ 0x100u) - 0x100;
	int offset_x = 0, offset_y = 0;

	if (!tilemap_on(number) || (control & (0x0200u | (1u << number))) != 0u)
		return false;
	if (number < 2 && ((video_register_word(0x50u + 4u * (uint32_t)number) & 0xFFFu) != NORMAL_ZOOM
			|| ((control & 0x4000u) != 0u && (video_register_word(0x52u + 4u * (uint32_t)number) & 0xFFFu) != NORMAL_ZOOM)))
		return false;
	if (number >= 2)
	{
		if ((line_control & ((1u << (number - 2)) | (1u << number) | (1u << (number + 2)))) != 0u || map == NULL)
			return false;
		center_x = center_x_word & 0x1FF;    /* (as draw_scrolled takes it: no sign, no y) */
		center_y = 0;
	}
	layer->pages[0] = upper & 0x7F;
	layer->pages[1] = (upper >> 8) & 0x7F;
	layer->pages[2] = lower & 0x7F;
	layer->pages[3] = (lower >> 8) & 0x7F;
	layer->base_x = ((scroll_x - center_x) & 0x3FF) + 1024;
	layer->base_y = ((scroll_y - center_y) & 0x1FF) + 1024;
	if (number >= 2)
	{
		layer->map_x = map->map_x;
		layer->map_y = map->map_y;
		return layer->base_x == map->base_x && layer->base_y == map->base_y;
	}
	/* (the stage's blocks go in the pages by their place in the map: a layer's pixel is the map's at the
	   same place, wrapped; its scroll can trail the camera a little) */
	offset_x = ((layer->base_x - (uint16_t)camera_x_pixels + 512) & 0x3FF) - 512;
	offset_y = ((layer->base_y - (uint16_t)camera_y_pixels + 256) & 0x1FF) - 256;
	layer->map_x = (uint16_t)camera_x_pixels + offset_x;
	layer->map_y = (uint16_t)camera_y_pixels + offset_y;
	return offset_x > -SCROLL_TRAIL && offset_x < SCROLL_TRAIL && offset_y > -SCROLL_TRAIL && offset_y < SCROLL_TRAIL;
}

/* A stage drawn, in any mode of a game or a demo (played, its events, scenes and transitions), but stage
   6, the opening's scenes (pictures 320 pixels wide), and 5, its cage room (the camera stays, its walls
   end past the 320 pixels; its switch scene is stage 0); not the island maps (game modes 4 and 8): their
   pictures end at their sides */
static bool stage_drawn(void)
{
	return stage != 6u && stage != 5u && game_mode != 4u && game_mode != 8u
		&& ((system_mode == MODE_GAME && (int8_t)game_in_progress < 0) || system_mode == MODE_DEMO_1
			|| system_mode == MODE_DEMO_2);
}

/* A stage's map on screen, wider, its sides drawn their own way: a stage drawn but the test corridor (the how
   to play's, player select's), the ending and stage 0, the cage room's switch scene (the tower's platform
   in the sky, the fall by the waterfall): its camera is the scene's, not the stage's sections, and the map
   holds its sides as they go on, the tower whole */
static bool stage_map_shown(void)
{
	return console_window_settings.wide && stage_drawn() && stage < 0x11u && stage != 7u && stage != 0u;
}

#define VIDEO_PAGES        128
#define PAGE_BYTES         0x400u
#define BITMAP_OFF         0x0020u    /* register 0x02: the bitmap layer off */

/* The bitmap layer on shows video RAM from its start, 512 pixels a row of 4 or 8 bits (register 0x00 bit
   11), from row register 0x8A on: true when the rows on screen hold pages the tilemaps that are on or the
   text layer read (its map and characters, register 0x5C), which it shows as dots and dashes. The
   original leaves it so in some stages (the ice and tower ones: rows of dots where the stage is dark) */
static bool bitmap_shows_pages(void)
{
	bool used[VIDEO_PAGES] = { false };
	const uint16_t control = video_register_word(0x00u);
	const uint16_t off = video_register_word(0x02u);
	const uint16_t more_off = video_register_word(0x8Eu);
	const uint16_t text = video_register_word(0x5Cu);
	const bool bits8 = (control & 0x0800u) != 0u;
	const int y_scroll = video_register_word(0x8Au) & 0x1FF;
	int number, row, page;

	if ((off & BITMAP_OFF) != 0u || (more_off & 0x0020u) != 0u)
		return false;
	for (number = 0; number < 4; number++)
	{
		const uint16_t upper = video_register_word(0x40u + 4u * (uint32_t)number);
		const uint16_t lower = video_register_word(0x42u + 4u * (uint32_t)number);

		if ((off & (1u << number)) != 0u || (more_off & (2u << number)) != 0u
			|| (number >= 2 && (control & (1u << (10 + number))) != 0u))
			continue;
		used[upper & 0x7F] = used[(upper >> 8) & 0x7F] = used[lower & 0x7F] = used[(lower >> 8) & 0x7F] = true;
	}
	if ((off & 0x0010u) == 0u && (more_off & 0x0001u) == 0u)
	{
		for (page = 0; page < 4; page++)
			used[(((text >> 4) & 0x1F) * 4 + page) & 0x7F] = true;
		for (page = 0; page < 16; page++)
			used[((text & 7) * 16 + page) & 0x7F] = true;
	}
	for (row = 0; row < BOARD_HEIGHT; row++)
	{
		const uint32_t start = bits8 ? 512u * (uint32_t)((row + y_scroll) & 0xFF) : 256u * (uint32_t)((row + y_scroll) & 0x1FF);

		if (used[(start / PAGE_BYTES) & 0x7F])
			return true;
	}
	return false;
}

/* The frame, with each side's cells of the map the board's camera never shows going on as the cells beside
   them: the stages are drawn as far as it looks, past it they end, or keep a piece of somewhere else. The
   video draws them in place of video RAM's (s32_video_set_cells), which nothing writes: the bitmap, the
   text layer and the backgrounds read it too, some stages where the map layers' pages are. In a stage, the
   bitmap is off while it is drawn when it would show those pages (bitmap_shows_pages) */
void console_draw_frame(void)
{
	map_layer layers[MAP_LAYERS];
	bool on[MAP_LAYERS] = { false };
	int number, index;
	const uint16_t layers_off = video_register_word(0x02u);
	bool hide_bitmap = false;

	filled_count = 0;
	for (number = 0; number < MAP_LAYERS && stage_map_shown(); number++)
		on[number] = map_layer_of(number, &layers[number], on[1] ? &layers[1] : on[0] ? &layers[0] : NULL);
	for (number = 0; number < MAP_LAYERS; number++)
		if (on[number])
			fill_sides(&layers[number]);
	hide_bitmap = stage_drawn() && bitmap_shows_pages();
	if (hide_bitmap)
		mem_write16(VIDEO_REGISTERS + 0x02u, (uint16_t)(layers_off | BITMAP_OFF));
	s32_video_set_cells(own_words, own_marks);
	s32_video_draw_frame();
	if (hide_bitmap)
		mem_write16(VIDEO_REGISTERS + 0x02u, layers_off);    /* (as the game left it) */
	for (index = 0; index < filled_count; index++)
		own_marks[own_cells[index]] = 0u;
	filled_count = 0;
}

/* ---- debug: a stage's sections, as the game's own scroll skip mode (unused) moves the camera ---- */

#define SECTION_ROW        3u
#define SECTION_SHOWN      120     /* frames "SECTION n/last" stays */

static int debug_section_step;
static int debug_section_frames;

void console_debug_section(int step)
{
	debug_section_step += step;
}

/* The section the camera is in: load_camera_start_section leaves section_limit_x_list two entries past it */
static int current_section(void)
{
	const uint32_t first = stage_section_limits_x_ADDRESS + stage_section_limits_x[stage];

	return (int)(section_limit_x_list - first) / 2 - 2;
}

/* F7 and F8 (the port): the camera at the start of the section before or after, as the scroll skip
   mode's CALL does (move_camera_to_start_section, stage_start_section kept at most
   scroll_skip_last_points[stage]); its number shown a moment */
static void debug_sections(void)
{
	uint32_t ignored = 0, ignored_2 = 0, ignored_3 = 0;
	char text[24] = "SECTION ";
	int section, last, at;

	if (debug_section_step != 0 && console_in_game())
	{
		last = scroll_skip_last_points[stage];
		section = clamp(current_section() + debug_section_step, 0, last);
		stage_start_section = (uint16_t)section;
		move_camera_to_start_section(&ignored, &ignored_2, &ignored_3);
		at = 8;
		if (section >= 10)
			text[at++] = (char)('0' + section / 10);
		text[at++] = (char)('0' + section % 10);
		text[at++] = '/';
		if (last >= 10)
			text[at++] = (char)('0' + last / 10);
		text[at++] = (char)('0' + last % 10);
		text[at] = 0;
		erase_line(SECTION_ROW);
		write_centered(SECTION_ROW, text, MENU_CHOSEN);
		debug_section_frames = SECTION_SHOWN;
	}
	debug_section_step = 0;
	if (debug_section_frames > 0 && --debug_section_frames == 0)
		erase_line(SECTION_ROW);
}

/* ---- the players out of play ---- */

static const uint8_t player_start_bits[3] = { 0x10u, 0x20u, 0x80u };

/* In play, as player_select_character counts them: player_status with bit 7, 1 or 4 (choosing) */
static bool player_in_play(int player)
{
	const uint8_t status = player_status[player];

	return (int8_t)status < 0 || status == 1u || status == 4u;
}

#define NO_CONTINUE_CELLS  8    /* hud_no_continue_text's lines: "   No   ", "Continue" */
#define TEXT_ROW_BYTES     0x80u

/* The "No Continue" the HUD prints once where a player's start prompt is, when the last stage closes
   joining (joining_closed, player_hud_update): erased */
static void erase_no_continue(int player)
{
	const uint32_t position = 0x310000u | ((cabinet_config & 1u) != 0u ? start_prompt_positions_3p[player]
		: start_prompt_positions_2p[player]);
	int line, cell;

	if (joining_closed == 0u)
		return;
	for (line = 0; line < 2; line++)
		for (cell = 0; cell < NO_CONTINUE_CELLS; cell++)
			mem_write16(position + (uint32_t)line * TEXT_ROW_BYTES + 2u * (uint32_t)cell, (uint16_t)' ');
}

/* Each frame of a game: a player out of play has no ring count on screen (player select, with the 3-player
   cabinet, draws them, and the stage never erases them); while TIME ATTACK's clock runs, a player who was
   not in its run when it started cannot join (their start is not seen, nor the last stage's "No
   Continue" where their prompt is), and one who was goes on as soon as their continue's count is up
   (TIME ATTACK has no end of lives: the clock keeps running) */
static void keep_idle_players_out(void)
{
	int player, cell;

	if (system_mode != MODE_GAME && system_mode != MODE_DEMO_1 && system_mode != MODE_DEMO_2)
		return;
	for (player = 0; player < 3; player++)
	{
		if (player_in_play(player))
			continue;
		for (cell = 0; cell < 4; cell++)
			mem_write16((0x310000u | hud_ring_count_positions_3p[player]) + 2u * (uint32_t)cell, (uint16_t)' ');
		if (time_attack && clock_running && (time_attack_players & (1u << player)) == 0u)
		{
			inputs_pressed &= ~(uint32_t)player_start_bits[player];
			erase_no_continue(player);
		}
		else if (time_attack && clock_running && start_prompt_frames[player] > 0)
			inputs_pressed |= player_start_bits[player];    /* (out of the run's: it goes on at once) */
	}
}

/* ---- TIME ATTACK ---- */

#define CLOCK_ROW          1u
#define TIMES_TITLE_ROW    2u
#define TIMES_HEADER_ROW   5u
#define TIMES_FIRST_ROW    7u       /* then a level every two rows */
#define TIMES_TOTAL_ROW    24u      /* the whole game's */
#define WHOLE_GAME         0        /* the map's place for the whole game: Eggman's (the opening) */
#define TIMES_NAME_COLUMN  2u
#define TIMES_TIME_COLUMN  20u
#define TIMES_RUN_COLUMN   30u
#define TIMES_RECORD_ROW   26u
#define TIMES_BLINK        16u      /* the new record blinks every 16 frames */
#define FRAMES_A_SECOND    60u

/* The map's names of the levels (map_marker_blink's labels), the opening's ours */
static const char *const level_names[LEVELS] = {
	"OPENING", "VOLCANIC VAULT", "ICY ISLE", "DESERT DODGE", "TRAP TOWER", "LANDSLIDE LIMBO", "WILD WATER WAY",
	"EGGMAN'S TOWER"
};

console_times console_best_times;

static uint32_t clock_frames;
#define ENDING_STAGE       0x11u    /* the ending: the run's end */
#define LAST_STAGE         0x0Bu    /* Eggman's last: game_mode_play_stage closes joining there (joining_closed) */

static bool new_record;               /* the run that just ended is the best */
static bool shown_whole;              /* the page shows a whole game's run (else a level's) */
static int shown_level = -1;          /* the level the run that just ended timed alone */
static uint32_t last_run_frames;      /* its whole time (0: no run to show) */
static uint32_t run_splits[LEVELS];   /* its time at each level's end */
static int run_level;
static uint8_t times_blink;

/* frames as M'SS"HH (minutes as many digits as they take) */
static void format_time(uint32_t frames, char *text)
{
	const uint32_t hundredths = frames * 100u / FRAMES_A_SECOND;
	const uint32_t minutes = hundredths / 6000u, seconds = hundredths / 100u % 60u;
	char digits[12];
	int count = 0, at = 0;
	uint32_t left = minutes;

	do
	{
		digits[count++] = (char)('0' + left % 10u);
		left /= 10u;
	} while (left != 0u);
	while (count > 0)
		text[at++] = digits[--count];
	text[at++] = '\'';
	text[at++] = (char)('0' + seconds / 10u);
	text[at++] = (char)('0' + seconds % 10u);
	text[at++] = '"';
	text[at++] = (char)('0' + hundredths % 100u / 10u);
	text[at++] = (char)('0' + hundredths % 10u);
	text[at] = 0;
}

static void draw_clock(void)
{
	char text[24] = "TIME ";

	format_time(clock_frames, text + 5);
	erase_line(CLOCK_ROW);
	write_centered(CLOCK_ROW, text, MENU_CHOSEN);
}

/* From the title's menu: as STAGE SELECT starts a game (the map, then player select), timed: Eggman's
   place the whole game, a marker its level */
static void start_time_attack(void)
{
	int level;

	menu_open = false;
	erase_menu();
	time_attack = true;    /* (free play, as out of a game: no end of continues) */
	time_attack_level = -1;
	clock_running = false;
	times_pending = false;
	for (level = 0; level < LEVELS; level++)
		run_splits[level] = 0u;
	menu_after_title = false;
	stage_select_pending = true;
	stage_select_phase = 0;
	system_mode = MODE_GAME;
	game_mode = 0u;
	system_mode_step = 0u;
	game_in_progress = 0xFFu;
	game_sees_start = true;
}

/* The run is over: its time, kept when it beats the best (a whole game by its whole time) */
static void stop_clock(void)
{
	const bool whole = time_attack_level == WHOLE_GAME;
	int level;

	clock_running = false;
	erase_line(CLOCK_ROW);
	last_run_frames = clock_frames;
	shown_whole = whole;
	shown_level = time_attack_level;
	if (whole)
	{
		run_splits[LEVELS - 1] = clock_frames;
		new_record = console_best_times.run[LEVELS - 1] == 0u || clock_frames < console_best_times.run[LEVELS - 1];
		if (new_record)
			for (level = 0; level < LEVELS; level++)
				console_best_times.run[level] = run_splits[level];
	}
	else
	{
		new_record = console_best_times.levels[time_attack_level] == 0u
			|| clock_frames < console_best_times.levels[time_attack_level];
		if (new_record)
			console_best_times.levels[time_attack_level] = clock_frames;
	}
	console_best_times.changed = console_best_times.changed || new_record;
	times_pending = true;
}

/* A level's run is over at its results: the game ends as game over ends one, then BEST TIMES */
static void end_timed_game(void)
{
	time_attack = false;
	times_pending = false;
	game_in_progress = 0u;
	end_game_credits();
	stage_init_flags &= ~0x8040u;
	mixer_text_blend &= 0xBFFFu;
	mixer_nbg0_blend &= 0xBFFFu;
	mixer_nbg1_blend &= 0xBFFFu;
	mixer_nbg2_blend &= 0xBFFFu;
	mixer_nbg3_blend &= 0xBFFFu;
	mixer_bitmap_blend &= 0xBFFFu;
	open_times();
}

/* Each frame of a TIME ATTACK game: the clock starts with the picked level's first stage played and runs
   through its stages, scenes and continues (not in the pause: no game frames). A whole game runs on to
   the ending (stage 0x11), each level's end leaving its split, and BEST TIMES shows after the ending; a
   level stops at its stage clear banner (game mode 0x17; the tower at the ending) and BEST TIMES shows
   when its results begin. A game over ends the run with no time */
static void time_attack_frame(void)
{
	const int level = stage < sizeof(stage_level) ? stage_level[stage] : -1;
	const bool whole = time_attack_level == WHOLE_GAME;

	if (!time_attack)
		return;
	if (system_mode != MODE_GAME || (int8_t)game_in_progress >= 0)
	{
		time_attack = false;    /* (the game is over) */
		clock_running = false;
		erase_line(CLOCK_ROW);
		return;
	}
	if (times_pending)
	{
		if (!whole && (game_mode == 0x1Du || game_mode == 4u || (level >= 0 && level != time_attack_level)))
			end_timed_game();
		return;
	}
	if (!clock_running)
	{
		if (time_attack_level >= 0 && level == time_attack_level && console_in_game())
		{
			clock_running = true;
			clock_frames = 0u;
			run_level = level;
			time_attack_players = 0u;
			for (int player = 0; player < 3; player++)
				if (player_in_play(player))
					time_attack_players |= (uint8_t)(1u << player);
		}
		return;
	}
	if (stage == ENDING_STAGE || (!whole && (game_mode == 0x17u || (level >= 0 && level != time_attack_level))))
	{
		stop_clock();
		return;
	}
	if (whole && level > run_level)
	{
		run_splits[run_level] = clock_frames;    /* (the level before is over) */
		run_level = level;
	}
	if (stage == LAST_STAGE)
		joining_closed = 0u;    /* (no end of lives: the last stage closes continues each frame, before its players run) */
	clock_frames++;
	draw_clock();
}

/* ---- BEST TIMES: OPTIONS' page with the levels' best times ---- */

static uint32_t times_row(int level)
{
	return TIMES_FIRST_ROW + 2u * (uint32_t)level;
}

static void open_times(void)
{
	menu_open = false;
	times_open = true;
	times_blink = 0u;
	option_wait = MENU_PAUSE;
	system_mode = MODE_OPTIONS;
	system_mode_step = 0u;
}

/* A time, or dashes for none */
static void time_or_dashes(uint32_t frames, char *text)
{
	if (frames != 0u)
		format_time(frames, text);
	else
		strcpy(text, "-'--\"--");
}

/* After a whole game: the best run's splits and whole time, and the run's beside them. Else the levels'
   best times alone and the best whole game, the run's level with its time beside (a new best blinks) */
static void draw_times(void)
{
	char text[24];
	int level;
	const bool blinking = new_record && (times_blink & TIMES_BLINK) != 0u;

	times_blink++;
	write_centered(TIMES_TITLE_ROW, shown_whole ? "BEST RUN" : "BEST TIMES", MENU_CHOSEN);
	write_text(TIMES_TIME_COLUMN, TIMES_HEADER_ROW, "BEST", MENU_CHOSEN);
	if (last_run_frames != 0u)
		write_text(TIMES_RUN_COLUMN, TIMES_HEADER_ROW, "THIS RUN", MENU_CHOSEN);
	for (level = 0; level <= LEVELS; level++)
	{
		const bool total = level == LEVELS;
		const uint32_t row = total ? TIMES_TOTAL_ROW : times_row(level);
		const bool this_one = last_run_frames != 0u && (shown_whole ? total : level == shown_level);
		uint32_t best = 0u, run = 0u;

		if (shown_whole)
		{
			best = console_best_times.run[total ? LEVELS - 1 : level];
			run = total ? last_run_frames : run_splits[level];
		}
		else
		{
			best = total ? console_best_times.run[LEVELS - 1] : level == WHOLE_GAME ? 0u : console_best_times.levels[level];
			run = this_one ? last_run_frames : 0u;
		}
		erase_line(row);
		write_text(TIMES_NAME_COLUMN, row, total ? "WHOLE GAME" : level_names[level],
			this_one || total ? MENU_CHOSEN : MENU_TEXT);
		if (!(this_one && blinking))
		{
			time_or_dashes(best, text);
			write_text(TIMES_TIME_COLUMN, row, text, this_one && new_record ? MENU_CHOSEN : MENU_TEXT);
		}
		if (last_run_frames != 0u && (shown_whole || this_one))
		{
			time_or_dashes(run, text);
			write_text(TIMES_RUN_COLUMN, row, text, MENU_TEXT);
		}
	}
	if (new_record && last_run_frames != 0u)
		write_centered(TIMES_RECORD_ROW, "NEW RECORD", MENU_CHOSEN);
}

static void close_times(void)
{
	uint32_t row;

	for (row = TIMES_TITLE_ROW; row <= TIMES_RECORD_ROW; row++)
		erase_line(row);
	times_open = false;
	last_run_frames = 0u;
	new_record = false;
	menu_after_title = true;
	attract_step = 0u;
	system_mode = MODE_TITLE;
	system_mode_step = 0u;
}

/* BEST TIMES: a pick or back goes back to the title (after a moment, so the press that ended the level
   does not leave it at once) */
static void run_times(void)
{
	draw_times();
	if (option_wait > 0)
	{
		option_wait--;
		return;
	}
	if (picked() || (actions & ACTION_BIT(ACTION_BACK)) != 0u)
		close_times();
}

/* The game's modes: the first frame of a game the menu started sees player 1's start pressed, as the
   arcade's start frame does (run_frame_start_game runs the game in the frame of the press): that is how the
   game takes player 1 in and lets him choose a character. TRACKBALL's game holds the how to play: its clock
   stopped short of its end, and no button ends it (the buttons are back for the players' objects after) */
void run_game_mode(uint32_t pressed, uint32_t position, uint32_t speed, uint16_t next_scroll_position, uint32_t object,
	uint32_t *p_speed_value, uint32_t *p_count, uint32_t *p_cycle, uint32_t *p_list_cursor)
{
	bool holding = false;
	uint32_t pressed_now = 0;

	if (game_sees_start)
	{
		game_sees_start = false;
		inputs_pressed |= PRESSED_START_1;
	}
	/* STAGE SELECT: the map first (game mode 8, where player select would begin), then player select, then
	   in the how to play's place the chosen stage, as the original's stage select starts one (cleared_stages
	   from stage_select_cleared_values, game mode 2, stage_start_kind 1) */
	if (stage_select_pending && stage_select_phase == 0 && game_mode == 0x1Bu)
	{
		game_mode = 8u;
		stage_select_phase = 1;
	}
	else if (stage_select_pending && stage_select_phase == 2 && game_mode != 0x1Bu)
	{
		stage_select_pending = false;
		stage = stage_select_stage;
		mem_write16(cleared_stages_ADDRESS, stage_select_cleared_values[stage]);
		stage_start_kind = 1u;
		game_mode = 2u;
	}
	for (int player = 0; player < CONTROLS_PLAYERS; player++)
		if (start_prompt_frames[player] > 0)
			start_prompt_frames[player]--;
	keep_idle_players_out();
	if (trackball_open)
	{
		run_trackball();
		if (!trackball_open)
			return;    /* (BACK: OPTIONS from the next frame) */
	}
	holding = trackball_open && game_mode == 1u;
	pressed_now = inputs_pressed;
	if (holding)
		inputs_pressed &= ~PRESSED_BUTTONS;
	original_run_game_mode(pressed, position, speed, next_scroll_position, object, p_speed_value, p_count, p_cycle,
		p_list_cursor);
	if (holding)
	{
		inputs_pressed = pressed_now;
		game_mode_timer = TRACKBALL_TIMER;
	}
	note_levels_cleared();
	wide_how_to_play();
	debug_sections();
	time_attack_frame();
}

/* ---- WIDE in the game ---- */

#define BLOCK_WIDTH          0x200u    /* stage_block_maps' blocks: 512 x 256 pixels */
#define PANEL_FRAME          0x71F0u   /* the how to play's control panel strip (how_to_play_panel_start) */
#define PANEL_PALETTE        0x8780u
#define PANEL_FLAGS          0x100Au
#define PANEL_WIDTH          320
#define PANEL_Y              0x88

/* The camera's tile sets come from the 2 x 2 blocks from the one the screen's left edge is in, which
   covers its 320 pixels however it lies on them; WIDE shows S32_VIDEO_WIDE_SIDE more at each side, so
   the blocks start from the wide left edge (2 blocks of 512 still cover the 320 and both sides) */
uint32_t set_nbg0_scroll_from_camera(uint32_t scroll_x)
{
	uint16_t block_column = 0;

	scroll_x = original_set_nbg0_scroll_from_camera(scroll_x);
	if (!console_wide_now() || (uint16_t)camera_x_pixels < S32_VIDEO_WIDE_SIDE)
		return scroll_x;
	block_column = (uint16_t)(((uint16_t)camera_x_pixels - S32_VIDEO_WIDE_SIDE) & ~(BLOCK_WIDTH - 1u));
	tile_block_column = ((uint32_t)block_column << 7) | (block_column >> 9);
	return scroll_x;
}

static bool panel_sides_shown;

/* The how to play's panel strip at the sides too: a copy on each side, of which a wide picture shows the
   plain ends (the strip's middle has the title); as the strip, they go when the stage is no longer 7
   (how_to_play_panel_show) */
static void wide_how_to_play(void)
{
	uint32_t free_count = 0, panel = 0;
	int side;

	if (stage != 7u || game_mode != 1u)
	{
		panel_sides_shown = false;
		return;
	}
	if (panel_sides_shown)
		return;
	panel_sides_shown = true;
	for (side = -1; side <= 1; side += 2)
	{
		allocate_object_outside_loop(&free_count, &panel);
		if ((uint8_t)free_count == 0u)
			return;
		OBJECT(panel)->sprite_flags = PANEL_FLAGS;
		OBJECT(panel)->sprite_frame = PANEL_FRAME;
		OBJECT(panel)->sprite_palette = PANEL_PALETTE;
		OBJECT(panel)->screen_x = (int16_t)(side * PANEL_WIDTH);
		OBJECT(panel)->screen_y = PANEL_Y;
		OBJECT(panel)->handler = how_to_play_panel_show_ADDRESS;
	}
}

/* ---- the captions in English: the game's own English boxes, which its (Japanese) program never shows ---- */

typedef struct
{
	uint8_t kind;              /* message_box_kinds' */
	uint16_t frame;            /* the English box (sprite_sheets.txt: 9F28-9F88, 9F90, 9FF8, A1B8) */
	uint16_t width, height;    /* its full size */
	uint16_t palette;          /* 0: the Japanese one's */
	uint8_t pen, from_pen;     /* in its palette, pen takes from_pen's colour (none when they are equal) */
} english_box;

/* By kind, what each Japanese line says in the English boxes (0x0E is never drawn; 0x12, "haaa!!", has none:
   its own frame becomes "Huh!", console_prepare_sprites). Eggman's are drawn without their box */
static const english_box english_boxes[] = {
	{ 0x00, 0x9F58u, 304u, 32u },    /* "Try to get through this!" (this time, like this!) */
	{ 0x09, 0x9F58u, 304u, 32u },
	/* the how to play: 'Press button while spinning control ball for "Spin Jump".', in kind 0x0C's yellow
	   (0x0B's pens grey it), its outline (pen 1, white there) black as the Japanese line's (pen 2) */
	{ 0x0B, 0x9F90u, 304u, 24u, 0x89F0u, 1u, 2u },
	{ 0x0C, 0x9F90u, 304u, 24u, 0x89F0u, 1u, 2u },    /* (both of its lines) */
	{ 0x0D, 0x9FF8u, 120u, 32u, 0u, 11u, 1u },    /* "CONTROLS" (how to play): its letters' outline (pen 11,
	                                               white) dark as the Japanese title's (pen 1) */
	{ 0x0F, 0x9F50u, 304u, 32u },    /* "I'll get you." (drop dead, Sonic!!) */
	{ 0x10, 0x9F30u, 304u, 56u },    /* "Can you escape from Eggman's island of traps?" */
	{ 0x11, 0x9F48u, 304u, 56u },    /* "I'll show you what terror really means." */
	{ 0x13, 0x9F88u, 304u, 32u },    /* "This is just the beginning!" (don't think it's over) */
	{ 0x14, 0x9F70u, 304u, 32u },    /* "You'll never escape!" (not yet) */
	{ 0x15, 0x9F80u, 304u, 32u },    /* "I'll blow you away!" */
	{ 0x16, 0xA1B8u, 304u, 32u },    /* "Doggonit!" */
	{ 0x17, 0x9F78u, 304u, 32u },    /* "This is the end." */
	{ 0x18, 0x9F80u, 304u, 32u },    /* "I'll blow you away!" (with the tower) */
	{ 0x19, 0x9F50u, 304u, 32u },    /* "I'll get you." (drop dead, Sonic!!) */
	{ 0x1A, 0x9F50u, 304u, 32u },    /* (drop dead, Ray!!) */
	{ 0x1B, 0x9F50u, 304u, 32u },    /* (drop dead, Mighty!!) */
};

#define PALETTE_RAM          0x600000u
#define SPRITE_PALETTE_BASE  0xC00u    /* the sprite groups' (mixer registers 0x00-0x1E: 0x3n, 0x30 << 6) */
/* The how to play's English boxes outline their letters with a pen that is white in their palettes: while
   one is up, its 'pen' takes 'from_pen''s colour in its palette (dark, as the Japanese ones' outline) */
static uint32_t palette_pen(uint16_t palette, uint32_t pen)
{
	return PALETTE_RAM + (SPRITE_PALETTE_BASE + (palette & 0x7FF0u) + pen) * 2u;
}

/* A message box as the game sets it up, then its English box in the Japanese one's place: its own frame and
   size (and the scale its frame's record gives, as place_message_box takes it), the same palette unless it
   names one */
void place_message_box(uint16_t screen_x, uint16_t screen_y, uint32_t kind, uint16_t frames, uint32_t box)
{
	size_t entry;

	original_place_message_box(screen_x, screen_y, kind, frames, box);
	for (entry = 0; entry < sizeof(english_boxes) / sizeof(english_boxes[0]); entry++)
		if (english_boxes[entry].kind == (uint16_t)kind)
		{
			OBJECT(box)->sprite_frame = english_boxes[entry].frame;
			OBJECT(box)->speed_x_int = english_boxes[entry].width;
			OBJECT(box)->sprite_height = english_boxes[entry].height;
			OBJECT(box)->sprite_scale = mem_read16(((sprite_frames & ~0xFFFFu) | english_boxes[entry].frame) + 6u);
			if (english_boxes[entry].palette != 0u)
				OBJECT(box)->sprite_palette = english_boxes[entry].palette;
			if (english_boxes[entry].pen != english_boxes[entry].from_pen)
				mem_write16(palette_pen(OBJECT(box)->sprite_palette, english_boxes[entry].pen),
					mem_read16(palette_pen(OBJECT(box)->sprite_palette, english_boxes[entry].from_pen)));
			return;
		}
}

/* ---- Eggman's English lines without their red box: the sprite graphics' copy the port draws from ---- */

#define ENGLISH_LINES_FIRST  0x9F28u    /* the English speech boxes (sprite_sheets.txt), 8 apart ... */
#define ENGLISH_LINES_LAST   0x9F88u
#define ENGLISH_DOGGONIT     0xA1B8u    /* ... and one more */
#define LINE_BOX_PEN         4u         /* the box's red */
#define LINE_TEXT_PEN        2u         /* the letters (black in the box's palettes) */
#define LINE_FILL_PEN        1u         /* white there: the letters' fill now */
#define LINE_OUTLINE_PEN     2u         /* black: their outline */
#define LINE_MOST_WIDTH      304
#define LINE_MOST_HEIGHT     56

/* A pixel's pen in a 4-bit frame: 32-bit big-endian words of 8 pens, the first in the high nibble */
static uint8_t *pen_byte(uint8_t *sprites, uint32_t word, int words, int x, int y)
{
	return &sprites[((size_t)word + (size_t)y * (size_t)words + (size_t)(x >> 3)) * 4u + (size_t)((x & 7) >> 1)];
}

/* A halfword of the program ROM (the V60's byte order) */
static uint32_t program_word(const uint8_t *program, uint32_t address)
{
	return (uint32_t)program[address] | (uint32_t)program[address + 1u] << 8;
}

typedef struct
{
	uint32_t word;     /* where its pens start in the sprite graphics (32-bit words) */
	int width, height;
	uint8_t pens[LINE_MOST_HEIGHT][LINE_MOST_WIDTH];
} line_frame;

/* A frame's record (the V60's byte order) and its pens; false when it is not a 4-bit frame that fits */
static bool read_line_frame(const uint8_t *sprites, size_t size, const uint8_t *program, uint16_t frame,
	line_frame *line)
{
	const uint32_t record = sprite_frame_records_ADDRESS + frame;
	const uint32_t bank_bits = program_word(program, record + 4u);
	int x, y;

	line->width = (int)program_word(program, record);
	line->height = (int)(program_word(program, record + 2u) & 0xFFu);
	line->word = ((bank_bits & 0x0800u) >> 11 | (bank_bits & 0x4000u) >> 13) << 20
		| (program_word(program, record + 2u) & 0xF000u) << 4 | program_word(program, record + 6u);
	if (line->width > LINE_MOST_WIDTH || line->height > LINE_MOST_HEIGHT
		|| ((size_t)line->word + (size_t)line->height * (size_t)(line->width >> 3)) * 4u > size)
		return false;
	for (y = 0; y < line->height; y++)
		for (x = 0; x < line->width; x++)
		{
			const uint8_t byte = *pen_byte((uint8_t *)sprites, line->word, line->width >> 3, x, y);

			line->pens[y][x] = (x & 1) ? (byte & 0x0Fu) : (byte >> 4);
		}
	return true;
}

/* A line's pens back into its frame: its letters (LINE_TEXT_PEN) white, a dark outline around them where
   the box was (LINE_BOX_PEN), the rest of the box see-through (pen 0); its corners' end codes stay */
static void write_unboxed(uint8_t *sprites, const line_frame *line)
{
	const int words = line->width >> 3, width = line->width, height = line->height;
	int x, y;

	for (y = 0; y < height; y++)
		for (x = 0; x < width; x++)
		{
			uint8_t *byte = pen_byte(sprites, line->word, words, x, y);
			uint8_t pen = line->pens[y][x];
			int dx, dy;

			if (pen == LINE_TEXT_PEN)
				pen = LINE_FILL_PEN;
			else if (pen == LINE_BOX_PEN)
			{
				pen = 0u;
				for (dy = -1; dy <= 1; dy++)
					for (dx = -1; dx <= 1; dx++)
						if (y + dy >= 0 && y + dy < height && x + dx >= 0 && x + dx < width
							&& line->pens[y + dy][x + dx] == LINE_TEXT_PEN)
							pen = LINE_OUTLINE_PEN;
			}
			*byte = (x & 1) ? (uint8_t)((*byte & 0xF0u) | pen) : (uint8_t)((*byte & 0x0Fu) | (uint8_t)(pen << 4));
		}
}

/* The columns of a line's 'index'th letter (letters lie apart by empty columns; an "!" and its dot are one) */
static bool find_letter(const line_frame *line, int index, int *first, int *after)
{
	int x = 0, y, letter = -1;
	bool inked = false, was_inked = false;

	for (x = 0; x <= line->width; x++)
	{
		inked = false;
		for (y = 0; x < line->width && y < line->height; y++)
			inked = inked || line->pens[y][x] == LINE_TEXT_PEN;
		if (inked && !was_inked && ++letter == index)
			*first = x;
		if (!inked && was_inked && letter == index)
		{
			*after = x;
			return true;
		}
		was_inked = inked;
	}
	return false;
}

/* The first and last rows with letters in a line (its cap height and descenders) */
static void letter_rows(const line_frame *line, int *top)
{
	int x, y;

	for (y = 0; y < line->height; y++)
		for (x = 0; x < line->width; x++)
			if (line->pens[y][x] == LINE_TEXT_PEN)
			{
				*top = y;
				return;
			}
	*top = 0;
}

#define HUH_FRAME        0x65F0u    /* kind 0x12's Japanese box, "haaa!!" (128 x 24): "Huh!" in its place */
#define LETTER_GAP       2          /* between letters, as the English lines have them */
#define STROKE           3          /* the letters' upright strokes */
#define CAP_HEIGHT       15         /* the capitals' rows */
#define H_WIDTH          9          /* an H of two strokes and a bar, as wide as the u and h */
#define H_BAR_ROW        7          /* its bar, a row (as the e's) */

/* Copies a letter of 'from' into 'to' at x, the rows lined up by their tops (to_top, from_top) */
static int paste_letter(line_frame *to, int x, int to_top, const line_frame *from, int from_top, int first,
	int after)
{
	int column, y;

	for (column = first; column < after && x < to->width; column++, x++)
		for (y = 0; y + from_top < from->height && y + to_top < to->height; y++)
			if (from->pens[y + from_top][column] == LINE_TEXT_PEN)
				to->pens[y + to_top][x] = LINE_TEXT_PEN;
	return x;
}

/* "Huh!" for kind 0x12, which has no English line: h from "This is only the beginning.", u from
   "I'll get you.", ! from "Doggonit!", and an H of the u's strokes and the e's bar, centred on its frame */
static void make_huh(uint8_t *sprites, size_t size, const uint8_t *program)
{
	static line_frame huh, this_line, you_line, doggonit_line;
	int this_top = 0, you_top = 0, doggonit_top = 0, top, x, y, width;
	int h_first, h_after, u_first, u_after, mark_first, mark_after;

	if (!read_line_frame(sprites, size, program, HUH_FRAME, &huh)
		|| !read_line_frame(sprites, size, program, 0x9F28u, &this_line)
		|| !read_line_frame(sprites, size, program, 0x9F50u, &you_line)
		|| !read_line_frame(sprites, size, program, ENGLISH_DOGGONIT, &doggonit_line)
		|| !find_letter(&this_line, 1, &h_first, &h_after)        /* T h */
		|| !find_letter(&you_line, 9, &u_first, &u_after)         /* I ' l l g e t y o u */
		|| !find_letter(&doggonit_line, 8, &mark_first, &mark_after))    /* D o g g o n i t ! */
		return;
	letter_rows(&this_line, &this_top);
	letter_rows(&you_line, &you_top);
	letter_rows(&doggonit_line, &doggonit_top);
	for (y = 0; y < huh.height; y++)
		for (x = 0; x < huh.width; x++)
			huh.pens[y][x] = LINE_BOX_PEN;    /* (all of it becomes see-through, or the outline) */
	width = H_WIDTH + LETTER_GAP + (u_after - u_first) + LETTER_GAP + (h_after - h_first) + LETTER_GAP
		+ (mark_after - mark_first);
	x = (huh.width - width) / 2;
	top = (huh.height - (CAP_HEIGHT + 3)) / 2;
	for (y = 0; y < CAP_HEIGHT; y++)
		for (int column = 0; column < H_WIDTH; column++)
			if (column < STROKE || column >= H_WIDTH - STROKE || y == H_BAR_ROW)
				huh.pens[top + y][x + column] = LINE_TEXT_PEN;
	x += H_WIDTH + LETTER_GAP;
	x = paste_letter(&huh, x, top, &you_line, you_top, u_first, u_after) + LETTER_GAP;
	x = paste_letter(&huh, x, top, &this_line, this_top, h_first, h_after) + LETTER_GAP;
	paste_letter(&huh, x, top, &doggonit_line, doggonit_top, mark_first, mark_after);
	write_unboxed(sprites, &huh);
}

/* A frame's pens back as they are */
static void write_pens(uint8_t *sprites, const line_frame *line)
{
	int x, y;

	for (y = 0; y < line->height; y++)
		for (x = 0; x < line->width; x++)
		{
			uint8_t *byte = pen_byte(sprites, line->word, line->width >> 3, x, y);
			const uint8_t pen = line->pens[y][x];

			*byte = (x & 1) ? (uint8_t)((*byte & 0xF0u) | pen) : (uint8_t)((*byte & 0x0Fu) | (uint8_t)(pen << 4));
		}
}

#define LABEL_ROWS       8          /* the map labels' letters (pen 1, their shadow pen 2) */
#define LABEL_SPACE      8
#define LABEL_LINE_ROW   9          /* their underline: a row of pen 1 over one of pen 2 */

/* "ALL GAME" for TIME ATTACK's first place on the map, in the unused ROBOTNIK'S TOWER label's frame:
   A and L from VOLCANIC VAULT (9F98), G, M and E from EGGMAN'S TOWER (9FC8), their columns as those
   labels have them, then the labels' underline under them */
static void make_all_game(uint8_t *sprites, size_t size, const uint8_t *program)
{
	static line_frame label, volcanic, eggman;
	static const struct
	{
		char letter;
		const line_frame *from;
		int first, after;
	} letters[] = {
		{ 'A', &volcanic, 32, 40 }, { 'L', &volcanic, 16, 24 }, { 'L', &volcanic, 16, 24 }, { ' ', NULL, 0, LABEL_SPACE },
		{ 'G', &eggman, 24, 32 }, { 'A', &volcanic, 32, 40 }, { 'M', &eggman, 40, 49 }, { 'E', &eggman, 16, 24 },
	};
	size_t i;
	int x = 0, y, column;

	if (!read_line_frame(sprites, size, program, ALL_GAME_FRAME, &label)
		|| !read_line_frame(sprites, size, program, 0x9F98u, &volcanic)
		|| !read_line_frame(sprites, size, program, 0x9FC8u, &eggman))
		return;
	for (y = 0; y < label.height; y++)
		for (column = 0; column < label.width; column++)
			label.pens[y][column] = 0u;
	for (i = 0; i < sizeof(letters) / sizeof(letters[0]); i++)
	{
		for (column = letters[i].first; letters[i].from && column < letters[i].after; column++)
			for (y = 0; y < LABEL_ROWS; y++)
			{
				const uint8_t pen = letters[i].from->pens[y][column];

				label.pens[y][x + column - letters[i].first] = pen == 1u || pen == 2u ? pen : 0u;
			}
		x += letters[i].after - letters[i].first;
	}
	for (column = 0; column < x && column < label.width; column++)
	{
		label.pens[LABEL_LINE_ROW][column] = 1u;
		label.pens[LABEL_LINE_ROW + 1][column] = 2u;
	}
	write_pens(sprites, &label);
}

void console_prepare_sprites(uint8_t *sprites, size_t size, const uint8_t *program)
{
	static line_frame line;
	uint16_t frame;

	make_all_game(sprites, size, program);
	make_huh(sprites, size, program);    /* (first: it cuts its letters from the lines as they are) */
	for (frame = ENGLISH_LINES_FIRST; frame <= ENGLISH_LINES_LAST; frame += 8u)
		if (read_line_frame(sprites, size, program, frame, &line))
			write_unboxed(sprites, &line);
	if (read_line_frame(sprites, size, program, ENGLISH_DOGGONIT, &line))
		write_unboxed(sprites, &line);
}

/* ---- the window's icon: the three characters' heads, from the player's ROM ---- */

#define HEAD_SIZE            32
static const struct
{
	uint16_t frame;      /* join_chooser_portraits' head (32 x 32, its end code drawn: an opaque frame) */
	uint32_t palette;    /* its colours in the program ROM (sprite_palettes.json) */
	int x, y;            /* where in the icon */
} icon_heads[] = {
	{ 0x90C8u, 0x10738Eu, 0, 0 },                  /* Sonic */
	{ 0x90D0u, 0x1073AEu, HEAD_SIZE, 0 },          /* Mighty */
	{ 0x90D8u, 0x1073CEu, HEAD_SIZE / 2, HEAD_SIZE },    /* Ray */
};

/* A palette entry of the program ROM (the palette window's alternate format: the low bit of blue, green
   and red in bits 14-12) as 0xAARRGGBB, opaque */
static uint32_t rom_color(const uint8_t *program, uint32_t address)
{
	const uint32_t value = program_word(program, address);
	const uint32_t red = ((value >> 12) & 1u) | ((value << 1) & 0x1Eu);
	const uint32_t green = ((value >> 13) & 1u) | ((value >> 3) & 0x1Eu);
	const uint32_t blue = ((value >> 14) & 1u) | ((value >> 7) & 0x1Eu);

	return 0xFF000000u | (red * 255u / 31u) << 16 | (green * 255u / 31u) << 8 | blue * 255u / 31u;
}

bool console_window_icon(uint32_t *pixels, const uint8_t *sprites, size_t size, const uint8_t *program)
{
	static line_frame head;
	size_t i;
	int x, y;

	memset(pixels, 0, CONSOLE_ICON_SIZE * CONSOLE_ICON_SIZE * sizeof(pixels[0]));
	for (i = 0; i < sizeof(icon_heads) / sizeof(icon_heads[0]); i++)
	{
		if (!read_line_frame(sprites, size, program, icon_heads[i].frame, &head)
			|| head.width != HEAD_SIZE || head.height != HEAD_SIZE)
			return false;
		for (y = 0; y < HEAD_SIZE; y++)
			for (x = 0; x < HEAD_SIZE; x++)
				if (head.pens[y][x] != 0u)    /* (pen 0 see-through) */
					pixels[(icon_heads[i].y + y) * CONSOLE_ICON_SIZE + icon_heads[i].x + x] =
						rom_color(program, icon_heads[i].palette + head.pens[y][x] * 2u);
	}
	return true;
}

/* ---- the players' start prompts ---- */

/* A player's start prompt (to join, or a continue's count): drawn this frame or the last, it says that
   player's start is the board's start (console_start_prompted), not the pause */
uint32_t draw_player_start_prompt(void)
{
	const uint32_t player = OBJECT(current_object)->player;

	if (player < CONTROLS_PLAYERS)
		start_prompt_frames[player] = 2;
	return original_draw_player_start_prompt();
}

/* ---- player select ---- */

#define CHOOSERS           3
#define TURN_NOW           0xF8u    /* a count's low byte one frame (+ 8) short of a turn */

static bool chooser_rolling[CHOOSERS];

/* A player's character chooser: the game turns once every 32 frames the trackball keeps moving (+ 8 a
   frame into the count's low byte, a turn at each carry); the first frame of a move turns at once, so a
   touch of left or right turns one character, and holding it goes on as the game does */
void character_chooser_wait(void)
{
	uint32_t kind = OBJECT(current_object)->kind;
	int16_t trackball_x = 0;

	if (trackball_open)
	{
		character_chooser_confirm();    /* (TRACKBALL's game: player 1's first character, Sonic, at once */
		character_select_timer = 1;     /* and on to the how to play: game_mode_character_select) */
		return;
	}
	if (kind < CHOOSERS)
	{
		trackball_x = (int16_t)RAM16(GAME_VARIABLES + chooser_trackball_offsets[kind]);
		if (trackball_x != 0 && !chooser_rolling[kind])
		{
			if (trackball_x < 0)
				OBJECT(current_object)->character_chooser.left_count =
					(uint16_t)((OBJECT(current_object)->character_chooser.left_count & 0xFF00u) | TURN_NOW);
			else
				OBJECT(current_object)->character_chooser.right_count =
					(uint16_t)((OBJECT(current_object)->character_chooser.right_count & 0xFF00u) | TURN_NOW);
		}
		chooser_rolling[kind] = trackball_x != 0;
	}
	original_character_chooser_wait();
}

static bool join_chooser_rolling[CHOOSERS];

/* A joining player's chooser in a stage (a continue, or a player who joins): it turns as player select's
   does, once every 32 frames the trackball keeps moving (spawn_arg_1 counts right, spawn_arg_2 left, a
   turn at each carry); the first frame of a move turns at once here too */
void join_character_chooser_update(uint32_t text_x)
{
	const uint32_t player = OBJECT(object_cursor)->player;
	uint32_t block = 0;
	int16_t trackball_x = 0;

	if (player < CHOOSERS && (players_in_join_chooser & (1u << player)) != 0u)
	{
		block = join_chooser_blocks[player];
		trackball_x = (int16_t)RAM16(GAME_VARIABLES + chooser_trackball_offsets[OBJECT(block)->kind]);
		if (trackball_x != 0 && !join_chooser_rolling[player])
		{
			if (trackball_x < 0)
				OBJECT(block)->spawn_arg_2 = (uint16_t)((OBJECT(block)->spawn_arg_2 & 0xFF00u) | TURN_NOW);
			else
				OBJECT(block)->spawn_arg_1 = (uint16_t)((OBJECT(block)->spawn_arg_1 & 0xFF00u) | TURN_NOW);
		}
		join_chooser_rolling[player] = trackball_x != 0;
		if (time_attack && clock_running)
			OBJECT(block)->join_character_chooser.countdown = 0u;    /* (TIME ATTACK: the one shown, at once) */
	}
	original_join_character_chooser_update(text_x);
}

/* ---- for the port ---- */

#define PAUSE_ROW          2u

static uint16_t pause_saved[40];
#ifdef SEGASONIC_PAUSE_EXIT
/* (Android's: the pause's choices, CONTINUE and EXIT, a row apart in the screen's middle) */
static const char *const pause_items[CONSOLE_PAUSE_ITEMS] = { "CONTINUE", "EXIT" };
static uint16_t pause_menu_saved[CONSOLE_PAUSE_ITEMS][40];
static int pause_choice;
#endif

void console_actions(uint32_t pressed, uint32_t held)
{
	actions = pressed;
	actions_held = held;
}

void console_back(void)
{
	if (map_open)
	{
		leave_map();
		return;
	}
	if (times_open)
	{
		close_times();
		return;
	}
	if (controls_open || trackball_open)
		return;    /* (CONTROLS and TRACKBALL read the action back themselves) */
	if (options_open && video_open)
	{
		close_video();
		return;
	}
	if (options_open)
	{
		close_options();
		return;
	}
	if (!menu_open)
		return;
	menu_open = false;
	erase_menu();
	title_frames = 0;    /* (PRESS START again, from its shown half) */
}

int console_controls_row(int action)
{
	if (!options_open || !controls_open || system_mode != MODE_OPTIONS || action < 0 || action >= ACTIONS)
		return -1;
	return (int)controls_row(action + 1);
}

int console_controls_player(void)
{
	return controls_player;
}

bool console_start_prompted(int player)
{
	return player >= 0 && player < CONTROLS_PLAYERS && start_prompt_frames[player] > 0;
}

int console_controls_waiting(void)
{
	return controls_waiting;
}

void console_controls_bound(void)
{
	controls_waiting = -1;
}

bool console_controls_defaults(void)
{
	const bool asked = controls_defaults_asked;

	controls_defaults_asked = false;
	return asked;
}

bool console_wide_now(void)
{
	/* a stage drawn (stage_drawn); the how to play (its panel goes on at the sides: wide_how_to_play), the
	   SEGA logo and the title (their sky and sea go on), the ranking, OPTIONS and player select (their
	   faces) */
	return console_window_settings.wide
		&& (stage_drawn()
			|| (system_mode == MODE_GAME && (int8_t)game_in_progress < 0 && (game_mode == 1u || game_mode == 0x1Bu))
			|| system_mode == MODE_TITLE || system_mode == MODE_RANKING || system_mode == MODE_OPTIONS);
}

bool console_in_game(void)
{
	if (system_mode != MODE_GAME || (int8_t)game_in_progress >= 0)
		return false;
	/* a stage being played (game_mode_play_stage: modes 3, 5 and 6; the screen wipe that opens one, 0x11) or
	   one of its events, the scenes the game plays in it (modes 9 to 0x0D, 0x0F, 0x10, 0x12 to 0x16, 0x18 to
	   0x1A, 0x1C, 0x1F); not the intro, player select, the how to play, the map, the continue, the stage
	   clear or its results, where start is start */
	return game_mode == 3u || game_mode == 5u || game_mode == 6u || game_mode == 0x11u
		|| (game_mode >= 0x09u && game_mode <= 0x0Du) || game_mode == 0x0Fu || game_mode == 0x10u
		|| (game_mode >= 0x12u && game_mode <= 0x16u) || (game_mode >= 0x18u && game_mode <= 0x1Au)
		|| game_mode == 0x1Cu || game_mode == 0x1Fu;
}

void console_pause_text(bool shown)
{
	uint32_t column;
#ifdef SEGASONIC_PAUSE_EXIT
	int item;
#endif

	if (shown)
	{
		for (column = 0; column < 40u; column++)
			pause_saved[column] = mem_read16(TEXT_CELL(column, PAUSE_ROW));
		write_centered(PAUSE_ROW, "PAUSE", MENU_CHOSEN);
#ifdef SEGASONIC_PAUSE_EXIT
		for (item = 0; item < CONSOLE_PAUSE_ITEMS; item++)
		{
			const uint32_t row = CONSOLE_PAUSE_MENU_ROW + 2u * (uint32_t)item;

			for (column = 0; column < 40u; column++)
				pause_menu_saved[item][column] = mem_read16(TEXT_CELL(column, row));
			write_centered(row, pause_items[item], item == pause_choice ? MENU_CHOSEN : MENU_TEXT);
		}
#endif
		return;
	}
	for (column = 0; column < 40u; column++)
		mem_write16(TEXT_CELL(column, PAUSE_ROW), pause_saved[column]);
#ifdef SEGASONIC_PAUSE_EXIT
	for (item = 0; item < CONSOLE_PAUSE_ITEMS; item++)
		for (column = 0; column < 40u; column++)
			mem_write16(TEXT_CELL(column, CONSOLE_PAUSE_MENU_ROW + 2u * (uint32_t)item),
				pause_menu_saved[item][column]);
#endif
}

#ifdef SEGASONIC_PAUSE_EXIT
void console_pause_choice(int choice)
{
	pause_choice = choice;
}
#endif

bool console_quit_requested(void)
{
	return quit_requested;
}



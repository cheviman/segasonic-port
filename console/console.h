/**
 *    Name:        console.h
 *    Description: what the console build tells the port
 *    Version:     0.1.0
 *    Created:     19-10-2026
 *    Modified:    07-11-2026
 *    Author:      Sebastian Toyos
 */
#ifndef SEGASONIC_CONSOLE_H
#define SEGASONIC_CONSOLE_H

#include "console_actions.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CONSOLE_NAME_LENGTH   10          /* MULTIPLAYER > NAME: A-Z, 0-9, '.', '-' and spaces (arcade-lan's) */
#define CONSOLE_DEFAULT_NAME  "PLAYER"

/* The window's settings the console's OPTIONS show and change: the port fills them in (and again when its
   settings change), and applies and saves them when OPTIONS sets 'changed' */
typedef struct
{
	int window_scale;     /* 1-6: the window, in times the game's picture */
	bool full_screen;
	bool smooth;          /* smooth pixels (sharp bilinear), or sharp ones */
	bool bilinear;        /* plain bilinear pixels, softer than smooth ones (over smooth) */
	bool crt;             /* a CRT's look: scanlines and a phosphor mask over smooth pixels (over both) */
	int wide;             /* CONSOLE_WIDE_*: a wider picture, the stages wider (console_wide_now), the rest
	                         between bars; 16:10 the same game, the window's middle narrower (the Steam Deck) */
	bool vsync;           /* VSYNC: each frame shown at the screen's refresh (a computer's) */
	int volume;           /* 0-10: 5 is the game's own */
	int title_music;      /* the sound id of the song the title plays (SOUND TEST picks it) */
	int continues;        /* a game's continues, 0-9, or -1: free play (CONTINUES) */
	int stick_roll;       /* TRACKBALL: trackball counts a frame for a game controller's stick pushed all the way, */
	int key_roll;         /* for an arrow key or the pad's cross held, */
	double mouse_speed;   /* for a pixel of mouse movement */
	bool ball_rolls_on;   /* the mouse turns a ball that keeps rolling when it stops, */
	double ball_drag;     /* which keeps this part of its speed a frame, */
	double ball_friction; /* and loses this many counts a frame more */
	char name[CONSOLE_NAME_LENGTH + 1];    /* MULTIPLAYER > NAME: this device's player's, as the rooms show it */
	bool changed;
} console_window;

extern console_window console_window_settings;

/* WIDE's kinds, as segasonic.ini names them ("off", "16:9", "16:10"; yes is 16:9) and OPTIONS shows them (OFF,
   16:10, 16:9: the narrower first). The game draws the same frame with either: the video's 416 pixels, of which
   the window shows the middle 398 (16:9) or 358 (16:10, the Steam Deck's 1280 x 800) */
#define CONSOLE_WIDE_OFF    0
#define CONSOLE_WIDE_16_9   1
#define CONSOLE_WIDE_16_10  2
#define CONSOLE_WIDE_KINDS  3

/* The actions newly pressed this frame and the ones held (ACTION_BIT), before the game's frame: the
   menus read them */
void console_actions(uint32_t pressed, uint32_t held);

/* The instruments (the sound driver's programs, 0-36) of the SEGA logo's jingle: its voice, and the layer an
   octave under it (-1: none) */
void console_jingle_program(int program, int layer);

/* True once the menu's EXIT was picked: the port closes the game */
bool console_quit_requested(void);

/* Escape outside a game: the title's menu closes (back to PRESS START) */
void console_back(void);

/* OPTIONS > CONTROLS on screen: the text row of 'action''s line (the port draws its keys and buttons
   there), or -1 */
int console_controls_row(int action);

/* The player whose controls CONTROLS shows (0-2): the port draws theirs, and binds and puts back theirs */
int console_controls_player(void);

/* The action CONTROLS waits for a new key or button for, or -1: the port takes the next one pressed, binds it
   (Escape: leaves it) and says so */
int console_controls_waiting(void);
void console_controls_bound(void);

/* DEFAULTS was picked on CONTROLS (true once): the port puts the shown player's keys and buttons back */
bool console_controls_defaults(void);

#define CONSOLE_CONTROLS_KEYS_X     120    /* where the port draws an action's keys, in pixels, */
#define CONSOLE_CONTROLS_SECOND_X   160    /* its second key, */
#define CONSOLE_CONTROLS_BUTTON_X   224    /* and its game controller button */

/* A player's start prompt is up (to join, or a continue's count): their start is the board's start then,
   even in a stage, where it is otherwise the pause */
bool console_start_prompted(int player);

/* The console's changes to the sprite graphics, on the port's copy before the board draws any: Eggman's
   English lines without their red box, white letters with a dark outline ('program': the program ROM, for
   the frames' records) */
void console_prepare_sprites(uint8_t *sprites, size_t size, const uint8_t *program);

/* WIDE draws this frame wider: a stage is played (its tilemaps and objects go on past the sides); the
   other screens' pictures end at their 320 pixels, so they keep them, between black bars */
bool console_wide_now(void);

/* The board's frame drawn (the port's s32_board.frame_shown): s32_video_draw_frame, with WIDE's sides of a
   stage's map gone on where the board's camera never looks */
void console_draw_frame(void);

/* The window's icon, CONSOLE_ICON_SIZE x CONSOLE_ICON_SIZE pixels 0xAARRGGBB: Sonic's, Mighty's and Ray's
   heads (the join chooser's), from the player's sprite graphics and program ROM; false when they are not
   there */
#define CONSOLE_ICON_SIZE 64
bool console_window_icon(uint32_t *pixels, const uint8_t *sprites, size_t size, const uint8_t *program);

/* OPTIONS' backdrop held still, for the port's screen where the picture is not: the page of Sonic heads the
   ranking and OPTIONS show (sonic_head_pattern_script, 32 x 16 cells of 16 x 16 pixels) in their colours
   (nbg0_picture_palette), CONSOLE_BACKDROP_WIDTH x CONSOLE_BACKDROP_HEIGHT pixels 0x00RRGGBB as the video's
   frames, pen 0 black; the page repeats across and down. After s32_init and s32_video_init (the program
   ROM and the tiles) */
#define CONSOLE_BACKDROP_WIDTH  512
#define CONSOLE_BACKDROP_HEIGHT 256
void console_backdrop(uint32_t *pixels);

/* TIME ATTACK's best times, in the board's frames (0: none yet), by the map's levels (the opening, then its 7
   markers'): each level played alone (the opening is never alone: Eggman on the map is the whole game),
   and the best whole game, its time at each level's end (the last is the whole time); the port keeps
   them, and changed says they have changed since */
#define CONSOLE_LEVELS 8

typedef struct
{
	uint32_t levels[CONSOLE_LEVELS];
	uint32_t run[CONSOLE_LEVELS];
	bool changed;
} console_times;

extern console_times console_best_times;

/* Debug (the port's F7 and F8): the camera to the stage's section before (-1) or after (+1) the one it is
   in, as the game's own unused scroll skip mode does */
void console_debug_section(int step);

/* True while a stage is played in a game, its events too (not the intro, player select, the map, the
   continue, the stage clear, nor the attract mode's demos): start pauses it */
bool console_in_game(void);

/* "PAUSE" on the text layer's top (shown), or the cells it covered back as they were; with
   SEGASONIC_PAUSE_EXIT (Android's) its choices too, CONTINUE and EXIT, in the text layer's rows
   CONSOLE_PAUSE_MENU_ROW and two under it */
void console_pause_text(bool shown);

/* ---- MULTIPLAYER, the LAN game (lan.c: arcade-lan, in lockstep) ---- */

#define CONSOLE_LAN_SLOTS   3    /* the cabinet's players: a room's devices in its list's order */
#define CONSOLE_LAN_INPUT   3    /* a player's input at a frame: the trackball's x and y counts (signed bytes), and
                                    CONSOLE_LAN_BUTTON and CONSOLE_LAN_START */
#define CONSOLE_LAN_BUTTON  0x01u
#define CONSOLE_LAN_START   0x02u

/* A LAN game's start (the host's START) or its end (the game over, the host gone, out of step, BACK): at the
   frame's end the port leaves the program and resets the board as at power-on (s32_init, its own EEPROM, the
   sound board), then calls console_lan_power_on, which puts the host's EEPROM over it for a game, and runs the
   program from its start again */
bool console_lan_reset_wanted(void);
void console_lan_power_on(void);

/* In a LAN game: the port gives this device's player's controls each frame (console_lan_frame), and every slot's
   come back, the frame's; false while they have not come: the game holds there (the port shows the frame again,
   with console_lan_waiting's name after a third of a second), and asks again */
bool console_lan_playing(void);
bool console_lan_frame(const uint8_t own[CONSOLE_LAN_INPUT], uint8_t slots[CONSOLE_LAN_SLOTS][CONSOLE_LAN_INPUT]);
const char *console_lan_waiting(void);

/* This device leaves its LAN game (its BACK: the others play on without it) */
void console_lan_leave_game(void);

/* "WAITING FOR name" on the text layer's top (shown), or the cells it covered back, as console_pause_text */
void console_lan_waiting_text(bool shown);

/* The frame a LAN game ran last, and the state's hash the devices compare (the headless runs' --lan-hashes) */
uint32_t console_lan_frame_number(void);
uint64_t console_lan_state_hash(void);

#ifdef SEGASONIC_PAUSE_EXIT
#define CONSOLE_PAUSE_ITEMS     2
#define CONSOLE_PAUSE_CONTINUE  0
#define CONSOLE_PAUSE_EXIT      1
#define CONSOLE_PAUSE_MENU_ROW  12u    /* (of 28: the screen's middle) */

/* The pause's choice in yellow from the next console_pause_text(true) */
void console_pause_choice(int choice);
#endif

#endif

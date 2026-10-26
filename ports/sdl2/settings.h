/**
 *    Name:        settings.h
 *    Description: the window port's settings, kept in sonic.ini beside the executable
 *    Version:     0.1.0
 *    Created:     13-10-2026
 *    Modified:    13-10-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 */
#ifndef SONIC_SDL2_SETTINGS_H
#define SONIC_SDL2_SETTINGS_H

#include <stdbool.h>

typedef struct port_settings
{
	double mouse_speed;       /* trackball counts a pixel of mouse movement */
	bool separate_mice;       /* each mouse is a player's trackball (Windows), the first one player 1's */
	bool ball_rolls_on;       /* the mouse turns a ball that keeps rolling when the mouse stops */
	double ball_drag;         /* a ball rolling on its own keeps this part of its speed a frame ... */
	double ball_friction;     /* ... and loses this many counts a frame more */
	double key_roll;          /* trackball counts a frame for an arrow key held */
	double stick_roll;        /* ... for a controller's stick pushed all the way */
	double window_scale;      /* the window, in times the game's picture */
	bool full_screen;
	bool smooth;              /* smooth pixels (sharp bilinear), or sharp ones */
	bool bilinear;            /* plain bilinear pixels, softer (over smooth) */
	bool crt;                 /* a CRT's look over smooth pixels (over both) */
	bool wide;                /* the console's 16:9 picture */
	double volume;            /* of the game's sound */
	double sega_jingle_program;    /* the console's SEGA logo jingle: the sound driver's program (0-36) */
	double sega_jingle_layer;      /* ... and the layer an octave under it (-1: none) */
	double title_music;            /* the console's title: the sound id of its song */
	double continues;              /* the console's games: their continues (-1: free play) */
} port_settings;

extern port_settings settings;

/* Reads the file at 'path', or writes it with the settings as they are when there is none */
void settings_open(const char *path);

/* Reads the file again when it was saved since it was last read or written; true when it was */
bool settings_reread(void);

/* Writes the settings into the file, keeping its comments and the order of its lines */
void settings_save(void);

#endif

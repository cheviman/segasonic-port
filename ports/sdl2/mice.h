/**
 *    Name:        mice.h
 *    Description: one trackball a mouse: each mouse the system has is told apart (on Windows, by Raw Input)
 *                 and is a player's trackball, the first mouse player 1's, the next to press a button the
 *                 next player's. Without it (another system, or no raw input) the window's one mouse is
 *                 player 1's, as before.
 *    Version:     0.1.0
 *    Created:     26-10-2026
 *    Modified:    26-10-2026
 *
 *    license:BSD-3-Clause
 */
#ifndef SONIC_SDL2_MICE_H
#define SONIC_SDL2_MICE_H

#include <stdbool.h>
#include <stdint.h>

#define MICE_PLAYERS  3      /* the board's trackballs */
#define MICE_DEVICES  32     /* the mice told apart at once */

/* What a mouse did: 'device' is the system's name for it (never 0), its movement in the device's own counts,
   and 'down' the buttons it holds after this (a bit a button: 1 the left, 2 the right, 4 the middle ...) */
void mice_event(uintptr_t device, int dx, int dy, bool buttons_changed, unsigned down);

/* A mouse taken out of the system: its player is free again */
void mice_remove(uintptr_t device);

/* The movement a player's mouse made since the last time (and the count starts again), and the buttons of it
   held now (as 'down' above). False when no mouse is that player's */
bool mice_take(int player, int *dx, int *dy, unsigned *down);

/* Whether the system has told a mouse apart yet: until it has, the window's own mouse events are the mouse */
bool mice_seen(void);

/* All forgotten (before a game; the mice come again with their next movement) */
void mice_reset(void);

#ifdef _WIN32
/* Starts listening to the mice of the SDL window ('window' is an SDL_Window *); false if Windows would not */
bool mice_open(void *window);

/* Once a frame: the window's registration for raw input is put back (SDL's own relative mouse mode takes it
   away when it ends) */
void mice_update(void);

void mice_close(void);
#endif

#endif

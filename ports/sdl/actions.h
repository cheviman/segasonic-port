/**
 *    Name:        actions.h
 *    Description: the keys and game controller buttons of each player's actions (console/console_actions.h),
 *                 which are held and pressed this frame, and controls.ini
 *    Version:     0.1.0
 *    Created:     19-10-2026
 *    Modified:    25-10-2026
 *    Author:      Sebastian Toyos
 */
#ifndef SEGASONIC_PORT_ACTIONS_H
#define SEGASONIC_PORT_ACTIONS_H

#include "console_actions.h"

#include "port_sdl.h"
#include <stdbool.h>
#include <stdint.h>

#define ACTION_PLAYERS  3
#define ACTION_KEYS     4
#define ACTION_BUTTONS  2

typedef struct
{
	const char *name;                                  /* as the settings file says it */
	SDL_Scancode keys[ACTION_KEYS];                    /* SDL_SCANCODE_UNKNOWN: none */
	SDL_GamepadButton buttons[ACTION_BUTTONS];  /* SDL_GAMEPAD_BUTTON_INVALID: none */
} action_binding;

/* Each player's: their keys, and the buttons of their own game controller (the first one plugged in is
   player 1's, the second player 2's...) */
extern action_binding action_bindings[ACTION_PLAYERS][ACTIONS];

/* controls.ini at 'path': read (the defaults where it says nothing), or written when there is none */
void actions_open(const char *path);
void actions_save(void);
void actions_defaults(int player);    /* (and saved) */

/* The key or the game controller button pressed for a player's 'action' on the CONTROLS screen: its first
   one now (and saved) */
void actions_bind_key(int player, int action, SDL_Scancode key);
void actions_bind_button(int player, int action, SDL_GamepadButton button);

/* Once a frame (and while paused): the actions held now and the ones newly held, each player's with their
   keys, their game controller's buttons and its left stick (controllers[player], ACTION_PLAYERS of them) */
void actions_read(SDL_Gamepad *const *controllers);
uint32_t actions_held(void);       /* any player's: the menus' */
uint32_t actions_pressed(void);
uint32_t actions_player_held(int player);
uint32_t actions_player_pressed(int player);

/* A player's keys and game controller buttons alone (one held), without the stick: for the board's own
   controls, which take the sticks apart */
bool actions_holds(int player, SDL_Gamepad *pad, int action);

/* Player 1's actions the touch screen holds now (touch.c), for actions_read to count with their keys */
void actions_touch(uint32_t actions);

/* What was pressed this frame counts no more (a key the CONTROLS screen just took for an action) */
void actions_forget_pressed(void);

#endif

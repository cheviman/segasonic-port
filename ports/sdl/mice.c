/**
 *    Name:        mice.c
 *    Description: one trackball a mouse (mice.h): who is whose, in plain C, with a test of its own
 *                 (tests/mice_test.c). The window port gives it each mouse's movement and buttons as SDL 3 tells
 *                 the mice apart (main.c's mouse_to_mice; until SDL 3, this file heard Windows' WM_INPUT
 *                 messages itself, which SDL 3's own raw input of the relative mode took over).
 *    Version:     0.1.0
 *    Created:     26-10-2026
 *    Modified:    27-11-2026
 *
 *    license:BSD-3-Clause
 */
#include "mice.h"

#include <string.h>

typedef struct
{
	uintptr_t device;     /* 0: not in use */
	int player;           /* -1: not a player's yet */
	unsigned down;
} mouse;

static mouse mice[MICE_DEVICES];
static struct
{
	int dx, dy;
} moved[MICE_PLAYERS];
static bool seen;

static mouse *find(uintptr_t device)
{
	mouse *free_one = NULL;

	for (int index = 0; index < MICE_DEVICES; index++)
	{
		if (mice[index].device == device)
			return &mice[index];
		if (!mice[index].device && !free_one)
			free_one = &mice[index];
	}
	if (free_one)
	{
		free_one->device = device;
		free_one->player = -1;
		free_one->down = 0;
	}
	return free_one;
}

static bool player_has_mouse(int player)
{
	for (int index = 0; index < MICE_DEVICES; index++)
		if (mice[index].device && mice[index].player == player)
			return true;
	return false;
}

/* The first mouse (by its movement or a button) is player 1's, as the window's one mouse always was; the
   others take the next player when they press a button, so that a mouse moved by chance, or the touchpad
   among them, does not take a trackball */
static void assign(mouse *which, bool button_pressed)
{
	bool any = false;

	for (int player = 0; player < MICE_PLAYERS; player++)
		any |= player_has_mouse(player);
	if (!any)
	{
		which->player = 0;
		return;
	}
	if (!button_pressed)
		return;
	for (int player = 0; player < MICE_PLAYERS; player++)
		if (!player_has_mouse(player))
		{
			which->player = player;
			return;
		}
}

void mice_event(uintptr_t device, int dx, int dy, bool buttons_changed, unsigned down)
{
	mouse *which;
	bool pressed;

	if (!device)
		return;
	which = find(device);
	if (!which)
		return;
	seen = true;
	pressed = buttons_changed && (down & ~which->down) != 0;
	if (buttons_changed)
		which->down = down;
	if (which->player < 0 && (pressed || dx || dy))
		assign(which, pressed);
	if (which->player >= 0)
	{
		moved[which->player].dx += dx;
		moved[which->player].dy += dy;
	}
}

void mice_remove(uintptr_t device)
{
	for (int index = 0; index < MICE_DEVICES; index++)
		if (mice[index].device == device)
			memset(&mice[index], 0, sizeof(mice[index]));
}

bool mice_take(int player, int *dx, int *dy, unsigned *down)
{
	unsigned held = 0;

	*dx = *dy = 0;
	*down = 0;
	if (player < 0 || player >= MICE_PLAYERS)
		return false;
	for (int index = 0; index < MICE_DEVICES; index++)
		if (mice[index].device && mice[index].player == player)
			held |= mice[index].down;
	*dx = moved[player].dx;
	*dy = moved[player].dy;
	moved[player].dx = moved[player].dy = 0;
	*down = held;
	return player_has_mouse(player);
}

bool mice_seen(void)
{
	return seen;
}

void mice_reset(void)
{
	memset(mice, 0, sizeof(mice));
	memset(moved, 0, sizeof(moved));
}

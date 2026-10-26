/**
 *    Name:        mice.c
 *    Description: one trackball a mouse (mice.h). The part that decides who is whose is plain C, and has a test
 *                 of its own (tests/mice_test.c); the Windows part below it hears the window's WM_INPUT
 *                 messages through SDL's message hook and gives each mouse's movement and buttons to it.
 *    Version:     0.1.0
 *    Created:     26-10-2026
 *    Modified:    26-10-2026
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

#if defined(_WIN32) && !defined(MICE_TEST)    /* (the test builds the part above alone) */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <SDL.h>
#include <SDL_syswm.h>

#ifndef RIDEV_DEVNOTIFY
#define RIDEV_DEVNOTIFY 0x2000
#endif

static HWND listening;

/* A button's bits in the raw flags: each button's down and up (the five an ordinary mouse has) */
static const struct
{
	USHORT down, up;
} raw_buttons[] = {
	{ RI_MOUSE_BUTTON_1_DOWN, RI_MOUSE_BUTTON_1_UP }, { RI_MOUSE_BUTTON_2_DOWN, RI_MOUSE_BUTTON_2_UP },
	{ RI_MOUSE_BUTTON_3_DOWN, RI_MOUSE_BUTTON_3_UP }, { RI_MOUSE_BUTTON_4_DOWN, RI_MOUSE_BUTTON_4_UP },
	{ RI_MOUSE_BUTTON_5_DOWN, RI_MOUSE_BUTTON_5_UP },
};

static unsigned held_by[MICE_DEVICES * 2];    /* a mouse's buttons between its messages: see device_buttons */
static uintptr_t held_device[MICE_DEVICES * 2];

/* The buttons a device holds, from the changes a message tells of */
static unsigned device_buttons(uintptr_t device, USHORT flags, bool *changed)
{
	int slot = -1;
	unsigned down;

	for (int index = 0; index < MICE_DEVICES * 2; index++)
	{
		if (held_device[index] == device)
		{
			slot = index;
			break;
		}
		if (!held_device[index] && slot < 0)
			slot = index;
	}
	if (slot < 0)
	{
		*changed = false;
		return 0;
	}
	held_device[slot] = device;
	down = held_by[slot];
	*changed = false;
	for (int button = 0; button < (int)(sizeof(raw_buttons) / sizeof(raw_buttons[0])); button++)
	{
		if (flags & raw_buttons[button].down)
		{
			down |= 1u << button;
			*changed = true;
		}
		if (flags & raw_buttons[button].up)
		{
			down &= ~(1u << button);
			*changed = true;
		}
	}
	held_by[slot] = down;
	return down;
}

static void forget_device(uintptr_t device)
{
	for (int index = 0; index < MICE_DEVICES * 2; index++)
		if (held_device[index] == device)
			held_device[index] = 0, held_by[index] = 0;
	mice_remove(device);
}

/* SDL calls this with every message of its windows, before it looks at it */
static void SDLCALL on_message(void *userdata, void *hwnd, unsigned int message, Uint64 wparam, Sint64 lparam)
{
	(void)userdata;
	(void)hwnd;
	if (message == WM_INPUT_DEVICE_CHANGE)
	{
		if (wparam == GIDC_REMOVAL)
			forget_device((uintptr_t)lparam);
		return;
	}
	if (message == WM_INPUT)
	{
		RAWINPUT raw;
		UINT size = sizeof(raw);

		if ((wparam & 0xff) != RIM_INPUT)
			return;    /* (a window that is not in front hears nothing) */
		if (GetRawInputData((HRAWINPUT)(UINT_PTR)lparam, RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER)) == (UINT)-1)
			return;
		if (raw.header.dwType != RIM_TYPEMOUSE)
			return;
		{
			const uintptr_t device = (uintptr_t)raw.header.hDevice;
			const bool absolute = (raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) != 0;    /* (a pen, a remote desktop) */
			bool changed;
			const unsigned down = device_buttons(device, raw.data.mouse.usButtonFlags, &changed);

			mice_event(device, absolute ? 0 : (int)raw.data.mouse.lLastX, absolute ? 0 : (int)raw.data.mouse.lLastY,
				changed, down);
		}
	}
}

static void register_mice(void)
{
	RAWINPUTDEVICE device;

	device.usUsagePage = 0x01;    /* generic desktop */
	device.usUsage = 0x02;        /* mouse */
	device.dwFlags = RIDEV_DEVNOTIFY;
	device.hwndTarget = listening;
	RegisterRawInputDevices(&device, 1, sizeof(device));
}

bool mice_open(void *window)
{
	SDL_SysWMinfo info;

	SDL_VERSION(&info.version);
	if (!window || !SDL_GetWindowWMInfo((SDL_Window *)window, &info) || info.subsystem != SDL_SYSWM_WINDOWS)
		return false;
	listening = info.info.win.window;
	SDL_SetWindowsMessageHook(on_message, NULL);
	register_mice();
	return true;
}

void mice_update(void)
{
	if (listening)
		register_mice();
}

void mice_close(void)
{
	if (listening)
		SDL_SetWindowsMessageHook(NULL, NULL);
	listening = NULL;
}

#endif

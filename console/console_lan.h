/**
 *    Name:        console_lan.h
 *    Description: MULTIPLAYER between the console's pages (console.c) and its LAN game (lan.c)
 *    Version:     0.1.0
 *    Created:     07-11-2026
 *    Modified:    07-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 */
#ifndef SEGASONIC_CONSOLE_LAN_H
#define SEGASONIC_CONSOLE_LAN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* The rooms: HOST (false: no network), JOIN's search, out of a room; NAME changed (the device opened again) */
bool console_lan_host(void);
void console_lan_search(void);
void console_lan_leave(void);
void console_lan_name_changed(void);

bool console_lan_searching(void);
bool console_lan_in_room(void);       /* (joining too) */
bool console_lan_joining(void);
bool console_lan_hosting(void);

/* JOIN's rooms, and one joined (false: full, playing, gone) */
int console_lan_room_count(void);
void console_lan_room_text(int index, char *text, size_t size);
bool console_lan_join(int index);

/* The room's devices, the host first */
int console_lan_member_count(void);
void console_lan_member_text(int index, char *text, size_t size);

/* The host's START: two or three devices; its settings to every device */
bool console_lan_can_start(void);
void console_lan_start(void);

/* What went wrong last ("NO NETWORK", "ROOM FULL", "HOST GONE", "OUT OF STEP", ...: NULL when nothing), until seen */
const char *console_lan_problem(void);
void console_lan_problem_seen(void);

/* Each frame of the MULTIPLAYER pages: the datagrams, and the game's start (console_lan_reset_wanted) */
void console_lan_update(void);

/* After the board's reset that ended a LAN game: true once, the MULTIPLAYER page opens in the title's place */
bool console_lan_page_after(void);

/* A LAN game's: the host's CONTINUES and WIDE (true), in this device's settings' place */
bool console_lan_settings(int *continues, bool *wide);

/* The game over (the attract mode again): the LAN game ends for this device */
void console_lan_game_over(void);

/* The console's state as at power-on (console.c): every page closed, no TIME ATTACK, the SEGA logo not again */
void console_power_on(void);

#endif

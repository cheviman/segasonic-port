/**
 *    Name:        lan.c
 *    Description: MULTIPLAYER, the LAN game: its rooms through arcade-lan, and its game played in lockstep, every
 *                 device the same frames from the same start (TMNT's console/lan.c, for SegaSonic)
 *    Version:     0.1.0
 *    Created:     07-11-2026
 *    Modified:    07-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    Up to three devices, each with its player (the cabinet's three): HOST makes a room, JOIN finds them on the
 *    network, the host's START begins the game (the MULTIPLAYER pages: console.c). Every device then starts from the
 *    same state: the board as at power-on (the port's restart, console_lan_reset_wanted), the console's too
 *    (console_power_on), the host's EEPROM (DIFFICULTY, VITALITY, the ranking), CONTINUES and WIDE, and a game started
 *    as the menu's START GAME starts one, its player 1 the host's. Each frame on, every device gives its player's
 *    trackball, button and start, and runs the frame with every slot's: the room's list order is the cabinet's
 *    players' order. The state's hash (the work RAM and the RAM the sound board shares) before each frame goes to
 *    the host, which compares them. The game over, the host gone, a device out of step or this one leaving (its
 *    BACK): the board as at power-on again, with this device's own EEPROM and settings, and the MULTIPLAYER page.
 *    Without the library (SEGASONIC_LAN not built) the page says NO NETWORK.
 */
#include "console.h"
#include "console_lan.h"

#ifdef SEGASONIC_LAN
#include "arcade_lan.h"    /* (before game.h: ram.h's names are macros) */
#endif
#include "game.h"

#include <stdio.h>
#include <string.h>

#ifdef SEGASONIC_LAN

#define LAN_GAME_VERSION    1          /* the game's datagrams and play: another version's devices are not heard */
#define LAN_BEACON_PORT     27760      /* (TMNT's 27740, OutRunners' 27720) */
#define LAN_DELAY           3          /* frames: a player's controls play 3 frames after they are given */
#define SETTINGS_LAYOUT     1u         /* the START's settings: this layout, CONTINUES, WIDE, the EEPROM */
#define SETTINGS_BYTES      (3 + S32_EEPROM_BYTES)
#define S32_EEPROM_BYTES    ((int)sizeof(s32_board.eeprom.data))
#define LEAVE_AFTER         30         /* a game over's host keeps its room half a second more (lan_update on the
                                          MULTIPLAYER page): a device a frame or two behind takes the game's last
                                          frames and its own game over, not HOST GONE (arcade-lan's LEAVE ends a
                                          guest's game at once) */

static struct
{
	lan_device *device;
	const char *problem;       /* what went wrong last, shown on the MULTIPLAYER page until a line is moved to */
	bool reset_wanted;         /* a LAN game's start or end: the board to be reset for it, at the frame's end */
	bool starting;             /* ... its start (else its end) */
	bool playing;              /* its game, from the reset */
	bool page_after;           /* the reset after a game: the MULTIPLAYER page, not the title */
	bool frame_given;          /* a frame's inputs on the board: its state's hash at the next one */
	uint32_t frame;
	uint8_t settings[LAN_SETTINGS_MOST];
	size_t settings_size;
	int first_slot;
	int continues;             /* the host's CONTINUES and WIDE, while its game goes on */
	bool wide;
	char waiting[LAN_NAME_LENGTH + 1];
	int leave_after;           /* a game over's host: frames before it leaves its room (LEAVE_AFTER) */
	bool starting_unread;      /* a START whose settings this version cannot read */
} lan;

/* Out of the room now (a game over's host too, before its half second) */
static void leave_now(void)
{
	lan.leave_after = 0;
	if (lan.device)
		lan_leave(lan.device);
}

/* The device, opened when first wanted (its name OPTIONS' NAME, the settings') */
static lan_device *device_of(void)
{
	static const lan_config config = { { 'S', 'S', 'O', 'N' }, LAN_GAME_VERSION, LAN_BEACON_PORT, CONSOLE_LAN_INPUT,
		LAN_DELAY, 0, LAN_LOCKSTEP };

	if (!lan.device)
	{
		lan.device = lan_open(&config, console_window_settings.name[0] ? console_window_settings.name
			: CONSOLE_DEFAULT_NAME, 1);
		if (!lan.device)
			lan.problem = "NO NETWORK";
	}
	return lan.device;
}

static void take_problem(void)
{
	const char *problem = lan.device ? lan_problem(lan.device) : NULL;

	if (problem)
		lan.problem = problem;
}

bool console_lan_host(void)
{
	if (lan.leave_after > 0)
		leave_now();
	lan.problem = NULL;
	return device_of() && lan_host(lan.device);
}

void console_lan_search(void)
{
	if (lan.leave_after > 0)
		leave_now();
	lan.problem = NULL;
	if (device_of())
		lan_search(lan.device);
	take_problem();
}

void console_lan_leave(void)
{
	leave_now();
}

/* NAME changed: the device opened again with it, when in no room */
void console_lan_name_changed(void)
{
	if (lan.device && lan_role_of(lan.device) == LAN_OFF)
	{
		lan_close(lan.device);
		lan.device = NULL;
	}
}

bool console_lan_searching(void)
{
	return lan_role_of(lan.device) == LAN_SEARCHING;
}

bool console_lan_in_room(void)
{
	return lan_role_of(lan.device) == LAN_IN_ROOM || lan_role_of(lan.device) == LAN_JOINING;
}

bool console_lan_joining(void)
{
	return lan_role_of(lan.device) == LAN_JOINING;
}

bool console_lan_hosting(void)
{
	return lan_hosting(lan.device);
}

int console_lan_room_count(void)
{
	lan_room rooms[LAN_ROOMS];

	return lan.device ? lan_rooms(lan.device, rooms) : 0;
}

/* A room found, as JOIN lists it: its host's name, its players, PLAYING when its game goes on */
void console_lan_room_text(int index, char *text, size_t size)
{
	lan_room rooms[LAN_ROOMS];
	const int count = lan.device ? lan_rooms(lan.device, rooms) : 0;

	if (index < 0 || index >= count)
	{
		snprintf(text, size, "?");
		return;
	}
	snprintf(text, size, "%-10s %d OF %d%s", rooms[index].name, rooms[index].players, CONSOLE_LAN_SLOTS,
		rooms[index].playing ? " PLAYING" : "");
}

bool console_lan_join(int index)
{
	lan_room rooms[LAN_ROOMS];
	const int count = lan.device ? lan_rooms(lan.device, rooms) : 0;

	if (index < 0 || index >= count || rooms[index].playing || rooms[index].players >= CONSOLE_LAN_SLOTS)
		return false;
	lan.problem = NULL;
	return lan_join(lan.device, &rooms[index]);
}

int console_lan_member_count(void)
{
	lan_member members[LAN_DEVICES];

	return lan.device ? lan_members(lan.device, members) : 0;
}

/* A device of the room, as the room's page lists it: its player's number, its name, HOST, YOU, WAITING when
   unheard; one past the cabinet's three players watches */
void console_lan_member_text(int index, char *text, size_t size)
{
	lan_member members[LAN_DEVICES];
	const int count = lan.device ? lan_members(lan.device, members) : 0;

	if (index < 0 || index >= count)
	{
		snprintf(text, size, "?");
		return;
	}
	if (members[index].first_slot >= CONSOLE_LAN_SLOTS)
		snprintf(text, size, "-- %-10s%s", members[index].name, members[index].own ? " YOU" : "");
	else
		snprintf(text, size, "%dP %-10s%s%s%s", members[index].first_slot + 1, members[index].name,
			index == 0 ? " HOST" : "", members[index].own ? " YOU" : "", members[index].heard ? "" : " WAITING");
}

/* The host's START: two or three devices (the cabinet's players) */
bool console_lan_can_start(void)
{
	const int members = console_lan_member_count();

	return lan_hosting(lan.device) && lan_role_of(lan.device) == LAN_IN_ROOM && members > 1
		&& members <= CONSOLE_LAN_SLOTS;
}

/* The host's START: its settings that change the play, sent to every device (the EEPROM as the board has it now:
   OPTIONS' DIFFICULTY and VITALITY are kept there) */
void console_lan_start(void)
{
	uint8_t settings[SETTINGS_BYTES];

	settings[0] = SETTINGS_LAYOUT;
	settings[1] = (uint8_t)(int8_t)console_window_settings.continues;
	settings[2] = console_window_settings.wide ? 1u : 0u;
	memcpy(settings + 3, s32_board.eeprom.data, S32_EEPROM_BYTES);
	if (console_lan_can_start())
		lan_start(lan.device, settings, sizeof(settings));
}

const char *console_lan_problem(void)
{
	take_problem();
	return lan.problem;
}

void console_lan_problem_seen(void)
{
	lan.problem = NULL;
}

/* Each frame on the MULTIPLAYER pages: the datagrams, and the game's start */
void console_lan_update(void)
{
	if (!lan.device)
		return;
	if (lan.leave_after > 0)
	{
		/* (a game over's host: its last frames sent again a half second, what it hears now no problem of this
		   device's, then out of its room) */
		lan_update(lan.device);
		(void)lan_problem(lan.device);
		if (--lan.leave_after == 0)
			lan_leave(lan.device);
		return;
	}
	lan_update(lan.device);
	take_problem();
	if (lan_game_started(lan.device, lan.settings, &lan.settings_size, &lan.first_slot))
	{
		lan.reset_wanted = true;
		lan.starting = true;
	}
}

/* ---- the game ---- */

bool console_lan_reset_wanted(void)
{
	return lan.reset_wanted;
}

bool console_lan_playing(void)
{
	return lan.playing;
}

bool console_lan_page_after(void)
{
	const bool after = lan.page_after;

	lan.page_after = false;
	return after;
}

bool console_lan_settings(int *continues, bool *wide)
{
	if (!lan.playing)
		return false;
	*continues = lan.continues;
	*wide = lan.wide;
	return true;
}

void console_lan_power_on(void)
{
	const bool starting = lan.starting && lan.settings_size >= SETTINGS_BYTES && lan.settings[0] == SETTINGS_LAYOUT;

	lan.starting_unread = lan.starting && !starting;
	lan.reset_wanted = false;
	lan.starting = false;
	lan.frame_given = false;
	lan.waiting[0] = '\0';
	console_power_on();
	if (!starting)
	{
		/* (the game's end, or a START this version cannot read: this device's own, and the MULTIPLAYER page; out
		   of a room whose START it could not read) */
		if (lan.starting_unread)
		{
			lan.starting_unread = false;
			leave_now();
			lan.problem = "OTHER VERSION";
		}
		if (lan.playing || lan.problem)
			lan.page_after = true;
		lan.playing = false;
		return;
	}
	lan.playing = true;
	lan.page_after = false;
	lan.continues = (int8_t)lan.settings[1];
	lan.wide = lan.settings[2] != 0u;
	memcpy(s32_board.eeprom.data, lan.settings + 3, S32_EEPROM_BYTES);    /* (the host's, over the board's) */
}

/* The game ended for this device ('over': the game over, every device's at once): the board to be reset with its
   own EEPROM and settings, the MULTIPLAYER page; out of the room, a game over's host half a second later */
static void end_game(const char *problem, bool over)
{
	if (over && lan.device && lan_hosting(lan.device))
		lan.leave_after = LEAVE_AFTER;
	else
		leave_now();
	if (problem)
		lan.problem = problem;
	lan.frame_given = false;
	lan.reset_wanted = true;
	lan.starting = false;
	lan.page_after = true;
}

void console_lan_game_over(void)
{
	if (lan.playing && !lan.reset_wanted)
		end_game(NULL, true);
}

void console_lan_leave_game(void)
{
	if (lan.playing && !lan.reset_wanted)
		end_game(NULL, false);
}

/* The state the devices compare: FNV-1a over the work RAM and the RAM the sound board shares */
uint64_t console_lan_state_hash(void)
{
	uint64_t hash = 0xCBF29CE484222325ull;

	for (size_t at = 0; at < sizeof(s32_board.work_ram); at++)
		hash = (hash ^ s32_board.work_ram[at]) * 0x100000001B3ull;
	for (size_t at = 0; at < sizeof(s32_board.shared_ram); at++)
		hash = (hash ^ s32_board.shared_ram[at]) * 0x100000001B3ull;
	return hash;
}

bool console_lan_frame(const uint8_t own[CONSOLE_LAN_INPUT], uint8_t slots[CONSOLE_LAN_SLOTS][CONSOLE_LAN_INPUT])
{
	uint8_t all[LAN_PLAYERS * CONSOLE_LAN_INPUT];
	const char *problem;

	if (!lan.playing || lan.reset_wanted)
	{
		memset(slots, 0, CONSOLE_LAN_SLOTS * CONSOLE_LAN_INPUT);
		return true;
	}
	if (lan.frame_given)
	{
		lan_frame_hash(lan.device, lan.frame, console_lan_state_hash());
		lan.frame_given = false;
	}
	lan_update(lan.device);
	problem = lan_problem(lan.device);
	if (lan_role_of(lan.device) != LAN_PLAYING || (problem && strcmp(problem, "OUT OF STEP") == 0))
	{
		/* (the host gone, or a device out of step: the game ended where it is, for this device) */
		end_game(problem ? problem : "HOST GONE", false);
		memset(slots, 0, CONSOLE_LAN_SLOTS * CONSOLE_LAN_INPUT);
		return true;
	}
	lan_give_inputs(lan.device, own);
	if (!lan_next_frame(lan.device, all))
	{
		if (!lan_waiting(lan.device, lan.waiting))
			lan.waiting[0] = '\0';
		return false;
	}
	lan.waiting[0] = '\0';
	lan.frame = lan_frame_number(lan.device) - 1u;
	lan.frame_given = true;
	memcpy(slots, all, CONSOLE_LAN_SLOTS * CONSOLE_LAN_INPUT);
	return true;
}

uint32_t console_lan_frame_number(void)
{
	return lan.frame;
}

const char *console_lan_waiting(void)
{
	return lan.playing && lan.waiting[0] ? lan.waiting : NULL;
}

#else

/* (no LAN in this build: the page says so) */
bool console_lan_host(void) { return false; }
void console_lan_search(void) { }
void console_lan_leave(void) { }
void console_lan_name_changed(void) { }
bool console_lan_searching(void) { return false; }
bool console_lan_in_room(void) { return false; }
bool console_lan_joining(void) { return false; }
bool console_lan_hosting(void) { return false; }
int console_lan_room_count(void) { return 0; }
void console_lan_room_text(int index, char *text, size_t size) { (void)index; snprintf(text, size, "?"); }
bool console_lan_join(int index) { (void)index; return false; }
int console_lan_member_count(void) { return 0; }
void console_lan_member_text(int index, char *text, size_t size) { (void)index; snprintf(text, size, "?"); }
bool console_lan_can_start(void) { return false; }
void console_lan_start(void) { }
const char *console_lan_problem(void) { return "NO NETWORK"; }
void console_lan_problem_seen(void) { }
void console_lan_update(void) { }
bool console_lan_reset_wanted(void) { return false; }
bool console_lan_playing(void) { return false; }
bool console_lan_page_after(void) { return false; }
bool console_lan_settings(int *continues, bool *wide) { (void)continues; (void)wide; return false; }
void console_lan_power_on(void) { console_power_on(); }
void console_lan_game_over(void) { }
void console_lan_leave_game(void) { }
uint64_t console_lan_state_hash(void) { return 0; }
bool console_lan_frame(const uint8_t own[CONSOLE_LAN_INPUT], uint8_t slots[CONSOLE_LAN_SLOTS][CONSOLE_LAN_INPUT])
{
	(void)own;
	memset(slots, 0, CONSOLE_LAN_SLOTS * CONSOLE_LAN_INPUT);
	return true;
}
uint32_t console_lan_frame_number(void) { return 0; }
const char *console_lan_waiting(void) { return NULL; }

#endif

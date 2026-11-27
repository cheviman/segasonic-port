/**
 *    Name:        actions.c
 *    Description: the keys and game controller buttons of each player's actions, which are held and pressed
 *                 this frame, and the file that keeps them (controls.ini)
 *    Version:     0.1.0
 *    Created:     19-10-2026
 *    Modified:    27-11-2026
 *    Author:      Sebastian Toyos
 *
 *    The defaults below; controls.ini, beside the executable, keeps what the console's CONTROLS screen
 *    changes: a section a player, "[player 1]", and a line an action, "jump = Z, Space | a" (SDL's names of
 *    the keys, then of the buttons). A player's game controller is the one plugged in in their turn; its
 *    left stick counts as its directional pad.
 */
#include "actions.h"

#include <stdio.h>
#include <string.h>

#define NO_KEY     SDL_SCANCODE_UNKNOWN
#define NO_BUTTON  SDL_GAMEPAD_BUTTON_INVALID
#define STICK_DEAD 16000    /* how far the left stick goes before it counts as a direction */
#define LINE_SIZE  256

/* The actions' names in controls.ini */
static const char *const action_names[ACTIONS] = {
	[ACTION_UP] = "up", [ACTION_DOWN] = "down", [ACTION_LEFT] = "left", [ACTION_RIGHT] = "right",
	[ACTION_ACCEPT] = "accept", [ACTION_BACK] = "back", [ACTION_START] = "start", [ACTION_JUMP] = "jump",
};

/* Player 1 the arrows (or WASD), Z or Space and Enter; player 2 IJKL, U and 2; player 3 the keypad's 8456,
   0 and 3; every one their game controller's pad, A, B and Start */
static const SDL_Scancode default_keys[ACTION_PLAYERS][ACTIONS][ACTION_KEYS] = {
	{
		[ACTION_UP] = { SDL_SCANCODE_UP, SDL_SCANCODE_W }, [ACTION_DOWN] = { SDL_SCANCODE_DOWN, SDL_SCANCODE_S },
		[ACTION_LEFT] = { SDL_SCANCODE_LEFT, SDL_SCANCODE_A }, [ACTION_RIGHT] = { SDL_SCANCODE_RIGHT, SDL_SCANCODE_D },
		[ACTION_ACCEPT] = { SDL_SCANCODE_RETURN, SDL_SCANCODE_KP_ENTER, SDL_SCANCODE_Z, SDL_SCANCODE_SPACE },
#ifdef __ANDROID__
		[ACTION_BACK] = { SDL_SCANCODE_ESCAPE, SDL_SCANCODE_BACKSPACE, SDL_SCANCODE_AC_BACK },    /* (its back) */
#else
		[ACTION_BACK] = { SDL_SCANCODE_ESCAPE, SDL_SCANCODE_BACKSPACE },
#endif
		[ACTION_START] = { SDL_SCANCODE_RETURN, SDL_SCANCODE_KP_ENTER, SDL_SCANCODE_P },
		[ACTION_JUMP] = { SDL_SCANCODE_Z, SDL_SCANCODE_SPACE },
	},
	{
		[ACTION_UP] = { SDL_SCANCODE_I }, [ACTION_DOWN] = { SDL_SCANCODE_K },
		[ACTION_LEFT] = { SDL_SCANCODE_J }, [ACTION_RIGHT] = { SDL_SCANCODE_L },
		[ACTION_ACCEPT] = { SDL_SCANCODE_U }, [ACTION_BACK] = { SDL_SCANCODE_O },
		[ACTION_START] = { SDL_SCANCODE_2 }, [ACTION_JUMP] = { SDL_SCANCODE_U },
	},
	{
		[ACTION_UP] = { SDL_SCANCODE_KP_8 }, [ACTION_DOWN] = { SDL_SCANCODE_KP_5 },
		[ACTION_LEFT] = { SDL_SCANCODE_KP_4 }, [ACTION_RIGHT] = { SDL_SCANCODE_KP_6 },
		[ACTION_ACCEPT] = { SDL_SCANCODE_KP_0 }, [ACTION_BACK] = { SDL_SCANCODE_KP_PERIOD },
		[ACTION_START] = { SDL_SCANCODE_3 }, [ACTION_JUMP] = { SDL_SCANCODE_KP_0 },
	},
};

static const SDL_GamepadButton default_buttons[ACTIONS] = {
	[ACTION_UP] = SDL_GAMEPAD_BUTTON_DPAD_UP, [ACTION_DOWN] = SDL_GAMEPAD_BUTTON_DPAD_DOWN,
	[ACTION_LEFT] = SDL_GAMEPAD_BUTTON_DPAD_LEFT, [ACTION_RIGHT] = SDL_GAMEPAD_BUTTON_DPAD_RIGHT,
	[ACTION_ACCEPT] = SDL_GAMEPAD_BUTTON_SOUTH, [ACTION_BACK] = SDL_GAMEPAD_BUTTON_EAST,
	[ACTION_START] = SDL_GAMEPAD_BUTTON_START, [ACTION_JUMP] = SDL_GAMEPAD_BUTTON_SOUTH,
};

action_binding action_bindings[ACTION_PLAYERS][ACTIONS];

static uint32_t held, pressed, player_held[ACTION_PLAYERS], player_pressed[ACTION_PLAYERS];
static uint32_t touched;    /* player 1's actions the touch screen holds (actions_touch) */
static char file_path[1024];

static void set_defaults(int player)
{
	for (int action = 0; action < ACTIONS; action++)
	{
		action_binding *binding = &action_bindings[player][action];

		binding->name = action_names[action];
		for (int key = 0; key < ACTION_KEYS; key++)
			binding->keys[key] = default_keys[player][action][key];    /* (0 is SDL_SCANCODE_UNKNOWN: none) */
		binding->buttons[0] = default_buttons[action];
		for (int button = 1; button < ACTION_BUTTONS; button++)
			binding->buttons[button] = NO_BUTTON;
	}
}

static bool stick_says(SDL_Gamepad *pad, int action)
{
	const int x = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTX);
	const int y = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTY);

	switch (action)
	{
		case ACTION_UP: return y < -STICK_DEAD;
		case ACTION_DOWN: return y > STICK_DEAD;
		case ACTION_LEFT: return x < -STICK_DEAD;
		case ACTION_RIGHT: return x > STICK_DEAD;
		default: return false;
	}
}

bool actions_holds(int player, SDL_Gamepad *pad, int action)
{
	const bool *keys = SDL_GetKeyboardState(NULL);
	const action_binding *binding = &action_bindings[player][action];

	for (int key = 0; key < ACTION_KEYS; key++)
		if (binding->keys[key] != NO_KEY && keys[binding->keys[key]])
			return true;
	for (int button = 0; pad && button < ACTION_BUTTONS; button++)
		if (binding->buttons[button] != NO_BUTTON && SDL_GetGamepadButton(pad, binding->buttons[button]))
			return true;
	return false;
}

void actions_read(SDL_Gamepad *const *controllers)
{
	const bool alt = (SDL_GetModState() & SDL_KMOD_ALT) != 0;    /* (Alt+Enter is full screen, not an action) */
	uint32_t now = 0;

	for (int player = 0; player < ACTION_PLAYERS; player++)
	{
		uint32_t mine = 0;

		for (int action = 0; action < ACTIONS && !alt; action++)
			if (actions_holds(player, controllers[player], action)
				|| (controllers[player] && stick_says(controllers[player], action)))
				mine |= ACTION_BIT(action);
		if (player == 0)
			mine |= touched;
		player_pressed[player] = mine & ~player_held[player];
		player_held[player] = mine;
		now |= mine;
	}
	pressed = now & ~held;
	held = now;
}

void actions_touch(uint32_t actions)
{
	touched = actions;
}

void actions_forget_pressed(void)
{
	pressed = 0;
	for (int player = 0; player < ACTION_PLAYERS; player++)
		player_pressed[player] = 0;
}

uint32_t actions_held(void)
{
	return held;
}

uint32_t actions_pressed(void)
{
	return pressed;
}

uint32_t actions_player_held(int player)
{
	return player_held[player];
}

uint32_t actions_player_pressed(int player)
{
	return player_pressed[player];
}

/* ---- controls.ini ---- */

static char *trimmed(char *text)
{
	char *end;

	while (*text == ' ' || *text == '\t')
		text++;
	end = text + strlen(text);
	while (end > text && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n'))
		*--end = 0;
	return text;
}

/* One line "name = Key, Key | button, button": the player's action it names takes those, in place of its
   own */
static void read_line(int player, char *line)
{
	char *equals = strchr(line, '='), *bar, *part, *next;
	const char *name;
	action_binding *binding;
	int action, slot;

	if (line[0] == ';' || line[0] == '#' || !equals)
		return;
	*equals = 0;
	name = trimmed(line);
	for (action = 0; action < ACTIONS && strcmp(action_names[action], name) != 0; action++)
		;
	if (action == ACTIONS)
		return;
	binding = &action_bindings[player][action];
	bar = strchr(equals + 1, '|');
	if (bar)
		*bar = 0;
	for (slot = 0; slot < ACTION_KEYS; slot++)
		binding->keys[slot] = NO_KEY;
	for (slot = 0; slot < ACTION_BUTTONS; slot++)
		binding->buttons[slot] = NO_BUTTON;
	for (part = equals + 1, slot = 0; part && slot < ACTION_KEYS; part = next)
	{
		next = strchr(part, ',');
		if (next)
			*next++ = 0;
		if (*trimmed(part))
			binding->keys[slot++] = SDL_GetScancodeFromName(trimmed(part));
	}
	for (part = bar ? bar + 1 : NULL, slot = 0; part && slot < ACTION_BUTTONS; part = next)
	{
		next = strchr(part, ',');
		if (next)
			*next++ = 0;
		if (*trimmed(part))
			binding->buttons[slot++] = SDL_GetGamepadButtonFromString(trimmed(part));
	}
}

void actions_open(const char *path)
{
	FILE *file;
	char line[LINE_SIZE];
	int player = 0, number = 0;

	for (int each = 0; each < ACTION_PLAYERS; each++)
		set_defaults(each);
	SDL_strlcpy(file_path, path, sizeof(file_path));
	file = fopen(file_path, "r");
	if (!file)
	{
		actions_save();
		return;
	}
	while (fgets(line, sizeof(line), file))
	{
		/* "[player N]" starts that player's lines (the lines before any are player 1's) */
		if (sscanf(trimmed(line), "[player %d]", &number) == 1)
			player = number >= 1 && number <= ACTION_PLAYERS ? number - 1 : -1;
		else if (player >= 0)
			read_line(player, line);
	}
	fclose(file);
}

void actions_save(void)
{
	FILE *file = fopen(file_path, "w");

	if (!file)
		return;
	fputs("; The console's controls, a section a player: an action a line, its keys (SDL's names), then after |\n"
		"; its game controller buttons. OPTIONS > CONTROLS writes this file; it can be changed by hand too.\n", file);
	for (int player = 0; player < ACTION_PLAYERS; player++)
	{
		fprintf(file, "\n[player %d]\n", player + 1);
		for (int action = 0; action < ACTIONS; action++)
		{
			const action_binding *binding = &action_bindings[player][action];
			bool first = true;

			fprintf(file, "%s =", binding->name);
			for (int key = 0; key < ACTION_KEYS; key++)
				if (binding->keys[key] != NO_KEY)
				{
					fprintf(file, "%s %s", first ? "" : ",", SDL_GetScancodeName(binding->keys[key]));
					first = false;
				}
			fputs(" |", file);
			first = true;
			for (int button = 0; button < ACTION_BUTTONS; button++)
				if (binding->buttons[button] != NO_BUTTON)
				{
					fprintf(file, "%s %s", first ? "" : ",",
						SDL_GetGamepadStringForButton(binding->buttons[button]));
					first = false;
				}
			fputs("\n", file);
		}
	}
	fclose(file);
}

void actions_defaults(int player)
{
	set_defaults(player);
	actions_save();
}

/* The key or button the player pressed for 'action': its first one, the one it had before moving on to the
   second (a key that was already the action's moves to the front) */
void actions_bind_key(int player, int action, SDL_Scancode key)
{
	action_binding *binding = &action_bindings[player][action];
	int slot;

	for (slot = 0; slot < ACTION_KEYS - 1 && binding->keys[slot] != key; slot++)
		;
	memmove(&binding->keys[1], &binding->keys[0], (size_t)slot * sizeof(binding->keys[0]));
	binding->keys[0] = key;
	actions_save();
}

void actions_bind_button(int player, int action, SDL_GamepadButton button)
{
	action_binding *binding = &action_bindings[player][action];

	if (binding->buttons[0] != button)
		binding->buttons[1] = binding->buttons[0];
	binding->buttons[0] = button;
	actions_save();
}

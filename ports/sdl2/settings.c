/**
 *    Name:        settings.c
 *    Description: the window port's settings in sonic.ini: read when the game starts and again whenever the
 *                 file is saved while it runs, written back when a key changes one
 *    Version:     0.1.0
 *    Created:     13-10-2026
 *    Modified:    13-10-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    The file is an INI: "[section]" lines, then "key = value" lines; a line or the end of one that starts
 *    with ';' or '#' is a comment. Numbers take a point or a comma, flags yes or no. A value that cannot be
 *    read leaves its setting as it was, and the line is reported through SDL_Log. Writing the file back only
 *    rewrites the settings' lines, so the comments and the order stay as they were; a setting the file lacks
 *    is added at its end.
 */
#include "settings.h"

#include <SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define LINE_LENGTH 512

port_settings settings = {
	.mouse_speed = 0.35,
	.separate_mice = true,
	.ball_rolls_on = true,
	.ball_drag = 0.98,
	.ball_friction = 0.3,
	.key_roll = 30.0,
	.stick_roll = 30.0,
	.window_scale = 3.0,
	.full_screen = false,
	.smooth = false,
	.bilinear = false,
	.crt = false,
	.wide = false,
	.volume = 1.0,
	.sega_jingle_program = 13.0,
	.sega_jingle_layer = 2.0,
	.title_music = 1.0,
	.continues = 3.0,
};

/* A setting's place in the file and what it takes */
typedef struct entry
{
	const char *section;
	const char *key;
	const char *comment;    /* the lines above the setting when it is added to the file */
	double *number;         /* a number from low to high ... */
	bool *flag;             /* ... or yes or no */
	double low;
	double high;
} entry;

static const entry entries[] = {
	{ "mouse", "speed",
		"trackball counts a pixel of mouse movement (F5 and F6 change it while playing)",
		&settings.mouse_speed, NULL, 0.05, 4.0 },
	{ "mouse", "separate",
		"yes: on Windows each mouse is a trackball of its own, the first one player 1's and the next ones that\n"
		"press a button players 2 and 3, each with its own buttons; no: the mouse is player 1's",
		NULL, &settings.separate_mice, 0.0, 0.0 },
	{ "mouse", "rolls_on",
		"yes: the mouse turns a ball that keeps rolling when the mouse stops or is lifted, as the arcade's\n"
		"trackball does, and the right button holds it; no: the ball stops with the mouse (F4 changes it)",
		NULL, &settings.ball_rolls_on, 0.0, 0.0 },
	{ "mouse", "drag",
		"a ball rolling on its own keeps this part of its speed each frame, from 0 to 1 ...",
		&settings.ball_drag, NULL, 0.0, 1.0 },
	{ "mouse", "friction",
		"... and loses this many counts a frame more (0.98 and 0.3 stop it in about a second from the\n"
		"arrow keys' speed)",
		&settings.ball_friction, NULL, 0.0, 30.0 },
	{ "keyboard", "roll",
		"trackball counts a frame while an arrow key is held (MAME's is 30)",
		&settings.key_roll, NULL, 0.0, 127.0 },
	{ "controller", "roll",
		"trackball counts a frame for the left stick pushed all the way",
		&settings.stick_roll, NULL, 0.0, 127.0 },
	{ "video", "scale",
		"the window's size, in times the game's picture",
		&settings.window_scale, NULL, 1.0, 10.0 },
	{ "video", "full_screen",
		"yes: full screen (F11 or Alt+Enter change it)",
		NULL, &settings.full_screen, 0.0, 0.0 },
	{ "video", "smooth",
		"yes: smooth pixels; no: sharp ones (F3 changes it)",
		NULL, &settings.smooth, 0.0, 0.0 },
	{ "video", "bilinear",
		"yes: plain bilinear pixels, softer than smooth ones (it goes over smooth)",
		NULL, &settings.bilinear, 0.0, 0.0 },
	{ "video", "crt",
		"yes: a CRT's look, scanlines and a phosphor mask over smooth pixels (it goes over smooth and bilinear)",
		NULL, &settings.crt, 0.0, 0.0 },
	{ "sound", "volume",
		"the game's sound, from 0 (none) to 2 (twice as loud)",
		&settings.volume, NULL, 0.0, 2.0 },
#ifdef SEGASONIC_CONSOLE
	{ "sound", "sega_jingle_program",
		"the instrument of the SEGA logo's two notes: one of the sound program's 37 (0 to 36)",
		&settings.sega_jingle_program, NULL, 0.0, 36.0 },
	{ "sound", "sega_jingle_layer",
		"the instrument of the same two notes an octave under them, to give them body (-1: none)",
		&settings.sega_jingle_layer, NULL, -1.0, 36.0 },
	{ "sound", "title_music",
		"the song of the title and its menu, by the game's sound id (1 stage 3's; OPTIONS' SOUND TEST sets it)",
		&settings.title_music, NULL, 0.0, 255.0 },
	{ "video", "wide",
		"yes: a 16:9 picture, the stages wider at the sides, the other screens between black bars (OPTIONS' WIDE)",
		NULL, &settings.wide, 0.0, 0.0 },
	{ "game", "continues",
		"a game's continues after its start, 0 to 9, or -1 for free play (OPTIONS' CONTINUES sets it)",
		&settings.continues, NULL, -1.0, 9.0 },
#endif
};

#define ENTRIES (sizeof(entries) / sizeof(entries[0]))

static char path[1024];
static long long seen_time = -1;    /* when the file was last read or written, and its size then */
static long long seen_size = -1;

/* ---- reading ---- */

/* the text without the spaces around it */
static char *trim(char *text)
{
	char *end;

	while (*text == ' ' || *text == '\t')
		text++;
	end = text + strlen(text);
	while (end > text && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n'))
		end--;
	*end = '\0';
	return text;
}

/* where the comment at the end of a line starts, or NULL */
static char *comment_of(char *text)
{
	char *semicolon = strchr(text, ';');
	char *hash = strchr(text, '#');

	if (!semicolon)
		return hash;
	return hash && hash < semicolon ? hash : semicolon;
}

static const entry *find(const char *section, const char *key)
{
	for (size_t index = 0; index < ENTRIES; index++)
		if (!SDL_strcasecmp(entries[index].section, section) && !SDL_strcasecmp(entries[index].key, key))
			return &entries[index];
	return NULL;
}

/* A line of the file: a section opens (*section takes its name), a setting (*key and *value point into
   'line'), or neither (blank, a comment, or not a setting: *key is NULL) */
static void parse(char *line, char *section, size_t section_size, char **key, char **value)
{
	char *text = trim(line);
	char *equals;

	*key = *value = NULL;
	if (*text == '[')
	{
		char *close = strchr(text, ']');

		if (close)
			*close = '\0';
		SDL_strlcpy(section, trim(text + 1), section_size);
		return;
	}
	if (*text == '\0' || *text == ';' || *text == '#' || !(equals = strchr(text, '=')))
		return;
	*equals = '\0';
	*key = trim(text);
	*value = equals + 1;
	if (comment_of(*value))
		*comment_of(*value) = '\0';
	*value = trim(*value);
}

static bool is_any(const char *text, const char *const *words)
{
	for (; *words; words++)
		if (!SDL_strcasecmp(text, *words))
			return true;
	return false;
}

/* the setting from the text of its value; false when the text is not one */
static bool take(const entry *setting, char *text)
{
	static const char *const yes[] = { "yes", "true", "on", "1", NULL };
	static const char *const no[] = { "no", "false", "off", "0", NULL };
	char *end;
	double number;

	if (setting->flag)
	{
		if (!is_any(text, yes) && !is_any(text, no))
			return false;
		*setting->flag = is_any(text, yes);
		return true;
	}
	for (char *comma = strchr(text, ','); comma; comma = strchr(comma, ','))
		*comma = '.';
	number = strtod(text, &end);
	if (end == text || *end != '\0' || number < setting->low || number > setting->high)
		return false;
	*setting->number = number;
	return true;
}

/* when the file was last changed and its size; false when there is no file */
static bool stamp(long long *when, long long *size)
{
	struct stat status;

	if (stat(path, &status) != 0)
		return false;
	*when = (long long)status.st_mtime;
	*size = (long long)status.st_size;
	return true;
}

static void remember_stamp(void)
{
	if (!stamp(&seen_time, &seen_size))
		seen_time = seen_size = -1;
}

static bool read_file(void)
{
	FILE *input = fopen(path, "r");
	char line[LINE_LENGTH];
	char section[64] = "";
	int number = 0;

	if (!input)
		return false;
	while (fgets(line, sizeof(line), input))
	{
		char *key, *value;
		const entry *setting;

		number++;
		parse(line, section, sizeof(section), &key, &value);
		if (!key)
			continue;
		setting = find(section, key);
		if (!setting)
			SDL_Log("%s:%d: [%s] has no setting '%s'", path, number, section, key);
		else if (!take(setting, value))
			SDL_Log("%s:%d: '%s' is not a value of %s", path, number, value, key);
	}
	fclose(input);
	remember_stamp();
	return true;
}

/* ---- writing ---- */

/* "key = value", then the comment the line ended with, if it had one */
static void write_value(FILE *output, const entry *setting, const char *comment)
{
	if (setting->flag)
		fprintf(output, "%s = %s", setting->key, *setting->flag ? "yes" : "no");
	else
		fprintf(output, "%s = %.4g", setting->key, *setting->number);
	if (comment)
		fprintf(output, " %s", comment);
	fputc('\n', output);
}

/* the setting with its comment above it, each of the comment's lines after a "; " */
static void write_entry(FILE *output, const entry *setting)
{
	const char *line = setting->comment;

	while (*line)
	{
		const char *end = strchr(line, '\n');
		const int length = end ? (int)(end - line) : (int)strlen(line);

		fprintf(output, "; %.*s\n", length, line);
		line += length + (end ? 1 : 0);
	}
	write_value(output, setting, NULL);
}

/* the settings of 'section' the file lacks, at the end of the section */
static void write_missing(FILE *output, const char *section, bool *missing)
{
	for (size_t index = 0; index < ENTRIES; index++)
		if (missing[index] && !SDL_strcasecmp(entries[index].section, section))
		{
			write_entry(output, &entries[index]);
			missing[index] = false;
		}
}

static void free_lines(char **lines, size_t count)
{
	for (size_t index = 0; index < count; index++)
		SDL_free(lines[index]);
	free(lines);
}

void settings_save(void)
{
	FILE *input = fopen(path, "r");
	char **lines = NULL;
	size_t count = 0;
	size_t capacity = 0;
	bool missing[ENTRIES];
	char section[64] = "";
	int blanks = 0;            /* blank lines held back, so that what a section lacks goes before them */
	FILE *output;
	char line[LINE_LENGTH];

	/* the file as it is, to keep what is not a setting, and the settings it has */
	while (input && fgets(line, sizeof(line), input))
	{
		if (count == capacity)
		{
			char **more = (char **)realloc(lines, (capacity ? capacity * 2 : 64) * sizeof(char *));

			if (!more)
				break;
			lines = more;
			capacity = capacity ? capacity * 2 : 64;
		}
		lines[count++] = SDL_strdup(line);
	}
	if (input)
		fclose(input);
	for (size_t index = 0; index < ENTRIES; index++)
		missing[index] = true;
	for (size_t index = 0; index < count; index++)
	{
		char *key, *value;
		const entry *setting;

		SDL_strlcpy(line, lines[index], sizeof(line));
		parse(line, section, sizeof(section), &key, &value);
		if (key && (setting = find(section, key)) != NULL)
			missing[setting - entries] = false;
	}

	output = fopen(path, "w");
	if (!output)
	{
		SDL_Log("%s: the settings cannot be written", path);
		free_lines(lines, count);
		return;
	}
	if (!count)
		fputs("; SegaSonic The Hedgehog: the settings of the game's window.\n"
			"; Read when the game starts, and again whenever this file is saved while the game runs.\n", output);
	section[0] = '\0';
	for (size_t index = 0; index < count; index++)
	{
		char *text, *key, *value;
		const entry *setting;

		SDL_strlcpy(line, lines[index], sizeof(line));
		text = trim(line);
		if (*text == '\0')
		{
			blanks++;
			continue;
		}
		if (*text == '[')
			write_missing(output, section, missing);
		for (; blanks; blanks--)
			fputc('\n', output);
		parse(text, section, sizeof(section), &key, &value);
		setting = key ? find(section, key) : NULL;
		if (setting)
		{
			char *comment = comment_of(strchr(lines[index], '=') + 1);

			write_value(output, setting, comment ? trim(comment) : NULL);
		}
		else
		{
			fputs(lines[index], output);
			if (!strchr(lines[index], '\n'))
				fputc('\n', output);
		}
	}
	free_lines(lines, count);
	write_missing(output, section, missing);
	for (; blanks; blanks--)
		fputc('\n', output);

	/* the sections the file lacks */
	for (size_t index = 0; index < ENTRIES; index++)
		if (missing[index])
		{
			fprintf(output, "\n[%s]\n", entries[index].section);
			write_missing(output, entries[index].section, missing);
		}
	fclose(output);
	remember_stamp();
}

/* ---- the file ---- */

void settings_open(const char *file)
{
	SDL_strlcpy(path, file, sizeof(path));
	if (!read_file())
		settings_save();
}

bool settings_reread(void)
{
	long long when, size;

	if (!stamp(&when, &size) || (when == seen_time && size == seen_size))
		return false;
	return read_file();
}

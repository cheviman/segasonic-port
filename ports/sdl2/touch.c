/**
 *    Name:        touch.c
 *    Description: the touch screen's controls (Android): a stick and the buttons A and START drawn over the
 *                 picture or under it, and the actions the fingers on them hold
 *    Version:     0.1.0
 *    Created:     25-10-2026
 *    Modified:    31-10-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    The stick is where a finger lands on the screen's left half (it waits at the bottom left, dim): it
 *    leans as far as the finger goes from there, all the way at its ring, and it is player 1's left stick
 *    (the trackball, as far as it leans) and directional pad (the menus, past a third of the way). A and
 *    START are where their shapes are on the right; a finger that slides from one to the other presses the
 *    one under it. Back in the menus is Android's own back (actions.c). Places and sizes are in the height
 *    of the controls' area, or its width over 2.2 where that is smaller, the same on every phone (as
 *    OutRunners' touch.c): on a phone, the whole screen, whose sides, where they mostly are, are what the
 *    16:9 picture leaves free; on a screen tall enough for the game's own picture with room under it (an
 *    open Fold's: Sebas) that room, the picture at the top (main.c, touch_layout).
 */
#include "touch.h"

#include "console_actions.h"
#include "letters.h"

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#define FINGERS          10
#define SHAPE_HEIGHT     128     /* the shapes' pictures, scaled to each control */
#define EDGE             0.06    /* a ring's width, in its height */
#define STICK_DIRECTION  0.35    /* a lean that is a direction too, in the ring's radius */
#define BUTTON_MARGIN    0.03    /* a button takes a finger this far past its edge */
#define GLYPH_MIDDLE     7       /* font.bmp's capitals: rows 2 to 11 of their 16 */
#define AREA_ASPECT      2.2     /* the controls' unit: the area's height, or its width over this, the smaller */

typedef struct
{
	bool from_top;            /* its centre: from the right, and from the bottom (from the top: START) */
	double x, y;
	double width, height;     /* (as wide as high: a disc; wider: a pill) */
	uint32_t actions;
	const char *label;
} button;

enum { BUTTON_A, BUTTON_START, BUTTONS };

/* Where the controls are, in the unit: over the picture on a phone's whole screen (their room at its sides
   and corners), or filling the room under it on a tall screen (the stick as large as it lets it). A is as
   large as a thumb needs and no larger, as OutRunners' GAS (Sebas: it was enormous) */
typedef struct
{
	double stick_x, stick_y;     /* the stick's place while it waits: from the left and the bottom */
	double stick_radius;         /* its ring: a lean all the way */
	double knob_radius;
	button buttons[BUTTONS];
} layout;

static const layout over_picture = {
	0.25, 0.27, 0.15, 0.07,
	{
		[BUTTON_A] = { false, 0.17, 0.22, 0.20, 0.20, ACTION_BIT(ACTION_JUMP) | ACTION_BIT(ACTION_ACCEPT), "A" },
		[BUTTON_START] = { true, 0.17, 0.09, 0.24, 0.09, ACTION_BIT(ACTION_START), "START" },
	},
};

static const layout under_picture = {
	0.45, 0.50, 0.36, 0.15,
	{
		[BUTTON_A] = { false, 0.28, 0.40, 0.42, 0.42, ACTION_BIT(ACTION_JUMP) | ACTION_BIT(ACTION_ACCEPT), "A" },
		[BUTTON_START] = { true, 0.30, 0.10, 0.40, 0.14, ACTION_BIT(ACTION_START), "START" },
	},
};

typedef enum { FINGER_FREE, FINGER_STICK, FINGER_BUTTON } finger_role;

static struct
{
	SDL_FingerID id;
	finger_role role;
	int button;               /* (FINGER_BUTTON) the one under it, or -1 */
} fingers[FINGERS];

static bool shown;
static int screen_width = 1, screen_height = 1;    /* the screen, as touch_layout last had it */
static SDL_Rect area = { 0, 0, 1, 1 };              /* the controls': the screen, or the room under the picture */
static double unit = 1.0;                           /* their places and sizes, in pixels */
static const layout *laid = &over_picture;          /* their places there */
static const button *const buttons = over_picture.buttons;    /* (what each button does: the same in both) */
static double stick_x, stick_y;                     /* the stick's centre while a finger holds it */
static double lean_x, lean_y;                       /* -1 to 1 */
static SDL_Texture *disc, *ring, *pill, *pill_ring;
static letters font;
static bool have_font;

static double clamped(double value, double low, double high)
{
	return value < low ? low : (value > high ? high : value);
}

/* The distance from (x, y) to the edge of a pill centred on (0, 0), as wide as it is high a disc:
   negative inside */
static double pill_distance(double x, double y, double width, double height)
{
	const double radius = height / 2;
	const double along = fabs(x) - (width / 2 - radius);    /* (past its straight part) */

	return along > 0 ? hypot(along, y) - radius : fabs(y) - radius;
}

/* A shape in white, its edge smooth: filled, or its ring alone; tinted when it is drawn */
static SDL_Texture *make_shape(SDL_Renderer *renderer, int width, int height, bool edge_only)
{
	SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_ARGB8888);
	const double edge = EDGE * height;
	SDL_Texture *texture;

	if (!surface)
		return NULL;
	for (int y = 0; y < height; y++)
		for (int x = 0; x < width; x++)
		{
			const double distance = pill_distance(x + 0.5 - width / 2.0, y + 0.5 - height / 2.0, width, height);
			const double inside = edge_only ? fabs(distance + edge / 2) - edge / 2 : distance;
			const Uint32 cover = (Uint32)lround(255.0 * clamped(0.5 - inside, 0.0, 1.0));

			((Uint32 *)((Uint8 *)surface->pixels + y * surface->pitch))[x] = cover << 24 | 0xFFFFFFu;
		}
	texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
	if (texture)
	{
		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
		SDL_SetTextureScaleMode(texture, SDL_ScaleModeLinear);
	}
	return texture;
}

void touch_open(SDL_Renderer *renderer, const char *base)
{
	const int pill_width = (int)lround(SHAPE_HEIGHT * buttons[BUTTON_START].width / buttons[BUTTON_START].height);

	SDL_GetRendererOutputSize(renderer, &screen_width, &screen_height);
	touch_layout(screen_width, screen_height, 0);
	disc = make_shape(renderer, SHAPE_HEIGHT, SHAPE_HEIGHT, false);
	ring = make_shape(renderer, SHAPE_HEIGHT, SHAPE_HEIGHT, true);
	pill = make_shape(renderer, pill_width, SHAPE_HEIGHT, false);
	pill_ring = make_shape(renderer, pill_width, SHAPE_HEIGHT, true);
	have_font = letters_load(renderer, base, &font) != 0;
#ifdef SEGASONIC_TOUCH_TEST
	shown = true;    /* (a computer's check: no touch screen) */
#else
	shown = SDL_GetNumTouchDevices() > 0;
#endif
}

void touch_layout(int width, int height, int room)
{
	screen_width = width;
	screen_height = height;
	area = room > 0 ? (SDL_Rect){ 0, height - room, width, room } : (SDL_Rect){ 0, 0, width, height };
	unit = SDL_min(area.h, area.w / AREA_ASPECT);
	laid = room > 0 ? &under_picture : &over_picture;
}

/* A button's centre and size on the screen, in pixels */
static void place(int which, double *x, double *y, double *width, double *height)
{
	const button *it = &laid->buttons[which];

	*x = area.x + area.w - it->x * unit;
	*y = it->from_top ? area.y + it->y * unit : area.y + area.h - it->y * unit;
	*width = it->width * unit;
	*height = it->height * unit;
}

/* The button nearest a finger at (x, y), if it reaches it; else -1 */
static int button_at(double x, double y)
{
	int nearest = -1;
	double nearest_distance = BUTTON_MARGIN * unit;

	for (int which = 0; which < BUTTONS; which++)
	{
		double centre_x, centre_y, width, height, distance;

		place(which, &centre_x, &centre_y, &width, &height);
		distance = pill_distance(x - centre_x, y - centre_y, width, height);
		if (distance <= nearest_distance)
		{
			nearest = which;
			nearest_distance = distance;
		}
	}
	return nearest;
}

/* The finger's slot, or with 'take' a free one for it; -1 when there is none */
static int finger_slot(SDL_FingerID id, bool take)
{
	int free_slot = -1;

	for (int slot = 0; slot < FINGERS; slot++)
	{
		if (fingers[slot].role != FINGER_FREE && fingers[slot].id == id)
			return slot;
		if (free_slot < 0 && fingers[slot].role == FINGER_FREE)
			free_slot = slot;
	}
	return take ? free_slot : -1;
}

static bool stick_held(void)
{
	for (int slot = 0; slot < FINGERS; slot++)
		if (fingers[slot].role == FINGER_STICK)
			return true;
	return false;
}

/* The stick toward a finger at (x, y), a lean all the way at its ring and beyond */
static void lean_to(double x, double y)
{
	const double radius = laid->stick_radius * unit;
	const double length = hypot(x - stick_x, y - stick_y) / radius;
	const double keep = length > 1.0 ? 1.0 / length : 1.0;

	lean_x = (x - stick_x) / radius * keep;
	lean_y = (y - stick_y) / radius * keep;
}

static void finger_down(SDL_FingerID id, double x, double y)
{
	const double radius = laid->stick_radius * unit;
	const int slot = finger_slot(id, true);

	if (slot < 0)
		return;
	fingers[slot].id = id;
	fingers[slot].button = button_at(x, y);
	if (fingers[slot].button >= 0)
		fingers[slot].role = FINGER_BUTTON;
	else if (x < screen_width / 2.0 && !stick_held())
	{
		/* the stick where the finger lands, all of its ring in the controls' area */
		fingers[slot].role = FINGER_STICK;
		stick_x = clamped(x, area.x + radius, area.x + area.w / 2.0);
		stick_y = area.h > 2 * radius ? clamped(y, area.y + radius, area.y + area.h - radius) : area.y + area.h / 2.0;
		lean_to(x, y);
	}
}

void touch_event(const SDL_Event *event)
{
	int slot;

	switch (event->type)
	{
		case SDL_FINGERDOWN:
			shown = true;
			finger_down(event->tfinger.fingerId, event->tfinger.x * screen_width, event->tfinger.y * screen_height);
			break;
		case SDL_FINGERMOTION:
			if ((slot = finger_slot(event->tfinger.fingerId, false)) < 0)
				break;
			if (fingers[slot].role == FINGER_STICK)
				lean_to(event->tfinger.x * screen_width, event->tfinger.y * screen_height);
			else
				fingers[slot].button = button_at(event->tfinger.x * screen_width, event->tfinger.y * screen_height);
			break;
		case SDL_FINGERUP:
			if ((slot = finger_slot(event->tfinger.fingerId, false)) < 0)
				break;
			if (fingers[slot].role == FINGER_STICK)
				lean_x = lean_y = 0.0;
			fingers[slot].role = FINGER_FREE;
			break;
		case SDL_KEYDOWN:
#ifndef SEGASONIC_TOUCH_TEST    /* (a computer's check: the keyboard drives the game to them) */
			if (event->key.keysym.scancode != SDL_SCANCODE_AC_BACK)    /* (Android's back: the screen's too) */
				shown = false;
#endif
			break;
		case SDL_CONTROLLERBUTTONDOWN:
			shown = false;
			break;
		case SDL_CONTROLLERAXISMOTION:
			if (abs(event->caxis.value) > TOUCH_STICK_FULL / 2)
				shown = false;
			break;
		default:
			break;
	}
}

uint32_t touch_held(void)
{
	uint32_t held = 0;

	if (lean_x < -STICK_DIRECTION)
		held |= ACTION_BIT(ACTION_LEFT);
	if (lean_x > STICK_DIRECTION)
		held |= ACTION_BIT(ACTION_RIGHT);
	if (lean_y < -STICK_DIRECTION)
		held |= ACTION_BIT(ACTION_UP);
	if (lean_y > STICK_DIRECTION)
		held |= ACTION_BIT(ACTION_DOWN);
	for (int slot = 0; slot < FINGERS; slot++)
		if (fingers[slot].role == FINGER_BUTTON && fingers[slot].button >= 0)
			held |= buttons[fingers[slot].button].actions;
	return held;
}

void touch_stick(int *x, int *y)
{
	*x = (int)lround(lean_x * TOUCH_STICK_FULL);
	*y = (int)lround(lean_y * TOUCH_STICK_FULL);
}

/* A shape centred on (x, y), its colour and how much it shows */
static void draw_shape(SDL_Renderer *renderer, SDL_Texture *shape, double x, double y, double width, double height,
	Uint8 grey, Uint8 alpha)
{
	const SDL_Rect where = { (int)lround(x - width / 2), (int)lround(y - height / 2), (int)lround(width),
		(int)lround(height) };

	if (!shape)
		return;
	SDL_SetTextureColorMod(shape, grey, grey, grey);
	SDL_SetTextureAlphaMod(shape, alpha);
	SDL_RenderCopy(renderer, shape, NULL, &where);
}

void touch_draw(SDL_Renderer *renderer)
{
	bool stick;
	uint32_t held;
	double radius, knob, centre_x, centre_y;

	if (!shown)
		return;
	stick = stick_held();
	held = touch_held();
	radius = laid->stick_radius * unit;
	knob = laid->knob_radius * unit;
	centre_x = stick ? stick_x : area.x + laid->stick_x * unit;
	centre_y = stick ? stick_y : area.y + area.h - laid->stick_y * unit;
	draw_shape(renderer, disc, centre_x, centre_y, radius * 2, radius * 2, 0, stick ? 70 : 45);
	draw_shape(renderer, ring, centre_x, centre_y, radius * 2, radius * 2, 255, stick ? 150 : 90);
	draw_shape(renderer, disc, centre_x + lean_x * radius, centre_y + lean_y * radius, knob * 2, knob * 2, 255,
		stick ? 170 : 90);
	for (int which = 0; which < BUTTONS; which++)
	{
		const bool pressed = (held & buttons[which].actions) == buttons[which].actions;
		const bool round = buttons[which].width == buttons[which].height;
		double x, y, width, height;

		place(which, &x, &y, &width, &height);
		draw_shape(renderer, round ? disc : pill, x, y, width, height, pressed ? 255 : 0, pressed ? 110 : 80);
		draw_shape(renderer, round ? ring : pill_ring, x, y, width, height, 255, pressed ? 220 : 150);
		if (have_font)
		{
			const int scale = SDL_max(1, (int)lround(height * 0.4 / LETTERS_CELL));
			const SDL_Color white = { 255, 255, 255, 255 };

			SDL_SetTextureAlphaMod(font.sheet, pressed ? 255 : 190);
			letters_draw_at(renderer, &font, buttons[which].label, (int)lround(x),
				(int)lround(y) - GLYPH_MIDDLE * scale, scale, white);
		}
	}
}

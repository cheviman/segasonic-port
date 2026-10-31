/**
 *    Name:        touch.h
 *    Description: the touch screen's controls (Android): a stick and the buttons A and START drawn over the
 *                 picture or under it, and the actions the fingers on them hold
 *    Version:     0.1.0
 *    Created:     25-10-2026
 *    Modified:    31-10-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 */
#ifndef SEGASONIC_PORT_TOUCH_H
#define SEGASONIC_PORT_TOUCH_H

#include <SDL.h>
#include <stdint.h>

#define TOUCH_STICK_FULL 32767    /* the stick leaning all the way, as a game controller's left stick */

/* The controls' pictures for the renderer, their letters from font.bmp in the folder 'base' (with its
   separator); shown from the start where there is a touch screen */
void touch_open(SDL_Renderer *renderer, const char *base);

/* The screen, 'width' x 'height', at a frame, and 'room': the room under the picture at its bottom, in
   pixels, that the controls fill (a tall screen's: the picture at its top); 0, the controls over the picture
   on the whole screen */
void touch_layout(int width, int height, int room);

/* An event: a finger on the controls; a game controller's button or stick, or a key, hides them until a
   finger touches the screen again */
void touch_event(const SDL_Event *event);

/* The actions the fingers hold (ACTION_BIT): the stick's directions, A's accept and jump, START's start */
uint32_t touch_held(void);

/* How far the stick leans, -TOUCH_STICK_FULL to TOUCH_STICK_FULL a side (down and right positive) */
void touch_stick(int *x, int *y);

/* The controls over the picture, before it is presented: nothing while they are hidden */
void touch_draw(SDL_Renderer *renderer);

#endif

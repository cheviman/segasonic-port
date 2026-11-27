/**
 *    Name:        first_run.h
 *    Description: the first run: the game's assets pack from the player's ROM set (first_run.c)
 *    Version:     0.1.0
 *    Created:     20-10-2026
 *    Modified:    20-10-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 */
#ifndef PORT_FIRST_RUN_H
#define PORT_FIRST_RUN_H

/* The assets pack at 'pack' from the player's ROM set: sonic.zip beside the executable ('base') or in the
   folder of what the game keeps ('saves'), else one dropped on a window that asks for it. 1 when the pack
   is written, 0 when the window is closed first */
int first_run(const char *base, const char *saves, const char *pack);

#endif

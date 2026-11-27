/**
 *    Name:        port_sdl.h
 *    Description: SDL for the window port: SDL 3, or its names over the SDL 2 of a device that has no other
 *    Version:     0.1.0
 *    Created:     27-11-2026
 *    Modified:    27-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *
 *    The window port is written for SDL 3. A device whose system has SDL 2 alone (Anbernic's H700 handhelds: their
 *    stock firmware's own SDL 2.0.12, which no SDL 3 runs on) builds the same source with SEGASONIC_SDL2_LAYER:
 *    sdl2_layer.h gives it the SDL 3 names the port uses, each over SDL 2's.
 *
 *    (Not sdl.h: where a file's name is the same in either case, Windows' and macOS's disks, a folder of the
 *    compiler's with it in it would answer SDL 2's own <SDL.h>.)
 */
#ifndef SEGASONIC_PORT_SDL_H
#define SEGASONIC_PORT_SDL_H

#ifdef SEGASONIC_SDL2_LAYER
#include "sdl2_layer.h"
#else
#include <SDL3/SDL.h>
#endif

#endif

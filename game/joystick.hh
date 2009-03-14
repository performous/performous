#ifndef __JOYSTICK_H__
#define __JOYSITCK_H__

#include "SDL.h"
#include "audio.hh"

void check_joystick_event(SDL_Event event, Audio &audio);
void probe();

#endif //__JOYSTICK_H__

#ifdef USE_RAWINPUT
#include "SDL.h"


/// Get raw linux device events and translate them to SDL events
int RawInput_PollEvent(SDL_Event *event) {
	// TODO
	// Check out https://github.com/chriscamacho/gles2framework/blob/master/src/input.c
	// Keyboard and joystick things there
	return 0;
}

#endif

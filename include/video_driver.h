#ifndef __VIDEODRIVER_H__
#define __VIDEODRIVER_H__

#include "../config.h"
#include <vector>
#include <surface.h>

unsigned int screenW();
unsigned int screenH();
static inline float virtH() { return float(screenH()) / screenW(); }

class Window {
  public:
	Window(unsigned int width, unsigned int height, int fullscreen);
	void blank();
	void swap();
	void resize(unsigned int width, unsigned int height);
	void fullscreen() {
		SDL_WM_ToggleFullScreen(screen);
	}
  private:
	SDL_Surface* screen;
	unsigned int m_videoFlags;
};

#endif

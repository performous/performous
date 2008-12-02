#ifndef __VIDEODRIVER_H__
#define __VIDEODRIVER_H__

#include "surface.hh"
#include <boost/ptr_container/ptr_vector.hpp>
#include <vector>

unsigned int screenW();
unsigned int screenH();
static inline float virtH() { return float(screenH()) / screenW(); }

class SDL_Surface;

class Window {
  public:
	Window(unsigned int width, unsigned int height, int fullscreen);
	void blank();
	void swap();
	void resize(unsigned width, unsigned height) {
		if (m_fullscreen) { m_fsW = width; m_fsH = height; }
		else { m_windowW = width; m_windowH = height; }
		resize();
	}
	void resize();
	void fullscreen();
  private:
	SDL_Surface* screen;
	unsigned int m_videoFlags;
	unsigned int m_windowW, m_windowH;
	unsigned int m_fsW, m_fsH;
	bool m_fullscreen;
};

#endif

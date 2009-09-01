#pragma once

#include "surface.hh"
#include <boost/ptr_container/ptr_vector.hpp>
#include <vector>

unsigned int screenW();
unsigned int screenH();
static inline float virtH() { return float(screenH()) / screenW(); }

class SDL_Surface;

/// handles the window
class Window {
  public:
	/// constructor
	Window(unsigned int windowW, unsigned int windowH, bool fullscreen);
	/// destructor
	~Window();
	/// clears window
	void blank();
	/// swaps buffers
	void swap();
	/// resizes window to given dimensions
	/** @param width the new width
	 * @param height the new height
	 */
	void resize(unsigned width, unsigned height) {
		if (!m_fullscreen) { m_windowW = width; m_windowH = height; }
		resize();
	}
	/// does the resizing
	void resize();
	/// sets fullscreen
	/** @param _fs true for fullscreen
	 */
	void setFullscreen(bool _fs);
	/// gets fullscreen state
	bool getFullscreen();

  private:
	SDL_Surface* screen;
	unsigned int m_windowW, m_windowH;
	unsigned int m_fsW, m_fsH;
	bool m_fullscreen;
};


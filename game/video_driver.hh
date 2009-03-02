#ifndef __VIDEODRIVER_H__
#define __VIDEODRIVER_H__

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
	Window(unsigned int width, unsigned int height, int fullscreen, unsigned int fs_width, unsigned int fs_height);
	/// clears window
	void blank();
	/// swaps buffers
	void swap();
	/// resizes window to given dimensions
	/** @param width the new width
	 * @param height the new height
	 */
	void resize(unsigned width, unsigned height) {
		if (m_fullscreen) { m_fsW = width; m_fsH = height; }
		else { m_windowW = width; m_windowH = height; }
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

#endif

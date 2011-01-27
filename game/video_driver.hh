#pragma once

#include "glshader.hh"
#include "glutil.hh"
#include <boost/scoped_ptr.hpp>

unsigned int screenW();
unsigned int screenH();
static inline float virtH() { return float(screenH()) / screenW(); }

struct SDL_Surface;

/// Performs a GL transform for displaying background image at far distance
class FarTransform {
public:
	FarTransform();
private:
	glutil::PushMatrix pm;
};

/// handles the window
class Window {
public:
	/// constructor
	Window(unsigned int windowW, unsigned int windowH, bool fullscreen);
	/// destructor
	~Window();
	/// Setup everything for drawing a view.
	/// @param num should be 0 the first time each frame then incremented for each additional view
	/// @returns true if the view should be rendered, false if no more views are available
	bool view(unsigned num);
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
	/// take a screenshot
	void screenshot();

private:
	SDL_Surface* screen;
	unsigned int m_windowW, m_windowH;
	unsigned int m_fsW, m_fsH;
	bool m_fullscreen;
	boost::scoped_ptr<Shader> m_shader; ///< core shader used for general drawing
};


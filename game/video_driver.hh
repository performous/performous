#pragma once

#include "glshader.hh"
#include "glutil.hh"
#include <boost/function.hpp>
#include <boost/ptr_container/ptr_map.hpp>

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
	void render(boost::function<void (void)> drawFunc);
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

	/// Construct a new shader or return an existing one by name
	Shader& shader(std::string const& name) { return m_shaders[name]; }
private:
	/// Setup everything for drawing a view.
	/// @param num 0 = no stereo, 1 = left eye, 2 = right eye
	void view(unsigned num);
	SDL_Surface* screen;
	unsigned int m_windowW, m_windowH;
	unsigned int m_fsW, m_fsH;
	bool m_fullscreen;
	typedef boost::ptr_map<std::string, Shader> ShaderMap;
	ShaderMap m_shaders; ///< Shader programs by name
};


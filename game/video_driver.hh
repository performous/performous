#pragma once

#include "glmath.hh"
#include "glshader.hh"
#include "glutil.hh"
#include <boost/function.hpp>
#include <boost/ptr_container/ptr_map.hpp>

unsigned int screenW();
unsigned int screenH();
const unsigned int targetWidth = 1366; // One of the most common desktop resolutions in use today.
static inline float virtH() { return float(screenH()) / screenW(); }

struct SDL_Surface;
struct SDL_Window;

struct ColorTrans {
	ColorTrans(Color const& c);
	ColorTrans(glmath::mat4 const& mat);
	~ColorTrans();
private:
	glmath::mat4 m_old;
};

class ViewTrans {
public:
	/// Apply a translation on top of current viewport translation
	ViewTrans(glmath::mat4 const& m);
	/// Apply a subviewport with different perspective projection
	ViewTrans(double offsetX = 0.0, double offsetY = 0.0, double frac = 1.0);
	~ViewTrans();
private:
	glmath::mat4 m_old;
};

/// Apply a transform to current modelview stack
class Transform {
public:
	Transform(glmath::mat4 const& m);
	~Transform();
private:
	glmath::mat4 m_old;
};

/// Performs a GL transform for displaying background image at far distance
glmath::mat4 farTransform();

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
	/// Handler for SDL_VIDEORESIZE event (window resized by the user)
	/** @param width the new width
	 * @param height the new height
	 */
	void resize(unsigned width, unsigned height) {
		if (m_fullscreen) return;  // Ignore window resize events when in fullscreen (gives bogus size on Gnome 3 and others).
		m_windowW = width; m_windowH = height;
		resize();
	}
	/// Resize window (contents) / toggle full screen according to m_fullscreen
	void resize();
	/// sets fullscreen
	/** @param _fs true for fullscreen
	 */
	void setFullscreen(bool _fs);
	bool getFullscreen() const { return m_fullscreen; }
	/// take a screenshot
	void screenshot();

	/// Construct a new shader or return an existing one by name
	Shader& shader(std::string const& name) {
		ShaderMap::iterator it = m_shaders.find(name);
		if (it != m_shaders.end()) return *it->second;
		// const_cast required to workaround ptr_map's protection against construction of temporaries
		return *m_shaders.insert(const_cast<std::string&>(name), new Shader(name)).first->second;
	}
	void updateColor();
	void updateTransforms();
private:
	/// Setup everything for drawing a view.
	/// @param num 0 = no stereo, 1 = left eye, 2 = right eye
	void view(unsigned num);
	void updateStereo(float separation);
	unsigned int m_windowW, m_windowH;
	bool m_fullscreen;
	typedef boost::ptr_map<std::string, Shader> ShaderMap;
	ShaderMap m_shaders; ///< Shader programs by name
	SDL_Window *screen;
};


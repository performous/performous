#pragma once

#include "glmath.hh"
#include "glshader.hh"
#include "glutil.hh"
#include <map>

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
	Window();
	~Window();
	void render(std::function<void (void)> drawFunc);
	/// clears window
	void blank();
	/// swaps buffers
	void swap();
	void event();  ///< Handle window events
	void resize();	/// Resize window (contents) / toggle full screen according to config. Returns true if resized.
	/// take a screenshot
	void screenshot();

	/// Construct a new shader or return an existing one by name
	Shader& shader(std::string const& name) {
		ShaderMap::iterator it = m_shaders.find(name);
		if (it != m_shaders.end()) return *it->second;

		std::pair<std::string, std::unique_ptr<Shader>> kv = std::make_pair(name, std::move(std::make_unique<Shader>(name)));
		auto shaderItr = m_shaders.insert(std::move(kv));
		return *shaderItr.first->second;
	}
	void updateColor();
	void updateTransforms();
	/// Check if resizing (full screen toggle) caused OpenGL context to be lost, in which case textures etc. need reloading.
	bool needReload() { bool tmp = m_needReload; m_needReload = false; return tmp; }
private:
	void setFullscreen();
	/// Setup everything for drawing a view.
	/// @param num 0 = no stereo, 1 = left eye, 2 = right eye
	void view(unsigned num);
	void updateStereo(float separation);
	bool m_fullscreen = false;
	bool m_needResize = true;
	bool m_needReload = true;
	int m_windowX = 0;
	int m_windowY = 0;
	typedef std::map<std::string, std::unique_ptr<Shader>> ShaderMap;
	ShaderMap m_shaders; ///< Shader programs by name
	SDL_Window* screen = nullptr;
};


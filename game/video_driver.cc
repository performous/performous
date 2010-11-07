#include "video_driver.hh"

#include "config.hh"
#include "glutil.hh"
#include "fs.hh"
#include "image.hh"
#include "util.hh"
#include "joystick.hh"
#include <boost/date_time.hpp>
#include <fstream>
#include <SDL.h>


namespace {
	unsigned s_width;
	unsigned s_height;
	/// Attempt to set attribute and report errors.
	/// Tests for success when destoryed.
	struct GLattrSetter {
		GLattrSetter(SDL_GLattr attr, int value): m_attr(attr), m_value(value) {
			if (SDL_GL_SetAttribute(attr, value)) std::clog << "video/warning: Error setting GLattr " << m_attr << std::endl;
		}
		~GLattrSetter() {
			int value;
			SDL_GL_GetAttribute(m_attr, &value);
			if (value != m_value)
				std::clog << "video/warning: Error setting GLattr " << m_attr
				<< ": requested " << m_value << ", got " << value << std::endl;
		}
		SDL_GLattr m_attr;
		int m_value;
	};

}

unsigned int screenW() { return s_width; }
unsigned int screenH() { return s_height; }

Window::Window(unsigned int width, unsigned int height, bool fs): m_windowW(width), m_windowH(height), m_fullscreen(fs) {
	std::atexit(SDL_Quit);
	if( SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK) ==  -1 ) throw std::runtime_error("SDL_Init failed");
	SDL_WM_SetCaption(PACKAGE " " VERSION, PACKAGE);
	{
		SDL_Surface* icon = SDL_LoadBMP(getThemePath("icon.bmp").c_str());
		SDL_WM_SetIcon(icon, NULL);
		SDL_FreeSurface(icon);
	}
	// SDL_SetVideoMode not called yet => get the desktop resolution for fs mode
	SDL_VideoInfo const* vi = SDL_GetVideoInfo();
	m_fsW = vi->current_w;
	m_fsH = vi->current_h;
	resize();
	SDL_ShowCursor(SDL_DISABLE);
	SDL_EnableUNICODE(SDL_ENABLE);
	if (glewInit() != GLEW_OK) throw std::runtime_error("Initializing GLEW failed (is your OpenGL broken?)");
	// Dump some OpenGL info
	std::clog << "video/info: GL_VENDOR:     " << glGetString(GL_VENDOR) << std::endl;
	std::clog << "video/info: GL_VERSION:    " << glGetString(GL_VERSION) << std::endl;
	std::clog << "video/info: GL_RENDERER:   " << glGetString(GL_RENDERER) << std::endl;
	// Extensions would need more complex outputting, otherwise they will break clog.
	//std::clog << "video/info: GL_EXTENSIONS: " << glGetString(GL_EXTENSIONS) << std::endl; 

	input::SDL::init(); // Joysticks etc.
	m_shader.reset(new Shader(getThemePath("shaders/core.vert"), getThemePath("shaders/core.frag"), true));
}

Window::~Window() { }

void Window::blank() {
	glClear(GL_COLOR_BUFFER_BIT);
}

void Window::swap() {
	SDL_GL_SwapBuffers();
}

void Window::setFullscreen(bool _fs) {
	m_fullscreen = _fs;
	resize();
}

bool Window::getFullscreen() {
	return m_fullscreen;
}

void Window::screenshot() {
	Image img;
	img.w = m_fullscreen ? m_fsW : m_windowW;
	img.h = m_fullscreen ? m_fsH : m_windowH;
	img.data.resize(((img.w + 3) & ~3) * img.h * 3);
	img.format = pix::RGB;
	img.reverse = true;
	// Get pixel data from OpenGL
	glReadPixels(0, 0, img.w, img.h, GL_RGB, GL_UNSIGNED_BYTE, &img.data[0]);
	// Compose filename from timestamp
	fs::path filename = getHomeDir() / ("Performous_" + to_iso_string(boost::posix_time::second_clock::local_time()) + ".png");
	// Save to disk
	writePNG(filename.string(), img);
	std::clog << "video/info: Screenshot taken: " << filename << " (" << img.w << "x" << img.h << ")" << std::endl;
}


void Window::resize() {
	unsigned width = m_fullscreen ? m_fsW : m_windowW;
	unsigned height = m_fullscreen ? m_fsH : m_windowH;
	{ // Setup GL attributes for context creation
		GLattrSetter attr_r(SDL_GL_RED_SIZE, 8);
		GLattrSetter attr_g(SDL_GL_GREEN_SIZE, 8);
		GLattrSetter attr_b(SDL_GL_BLUE_SIZE, 8);
		GLattrSetter attr_a(SDL_GL_ALPHA_SIZE, 8);
		GLattrSetter attr_buf(SDL_GL_BUFFER_SIZE, 32);
		GLattrSetter attr_d(SDL_GL_DEPTH_SIZE, 24);
		GLattrSetter attr_s(SDL_GL_STENCIL_SIZE, 8);
		GLattrSetter attr_db(SDL_GL_DOUBLEBUFFER, 1);
		GLattrSetter attr_ar(SDL_GL_ACCUM_RED_SIZE, 0);
		GLattrSetter attr_ag(SDL_GL_ACCUM_GREEN_SIZE, 0);
		GLattrSetter attr_ab(SDL_GL_ACCUM_BLUE_SIZE, 0);
		GLattrSetter attr_aa(SDL_GL_ACCUM_ALPHA_SIZE, 0);
		screen = SDL_SetVideoMode(width, height, 0, SDL_OPENGL | SDL_RESIZABLE | (m_fullscreen ? SDL_FULLSCREEN : 0));
		if (!screen) throw std::runtime_error(std::string("SDL_SetVideoMode failed: ") + SDL_GetError());
	}

	s_width = screen->w;
	s_height = screen->h;
	if (!m_fullscreen) {
		config["graphic/window_width"].i() = s_width;
		config["graphic/window_height"].i() = s_height;
	}
	if (s_height < 0.56f * s_width) s_width = round(s_height / 0.56f);
	if (s_height > 0.8f * s_width) s_height = round(0.8f * s_width);
	glViewport(0.5f * (screen->w - s_width), 0.5f * (screen->h - s_height), s_width, s_height);
	// Set flags
	glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_BLEND);
	// Set projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float h = virtH();
	// stump: under MSVC, near and far are #defined to nothing for compatibility with ancient code, hence the underscores.
	const float near_ = 1.5f; // This determines FOV: the value is your distance from the monitor (the unit being the width of the Performous window)
	const float far_ = 100.0f; // How far away can things be seen
	// Set model-view matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	const float f = 0.9f; // Avoid texture surface being exactly at the near plane (MacOSX fix)
	glFrustum(-0.5f * f, 0.5f * f, 0.5f * h * f, -0.5f * h * f, f * near_, far_);
	glTranslatef(0.0f, 0.0f, -near_);  // So that z = 0.0f is still on monitor surface
	// Check for OpenGL errors
	glutil::GLErrorChecker glerror("Window::resize");
}


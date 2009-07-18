#include "video_driver.hh"

#include "config.hh"
#include "glutil.hh"
#include "screen.hh"
#include "util.hh"
#include "joystick.hh"
#include <SDL.h>

namespace {
	unsigned s_width;
	unsigned s_height;
}

unsigned int screenW() { return s_width; }
unsigned int screenH() { return s_height; }

Window::Window(unsigned int width, unsigned int height, int fs, unsigned int fs_width, unsigned int fs_height): m_windowW(width), m_windowH(height), m_fsW(fs_width), m_fsH(fs_height) {
	std::atexit(SDL_Quit);
	if( SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK) ==  -1 ) throw std::runtime_error("SDL_Init failed");
	SDL_WM_SetCaption(PACKAGE " " VERSION, PACKAGE);
	{
		SDL_Surface* icon = SDL_LoadBMP(getThemePath("icon.bmp").c_str());
		SDL_WM_SetIcon(icon, NULL);
		SDL_FreeSurface(icon);
	}
	m_fullscreen = (fs != 0);
	resize();
	SDL_ShowCursor(SDL_DISABLE);
	SDL_EnableUNICODE(SDL_ENABLE);
	SDL_EnableKeyRepeat(80, 80);
	joysticks_init();
}

Window::~Window() {
	joysticks.clear();
}

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

void Window::resize() {
	unsigned width = m_fullscreen ? m_fsW : m_windowW;
	unsigned height = m_fullscreen ? m_fsH : m_windowH;
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, 0);
	screen = SDL_SetVideoMode(width, height, 0, SDL_OPENGL | SDL_RESIZABLE | (m_fullscreen ? SDL_FULLSCREEN : 0));
	if (!screen) throw std::runtime_error(std::string("SDL_SetVideoMode failed: ") + SDL_GetError());

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
	glEnable(GL_BLEND);
	// Set projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float h = virtH();
	const float near = 1.5f; // This determines FOV: the value is your distance from the monitor (the unit being the width of the Performous window)
	const float far = 100.0f; // How far away can things be seen
	glFrustum(-0.5f, 0.5f, 0.5f * h, -0.5f * h, near, far);
	// Set model-view matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -near);  // So that z = 0.0f is still on monitor surface

}


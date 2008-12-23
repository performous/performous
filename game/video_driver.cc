#include "video_driver.hh"
#include "screen.hh"
#include "config.hh"
#include <cmath>
#include <SDL/SDL.h>

unsigned s_width;
unsigned s_height;

unsigned int screenW() { return s_width; }
unsigned int screenH() { return s_height; }

Window::Window(unsigned int width, unsigned int height, int fs): m_windowW(width), m_windowH(height), m_fsW(width), m_fsH(height) {
	std::atexit(SDL_Quit);
	if( SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK) ==  -1 ) throw std::runtime_error("SDL_Init failed");
	SDL_WM_SetCaption(PACKAGE " " VERSION, PACKAGE);
	{
		SDL_Surface* icon = SDL_LoadBMP(CScreenManager::getSingletonPtr()->getThemePathFile("icon.bmp").c_str());
		SDL_WM_SetIcon(icon, NULL);
		SDL_FreeSurface(icon);
	}
	m_fullscreen = (fs != 0);
	m_videoFlags = SDL_OPENGL | SDL_DOUBLEBUF | SDL_RESIZABLE;
	resize();
	SDL_EnableUNICODE(SDL_ENABLE);
	SDL_EnableKeyRepeat(80, 80);
}

void Window::blank() {
	glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
	glDisable (GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable (GL_BLEND);
	s_width = screen->w;
	s_height = screen->h;
	if (s_height < 0.56f * s_width) s_width = roundf(s_height / 0.56f);
	if (s_height > 0.8f * s_width) s_height = roundf(0.8f * s_width);
	glViewport(0.5f * (screen->w - s_width), 0.5f * (screen->h - s_height), s_width, s_height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float h = virtH();
	glOrtho(-0.5f, 0.5f, 0.5f * h, -0.5f * h, -1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

void Window::swap() {
	SDL_GL_SwapBuffers();
}

void Window::fullscreen() {
	m_fullscreen = !m_fullscreen;
	resize();
}

void Window::resize() {
	unsigned width = m_fullscreen ? m_fsW : m_windowW;
	unsigned height = m_fullscreen ? m_fsH : m_windowH;
	const SDL_VideoInfo* videoInf = SDL_GetVideoInfo();
	screen = SDL_SetVideoMode(width, height, videoInf->vfmt->BitsPerPixel, m_videoFlags | (m_fullscreen ? SDL_FULLSCREEN : 0));
	if (!screen) throw std::runtime_error("SDL_SetVideoMode failed");
}


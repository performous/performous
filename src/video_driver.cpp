#include "video_driver.hh"
#include "screen.hh"
#include <cmath>

unsigned s_width;
unsigned s_height;

unsigned int screenW() { return s_width; }
unsigned int screenH() { return s_height; }

Window::Window(unsigned int width, unsigned int height, int fs) {
	std::atexit(SDL_Quit);
	if( SDL_Init(SDL_INIT_VIDEO) ==  -1 ) throw std::runtime_error("SDL_Init failed");
	SDL_WM_SetCaption(PACKAGE" - "VERSION, "WM_DEFAULT");
#ifdef HAVE_LIBSDL_IMAGE
	SDLSurf icon(sm.getThemePathFile("icon.png"));
	SDL_WM_SetIcon(icon, NULL);
#endif
	m_videoFlags = SDL_OPENGL | SDL_DOUBLEBUF | SDL_RESIZABLE | (fs ? SDL_FULLSCREEN : 0);
	resize(width, height);
	SDL_ShowCursor(SDL_DISABLE);
	SDL_EnableUNICODE(SDL_ENABLE);
	SDL_EnableKeyRepeat(80, 80);
#ifdef DEBUG
	printf ("OpenGL version: %s\n", glGetString (GL_VERSION));
	printf ("OpenGL vendor: %s\n", glGetString (GL_VENDOR));
	printf ("OpenGL renderer: %s\n", glGetString (GL_RENDERER));
#endif
	glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
	glDisable (GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable (GL_BLEND);
}

void Window::blank() {
	s_width = screen->w;
	s_height = screen->h;
	if (s_height < 0.6f * s_width) s_width = roundf(s_height / 0.6f);
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

void Window::resize(unsigned int width, unsigned int height) {
	const SDL_VideoInfo* videoInf = SDL_GetVideoInfo();
	screen = SDL_SetVideoMode(width, height, videoInf->vfmt->BitsPerPixel, m_videoFlags);
	if (!screen) throw std::runtime_error("SDL_SetVideoMode failed");
}


#include "video_driver.hh"
#include "screen.hh"

SDL_Surface* CVideoDriver::init(int width, int height, int fs)
{
	m_videoFlags = SDL_OPENGL | SDL_DOUBLEBUF | SDL_RESIZABLE | (fs ? SDL_FULLSCREEN : 0);
	resize(width, height);
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
	return screen;
}

void CVideoDriver::blank()
{
	glViewport(0, 0, screen->w, screen->h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float ar = float(screen->w) / screen->h;
	glOrtho(-0.5f, 0.5f, 0.5f / ar, -0.5f / ar, -1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

void CVideoDriver::swap()
{
	SDL_GL_SwapBuffers();
}

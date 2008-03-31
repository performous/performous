#include <video_driver.h>
#include <screen.h>

CVideoDriver::CVideoDriver()
{
}

CVideoDriver::~CVideoDriver()
{
}

SDL_Surface * CVideoDriver::init(int width, int height, int fullscreen)
{
	const SDL_VideoInfo * videoInf = SDL_GetVideoInfo();
	unsigned SDL_videoFlags  = SDL_RLEACCEL | SDL_OPENGL | SDL_DOUBLEBUF;
	if (fullscreen) SDL_videoFlags |= SDL_FULLSCREEN;
	screen = SDL_SetVideoMode(width, height, videoInf->vfmt->BitsPerPixel, SDL_videoFlags );

	#ifdef DEBUG
	printf ("OpenGL version: %s\n", glGetString (GL_VERSION));
	printf ("OpenGL vendor: %s\n", glGetString (GL_VENDOR));
	printf ("OpenGL renderer: %s\n", glGetString (GL_RENDERER));
	#endif
	glClearColor (1.0f, 1.0f, 1.0f, 1.0f);
	glDisable (GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable (GL_BLEND);
	glEnable (GL_TEXTURE_RECTANGLE_ARB);
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

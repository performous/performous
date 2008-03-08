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
	glOrtho(0.0f, screen->w, screen->h, 0.0f, -1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

void CVideoDriver::swap()
{
	SDL_GL_SwapBuffers();
}

unsigned int CVideoDriver::initSurface(cairo_surface_t * _surf)
{
	int w = cairo_image_surface_get_width(_surf);
	int h = cairo_image_surface_get_height(_surf);
	texture_list.push_back(new Surface(w, h, Surface::RGBA, cairo_image_surface_get_data(_surf)));
	return texture_list.size() - 1;
}

void CVideoDriver::updateSurface(unsigned int _id, cairo_surface_t* _surf) {
	texture_list.replace(_id, new Surface(_surf));
}

void CVideoDriver::drawSurface(unsigned int _id, int _x, int _y) // Used for lyrics and pitch bars
{
	texture_list[_id].draw(_x, _y, 0.0f);
}



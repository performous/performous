#include <video_driver.h>
#include <screen.h>
#include <sdl_gl.h>

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
	unsigned int texture;
	SDL_GL::initTexture (cairo_image_surface_get_width(_surf),cairo_image_surface_get_height(_surf), &texture, GL_RGBA);
	texture_list.push_back(texture);
	cairo_list.push_back(_surf);
	return cairo_list.size()-1;
}

void CVideoDriver::updateSurface(unsigned int _id, cairo_surface_t * _surf) {
	cairo_list[_id] = _surf;
}

void CVideoDriver::drawSurface(unsigned int _id, int _x, int _y) // Used for lyrics and pitch bars
{
	if (cairo_list[_id] != NULL) {
		int w = cairo_image_surface_get_width(cairo_list[_id]);
		int h = cairo_image_surface_get_height(cairo_list[_id]);
		SDL_GL::draw_func(w,h,cairo_image_surface_get_data(cairo_list[_id]),texture_list[_id], GL_BGRA, _x, _y);
	} else SDL_GL::draw_func(screen->w,screen->h,NULL,texture_list[_id], GL_BGRA, _x, _y);
}

void CVideoDriver::drawSurface(cairo_surface_t* _surf, int _x, int _y) // Used for some texts (song selector, grade)
{
	unsigned int texture;
	int w = cairo_image_surface_get_width(_surf);
	int h = cairo_image_surface_get_height(_surf);
	SDL_GL::initTexture (w,h, &texture, GL_BGRA);
	SDL_GL::draw_func(w,h,cairo_image_surface_get_data(_surf),texture, GL_BGRA, _x, _y);
	SDL_GL::freeTexture(texture);
}

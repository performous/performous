#include <video_driver.h>
#include <screen.h>
#ifdef USE_OPENGL
#include <sdl_gl.h>
#else
#include <cairotosdl.h>
#endif

CVideoDriver::CVideoDriver()
{
}

CVideoDriver::~CVideoDriver()
{
}

SDL_Surface * CVideoDriver::init(int width, int height, int fullscreen)
{
	const SDL_VideoInfo * videoInf = SDL_GetVideoInfo();
	unsigned SDL_videoFlags  = SDL_RLEACCEL;
#ifdef USE_OPENGL
	SDL_videoFlags |= SDL_OPENGL;
	SDL_videoFlags |= SDL_DOUBLEBUF;
#else
	SDL_videoFlags |= videoInf->hw_available ? SDL_HWSURFACE : SDL_SWSURFACE;
	if (videoInf->blit_hw) SDL_videoFlags |= SDL_HWACCEL;
#endif
	if (fullscreen) SDL_videoFlags |= SDL_FULLSCREEN;
	screen = SDL_SetVideoMode(width, height, videoInf->vfmt->BitsPerPixel, SDL_videoFlags );

#ifdef USE_OPENGL
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
#endif
	return screen;
}

void CVideoDriver::blank()
{
#ifdef USE_OPENGL
	glViewport(0, 0, screen->w, screen->h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, screen->w, screen->h, 0.0f, -1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
#else
	SDL_FillRect(screen,NULL,0xffffff);
#endif
}

void CVideoDriver::swap()
{
#ifdef USE_OPENGL
	SDL_GL_SwapBuffers();
#else
	SDL_Flip(screen);
#endif

}

unsigned int CVideoDriver::initSurface(SDL_Surface * _surf)
{
	unsigned int texture;
#ifdef USE_OPENGL
	SDL_GL::initTexture (_surf->w,_surf->h, &texture, GL_RGBA);
#else
	texture = 0;
#endif
	surface_list.push_back(_surf);
	texture_list.push_back(texture);
	cairo_list.push_back(NULL);
	return surface_list.size()-1;
}

unsigned int CVideoDriver::initSurface(cairo_surface_t * _surf)
{
	unsigned int texture;
#ifdef USE_OPENGL
	SDL_GL::initTexture (cairo_image_surface_get_width(_surf),cairo_image_surface_get_height(_surf), &texture, GL_RGBA);
#else
	texture = 0;
#endif
	surface_list.push_back(NULL);
	texture_list.push_back(texture);
	cairo_list.push_back(_surf);
	return surface_list.size()-1;
}

void CVideoDriver::updateSurface(unsigned int _id, SDL_Surface * _surf) {
#ifdef USE_OPENGL
	surface_list[_id] = _surf;
	cairo_list[_id] = NULL;
#else
	if (_surf != NULL) {
		surface_list[_id] = _surf;
		cairo_list[_id] = NULL;
	}
#endif
}

void CVideoDriver::updateSurface(unsigned int _id, cairo_surface_t * _surf) {
#ifdef USE_OPENGL
	cairo_list[_id] = _surf;
	surface_list[_id] = NULL;
#else
	if (_surf != NULL) {
		cairo_list[_id] = _surf;
		surface_list[_id] = NULL;
	}
#endif
}

void CVideoDriver::drawSurface(unsigned int _id, int _x, int _y) // Used for lyrics and pitch bars
{
#ifdef USE_OPENGL
	if (cairo_list[_id] != NULL) {
		int w = cairo_image_surface_get_width(cairo_list[_id]);
		int h = cairo_image_surface_get_height(cairo_list[_id]);
		SDL_GL::draw_func(w,h,cairo_image_surface_get_data(cairo_list[_id]),texture_list[_id], GL_BGRA, _x, _y);
	} else if (surface_list[_id] != NULL)
	  SDL_GL::draw_func(surface_list[_id]->w,surface_list[_id]->h,(unsigned char*)surface_list[_id]->pixels,texture_list[_id], GL_BGRA, _x, _y);
	else
	  SDL_GL::draw_func(screen->w,screen->h,NULL,texture_list[_id], GL_BGRA, _x, _y);
#else
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	SDL_Rect position;
	position.x=_x;
	position.y=_y;
	if (cairo_list[_id] != NULL) {
 		SDL_Surface * SDL_surf = CairoToSdl::BlitToSdl(cairo_list[_id]);
		SDL_BlitSurface(SDL_surf,NULL,sm->getSDLScreen(),&position);
	} else if (surface_list[_id] != NULL)
		SDL_BlitSurface(surface_list[_id],NULL,sm->getSDLScreen(),&position);
#endif
}

void CVideoDriver::drawSurface(SDL_Surface* _surf, int _x, int _y) // Used for rendering cover images and video
{
#ifdef USE_OPENGL
	unsigned int texture;
	GLenum format = (_surf->format->Rmask == 0xFF ? GL_RGBA : GL_BGRA);
	SDL_GL::initTexture (_surf->w,_surf->h, &texture, format);
	SDL_GL::draw_func(_surf->w,_surf->h,(unsigned char*)_surf->pixels,texture, format, _x, _y);
	SDL_GL::freeTexture(texture);
#else
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	SDL_Rect position;
	position.x=_x;
	position.y=_y;
	SDL_BlitSurface(_surf,NULL,sm->getSDLScreen(),&position);
#endif
}

void CVideoDriver::drawSurface(cairo_surface_t* _surf, int _x, int _y) // Used for some texts (song selector, grade)
{
#ifdef USE_OPENGL
	unsigned int texture;
	int w = cairo_image_surface_get_width(_surf);
	int h = cairo_image_surface_get_height(_surf);
	SDL_GL::initTexture (w,h, &texture, GL_BGRA);
	SDL_GL::draw_func(w,h,cairo_image_surface_get_data(_surf),texture, GL_BGRA, _x, _y);
	SDL_GL::freeTexture(texture);
#else
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	SDL_Rect position;
	position.x=_x;
	position.y=_y;
	SDL_Surface * SDL_surf = CairoToSdl::BlitToSdl(_surf);
	SDL_BlitSurface(SDL_surf,NULL,sm->getSDLScreen(),&position);
	SDL_FreeSurface(SDL_surf);
#endif
}

#include <video_driver.h>

CVideoDriver::CVideoDriver()
{
}

CVideoDriver::~CVideoDriver()
{
}

SDL_Surface * CVideoDriver::init(int width, int height)
{
	const SDL_VideoInfo * videoInf = SDL_GetVideoInfo();
	unsigned SDL_videoFlags  = 0;
#ifdef USE_OPENGL
	SDL_videoFlags |= SDL_OPENGL;
	SDL_videoFlags |= SDL_DOUBLEBUF;
	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
#else
	if ( videoInf->hw_available )
		SDL_videoFlags |= SDL_HWSURFACE;
	else
		SDL_videoFlags |= SDL_SWSURFACE;
	if ( videoInf->blit_hw )
		SDL_videoFlags |= SDL_HWACCEL;
#endif

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

void CVideoDriver::blank( void )
{
#ifdef USE_OPENGL
	glClear (GL_COLOR_BUFFER_BIT);
#else
	SDL_FillRect(screen,NULL,0xffffff);
#endif
}

void CVideoDriver::swap( void )
{
#ifdef USE_OPENGL
	SDL_GL_SwapBuffers();
#else
	SDL_Flip(screen);
#endif

}

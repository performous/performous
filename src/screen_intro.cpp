#include <screen_intro.h>
#ifdef USE_OPENGL
#include <sdl_gl.h>
#endif

CScreenIntro::CScreenIntro(char * name)
{
	screenName = name;
	CScreenManager * sm = CScreenManager::getSingletonPtr();

	char * theme_path = new char[1024];
	sm->getThemePathFile(theme_path,"intro.svg");
	cairo_svg = new CairoSVG(theme_path,sm->getWidth(),sm->getHeight());
	delete[] theme_path;
#ifdef USE_OPENGL
        SDL_GL::initTexture (sm->getWidth(),sm->getHeight(), &texture, GL_BGRA);
#endif
}

CScreenIntro::~CScreenIntro()
{
	delete cairo_svg;
}

void CScreenIntro::manageEvent( SDL_Event event )
{
	int keypressed;
	switch(event.type) {
		case SDL_KEYDOWN:
			keypressed = event.key.keysym.sym;
			if( keypressed == SDLK_ESCAPE || keypressed == SDLK_q ) {
				CScreenManager::getSingletonPtr()->finished();
			} else if( keypressed == SDLK_s ) {
				CScreenManager::getSingletonPtr()->setSongs(new CSongs() );
				CScreenManager::getSingletonPtr()->getSongs()->sortByArtist();
				CScreenManager::getSingletonPtr()->activateScreen("Songs");
			}
	}
}

void CScreenIntro::draw( void )
{
	CScreenManager * sm = CScreenManager::getSingletonPtr();
#ifdef USE_OPENGL
        glClear (GL_COLOR_BUFFER_BIT);
        SDL_GL::draw_func(  sm->getWidth(),
                            sm->getHeight(),
                            (unsigned char *) cairo_svg->getSDLSurface()->pixels,
                            texture, GL_BGRA);

#else
	SDL_BlitSurface(cairo_svg->getSDLSurface(),NULL,sm->getSDLScreen(),NULL);
#endif
}

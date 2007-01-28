#include <screen_intro.h>

CScreenIntro::CScreenIntro(char * name)
{
	screenName = name;

	cairo_svg = new CairoSVG(THEMES_DIR "/default/intro.svg",CScreenManager::getSingletonPtr()->getWidth(),CScreenManager::getSingletonPtr()->getHeight());
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

	SDL_BlitSurface(cairo_svg->getSDLSurface(),NULL,sm->getSDLScreen(),NULL);
}

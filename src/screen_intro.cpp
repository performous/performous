#include <screen_intro.h>

CScreenIntro::CScreenIntro(char * name)
{
	screenName = name;
	CScreenManager * sm = CScreenManager::getSingletonPtr();

	char * theme_path = new char[1024];
	sm->getThemePathFile(theme_path,"intro.svg");
	cairo_svg = new CairoSVG(theme_path,sm->getWidth(),sm->getHeight());
	delete[] theme_path;
	texture = sm->getVideoDriver()->initSurface(cairo_svg->getSDLSurface());
}

CScreenIntro::~CScreenIntro()
{
	delete cairo_svg;
        delete video_driver;
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
	sm->getVideoDriver()->drawSurface(texture);
}

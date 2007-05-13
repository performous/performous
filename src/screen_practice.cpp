#include <screen_practice.h>

CScreenPractice::CScreenPractice(char * name)
{
	screenName = name;

}

CScreenPractice::~CScreenPractice()
{
}

void CScreenPractice::enter( void )
{
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	char * theme_path = new char[1024];
	sm->getThemePathFile(theme_path,"practice_bg.svg");
	cairo_svg = new CairoSVG(theme_path,sm->getWidth(),sm->getHeight());
	delete[] theme_path;
	texture = sm->getVideoDriver()->initSurface(cairo_svg->getSDLSurface());
}

void CScreenPractice::exit( void )
{
	delete cairo_svg;
}

void CScreenPractice::manageEvent( SDL_Event event )
{
	int keypressed;
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	switch(event.type) {
		case SDL_KEYDOWN:
			keypressed = event.key.keysym.sym;
			if( keypressed == SDLK_ESCAPE || keypressed == SDLK_q ) {
				sm->activateScreen("Intro");
			}
	}
}

void CScreenPractice::draw( void )
{
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	sm->getVideoDriver()->drawSurface(texture);
}

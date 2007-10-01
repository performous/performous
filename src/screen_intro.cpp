#include <screen_intro.h>

CScreenIntro::CScreenIntro(const char * name, unsigned int width, unsigned int height):CScreen(name,width,height)
{
	CScreenManager * sm = CScreenManager::getSingletonPtr();

	cairo_svg = new CairoSVG(sm->getThemePathFile("intro.svg").c_str(), width, height);
	texture = sm->getVideoDriver()->initSurface(cairo_svg->getSDLSurface());
}

CScreenIntro::~CScreenIntro()
{
	delete cairo_svg;
}

void CScreenIntro::enter( void )
{
}

void CScreenIntro::exit( void )
{
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
				CScreenManager::getSingletonPtr()->activateScreen("Songs");
			} else if( keypressed == SDLK_c ) {
				CScreenManager::getSingletonPtr()->activateScreen("Configuration");
			} else if( keypressed == SDLK_p ) {
				CScreenManager::getSingletonPtr()->activateScreen("Practice");
			}
	}
}

void CScreenIntro::draw( void )
{
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	sm->getVideoDriver()->drawSurface(texture);
}

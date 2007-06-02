#include <screen_configuration.h>

CScreenConfiguration::CScreenConfiguration(char * name)
{
	screenName = name;
	configuration.push_back(new CConfigurationFullscreen());
	selected=0;
}

CScreenConfiguration::~CScreenConfiguration()
{
	for( unsigned int i = 0 ; i < configuration.size() ; i++ )
		delete configuration[i];
}

void CScreenConfiguration::enter( void )
{
}

void CScreenConfiguration::exit( void )
{
}

void CScreenConfiguration::manageEvent( SDL_Event event )
{
	int keypressed;
	SDLMod modifier;

	switch(event.type) {
		case SDL_KEYDOWN:
			keypressed = event.key.keysym.sym;
			modifier   = event.key.keysym.mod;

			if( keypressed == SDLK_ESCAPE ) {
				CScreenManager::getSingletonPtr()->activateScreen("Intro");
			} else if( keypressed == SDLK_LEFT ) {
				configuration[selected]->setPrevious();
			} else if( keypressed == SDLK_RIGHT ) {
				configuration[selected]->setNext();
			} else if( keypressed == SDLK_UP ) {
				if( selected > 0 )
					selected--;
			} else if( keypressed == SDLK_DOWN ) {
				if( selected < configuration.size() - 1  )
					selected++;
			}
	}
}

void CScreenConfiguration::draw( void )
{
}

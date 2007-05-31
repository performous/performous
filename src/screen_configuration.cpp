#include <screen_configuration.h>

CScreenConfiguration::CScreenConfiguration(char * name)
{
	screenName = name;
	configuration.push_back(new CConfigurationFullscreen());
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
			}
	}
}

void CScreenConfiguration::draw( void )
{
}

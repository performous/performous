#include <screen_intro.h>
#include <SDL/SDL_ttf.h>

CScreenIntro::CScreenIntro(char * name)
{
	screenName = name;
	cursor=0;
	SDL_Color black = {0, 0, 0,0};
	TTF_Font *font = TTF_OpenFont("fonts/arial.ttf", 65);
	SDL_Surface *title = TTF_RenderUTF8_Blended(font, "UltraStar NG", black);
	titleTex = new CSdlTexture(title);
	
	SDL_FreeSurface(title);
	TTF_CloseFont(font);
}

CScreenIntro::~CScreenIntro()
{
	delete titleTex;
}

void CScreenIntro::manageEvent( SDL_Event event )
{
	int keypressed;
	switch(event.type) {
		case SDL_KEYDOWN:
			keypressed = event.key.keysym.sym;
			if( keypressed == SDLK_ESCAPE || keypressed == SDLK_q ) {
				CScreenManager::getSingletonPtr()->finished();
			} else if( keypressed == SDLK_LEFT ) {
				cursor--;
				fprintf(stdout,"cursor--\n");
			} else if( keypressed == SDLK_RIGHT ) {
				cursor++;
				fprintf(stdout,"cursor++\n");
			} else if( keypressed == SDLK_s ) {
				CScreenManager::getSingletonPtr()->setSongs(new CSongs() );
				CScreenManager::getSingletonPtr()->activateScreen("Songs");
			}
	}
}

void CScreenIntro::draw( void )
{
	glColor4f(1.0,1.0,1.0,1.0);
	titleTex->draw( 200 , 0 , 400 , 100 );
}

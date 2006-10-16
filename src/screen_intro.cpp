#include <screen_intro.h>
#include <SDL/SDL_ttf.h>

CScreenIntro::CScreenIntro(char * name)
{
	screenName = name;
	cursor=0;
	SDL_Color black = {0, 0, 0,0};
	TTF_Font *font = TTF_OpenFont("fonts/arial.ttf", 65);
	title = TTF_RenderUTF8_Blended(font, "UltraStar NG", black);
	
	TTF_CloseFont(font);
}

CScreenIntro::~CScreenIntro()
{
	SDL_FreeSurface(title);
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
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	SDL_Rect position;
	position.x=(sm->getWidth()-title->w)/2;
	position.y=0;
	SDL_BlitSurface(title, NULL,  sm->getSDLScreen(), &position);
}

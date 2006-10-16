#include <screen_songs.h>
#include <SDL/SDL_ttf.h>

CScreenSongs::CScreenSongs(char * name)
{
	screenName = name;
	songId=0;

	SDL_Color black = {0, 0, 0,0};
	TTF_Font *font = TTF_OpenFont("fonts/arial.ttf", 65);
	title = TTF_RenderUTF8_Blended(font, "Choose your song", black);
	
	TTF_CloseFont(font);
}

CScreenSongs::~CScreenSongs()
{
	SDL_FreeSurface(title);
}

void CScreenSongs::manageEvent( SDL_Event event )
{
	int keypressed;
	switch(event.type) {
		case SDL_KEYDOWN:
			keypressed = event.key.keysym.sym;
			if( keypressed == SDLK_ESCAPE || keypressed == SDLK_q ) {
				CScreenManager::getSingletonPtr()->activateScreen("Intro");
			} else if( keypressed == SDLK_LEFT ) {
				if(CScreenManager::getSingletonPtr()->getSongId() >0 )
					CScreenManager::getSingletonPtr()->setSongId(CScreenManager::getSingletonPtr()->getSongId()-1);
				else
					CScreenManager::getSingletonPtr()->setSongId(CScreenManager::getSingletonPtr()->getSongs()->nbSongs()-1);
			} else if( keypressed == SDLK_RIGHT ) {
				if(CScreenManager::getSingletonPtr()->getSongId() > CScreenManager::getSingletonPtr()->getSongs()->nbSongs()-2 )
					CScreenManager::getSingletonPtr()->setSongId(0);
				else
					CScreenManager::getSingletonPtr()->setSongId(CScreenManager::getSingletonPtr()->getSongId()+1);
			} else if( keypressed == SDLK_UP ) {
				switch(CScreenManager::getSingletonPtr()->getSongs()->getOrder()) {
					case 0:
						CScreenManager::getSingletonPtr()->getSongs()->sortByArtist();
						break;
					case 1:
						CScreenManager::getSingletonPtr()->getSongs()->sortByEdition();
						break;
					case 2:
						CScreenManager::getSingletonPtr()->getSongs()->sortByGenre();
						break;
					case 3:
						CScreenManager::getSingletonPtr()->getSongs()->sortByTitle();
						break;
				}
			} else if( keypressed == SDLK_DOWN ) {
				switch(CScreenManager::getSingletonPtr()->getSongs()->getOrder()) {
					case 0:
						CScreenManager::getSingletonPtr()->getSongs()->sortByGenre();
						break;
					case 1:
						CScreenManager::getSingletonPtr()->getSongs()->sortByTitle();
						break;
					case 2:
						CScreenManager::getSingletonPtr()->getSongs()->sortByArtist();
						break;
					case 3:
						CScreenManager::getSingletonPtr()->getSongs()->sortByEdition();
						break;
				}
			} else if( keypressed == SDLK_RETURN ) {
				CScreenManager::getSingletonPtr()->activateScreen("Sing");
			}
	}
}

char * order[4] = {
	"Order by edition",
	"Order by genre",
	"Order by title",
	"Order by artist",
};

void CScreenSongs::draw( void )
{
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	SDL_Rect position;

	// Draw the title
	position.x=(sm->getWidth()-title->w)/2;
	position.y=0;
	SDL_BlitSurface(title, NULL,  sm->getSDLScreen(), &position);
	// Draw the cover
	position.x=(sm->getWidth()-sm->getSong()->coverSurf->w)/2;
	position.y=200;
	SDL_BlitSurface(sm->getSong()->coverSurf,NULL,sm->getSDLScreen(), &position);
	// Draw the "Order by" text
	SDL_Color black = {0,0,0,0};
	TTF_Font *font = TTF_OpenFont("fonts/arial.ttf", 25);
	char * my_order = order[sm->getSongs()->getOrder()];
	SDL_Surface * orderSurf = TTF_RenderUTF8_Blended(font, my_order , black);
	position.x=(sm->getWidth()-sm->getSong()->coverSurf->w)/2;
	position.y=500;
	SDL_BlitSurface(orderSurf, NULL,  sm->getSDLScreen(), &position);

	SDL_FreeSurface(orderSurf);
	TTF_CloseFont(font);
}

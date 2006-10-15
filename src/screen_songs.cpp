#include <screen_songs.h>
#include <SDL/SDL_ttf.h>

CScreenSongs::CScreenSongs(char * name)
{
	screenName = name;
	songId=0;

	SDL_Color black = {0, 0, 0,0};
	TTF_Font *font = TTF_OpenFont("fonts/arial.ttf", 65);
	SDL_Surface *title = TTF_RenderUTF8_Blended(font, "Choose your song", black);
	titleTex = new CSdlTexture(title);
	
	SDL_FreeSurface(title);
	TTF_CloseFont(font);
}

CScreenSongs::~CScreenSongs()
{
	delete titleTex;
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
	glColor4f(1.0,1.0,1.0,1.0);
	titleTex->draw( 200 , 0 , 400 , 100 );
	CScreenManager::getSingletonPtr()->getSong()->coverTex->draw(300,200,200,200);
	
	SDL_Color black = {0,0,0,0};

	TTF_Font *font = TTF_OpenFont("fonts/arial.ttf", 25);
	char * my_order;

	my_order = order[CScreenManager::getSingletonPtr()->getSongs()->getOrder()];
	
	SDL_Surface * orderSurf = TTF_RenderUTF8_Blended(font, my_order , black);
	CSdlTexture * orderTex = new CSdlTexture(orderSurf);
	orderTex->draw(300,550);
	SDL_FreeSurface(orderSurf);
	TTF_CloseFont(font);

	delete orderTex;
}

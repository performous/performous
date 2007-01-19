#include <screen_songs.h>

CScreenSongs::CScreenSongs(char * name)
{
	screenName = name;
	songId=0;

	cairo_svg = new CairoSVG(THEMES_DIR "/default/songs.svg",800,600);
}

CScreenSongs::~CScreenSongs()
{
	delete cairo_svg;
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

	SDL_BlitSurface(cairo_svg->getSDLSurface(),NULL,sm->getSDLScreen(),NULL);
	
	// Draw the cover
	position.x=(sm->getWidth()-sm->getSong()->coverSurf->w)/2;
	position.y=(sm->getHeight()-sm->getSong()->coverSurf->h)/2;
	SDL_BlitSurface(sm->getSong()->coverSurf,NULL,sm->getSDLScreen(), &position);
	// Draw the "Order by" text
	SDL_Color black = {0,0,0,0};
	TTF_Font *font = TTF_OpenFont(FONTS_DIR "/DejaVuSansCondensed.ttf", 25);

	SDL_Surface * artistSurf = TTF_RenderUTF8_Blended(font, sm->getSong()->artist , black);
	position.x=(sm->getWidth()-artistSurf->w)/2;
	position.y=475;
	SDL_BlitSurface(artistSurf, NULL,  sm->getSDLScreen(), &position);
	SDL_FreeSurface(artistSurf); 

	SDL_Surface * titleSurf = TTF_RenderUTF8_Blended(font, sm->getSong()->title , black);
	position.x=(sm->getWidth()-titleSurf->w)/2;
	position.y=500;
	SDL_BlitSurface(titleSurf, NULL,  sm->getSDLScreen(), &position);
	SDL_FreeSurface(titleSurf); 

	char * my_order = order[sm->getSongs()->getOrder()];
	SDL_Surface * orderSurf = TTF_RenderUTF8_Blended(font, my_order , black);
	position.x=(sm->getWidth()-orderSurf->w)/2;
	position.y=550;
	SDL_BlitSurface(orderSurf, NULL,  sm->getSDLScreen(), &position);
	SDL_FreeSurface(orderSurf);

	TTF_CloseFont(font);
}

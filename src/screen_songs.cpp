#include <screen_songs.h>
#include <cairotosdl.h>
#ifdef USE_OPENGL
#include <sdl_gl.h>
#endif

CScreenSongs::CScreenSongs(char * name)
{
	screenName = name;
	songId=0;

	theme = new CThemeSongs();
#ifdef USE_OPENGL
        SDL_GL::initTexture (CScreenManager::getSingletonPtr()->getWidth(),CScreenManager::getSingletonPtr()->getHeight(), &bg_texture, GL_BGRA);
        SDL_GL::initTexture (CScreenManager::getSingletonPtr()->getWidth(),CScreenManager::getSingletonPtr()->getHeight(), &theme_texture, GL_BGRA);
#endif
}

CScreenSongs::~CScreenSongs()
{
	delete theme;
}

void CScreenSongs::manageEvent( SDL_Event event )
{
	int keypressed;
	CScreenManager * sm = CScreenManager::getSingletonPtr();

	switch(event.type) {
		case SDL_KEYDOWN:
			keypressed = event.key.keysym.sym;
			if( keypressed == SDLK_ESCAPE || keypressed == SDLK_q ) {
				sm->activateScreen("Intro");
			} else if( keypressed == SDLK_LEFT ) {
				if(sm->getSongId() >0 )
					sm->setSongId(sm->getSongId()-1);
				else
					sm->setSongId(sm->getSongs()->nbSongs()-1);
			} else if( keypressed == SDLK_RIGHT ) {
				if(sm->getSongId() > sm->getSongs()->nbSongs()-2 )
					sm->setSongId(0);
				else
					sm->setSongId(sm->getSongId()+1);
			} else if( keypressed == SDLK_UP ) {
				switch(sm->getSongs()->getOrder()) {
					case 0:
						sm->getSongs()->sortByArtist();
						break;
					case 1:
						sm->getSongs()->sortByEdition();
						break;
					case 2:
						sm->getSongs()->sortByGenre();
						break;
					case 3:
						sm->getSongs()->sortByTitle();
						break;
				}
			} else if( keypressed == SDLK_DOWN ) {
				switch(sm->getSongs()->getOrder()) {
					case 0:
						sm->getSongs()->sortByGenre();
						break;
					case 1:
						sm->getSongs()->sortByTitle();
						break;
					case 2:
						sm->getSongs()->sortByArtist();
						break;
					case 3:
						sm->getSongs()->sortByEdition();
						break;
				}
			} else if( keypressed == SDLK_RETURN ) {
				sm->activateScreen("Sing");
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

	if( sm->getSong() == NULL ) {
		fprintf(stdout,"No songs found in \"%s\", returning to intro screen\n", sm->getSongsDirectory());
		sm->activateScreen("Intro");
		return;
	}

	theme->theme->clear();
#ifdef USE_OPENGL
        SDL_Surface *virtSurf = theme->bg->getSDLSurface();
#else
	SDL_BlitSurface(theme->bg->getSDLSurface(),NULL,sm->getSDLScreen(),NULL);
#endif	
	// Draw the "Order by" text
	{
	char * orderStr = order[sm->getSongs()->getOrder()];
	theme->order.text = orderStr;
	cairo_text_extents_t extents = theme->theme->GetTextExtents(theme->order);
	theme->order.x = (theme->order.svg_width - extents.width)/2;
	theme->theme->PrintText(&theme->order);
	}

	// Draw the "Song informations"
	{
	char informationStr[1024];
	snprintf(informationStr,1024,"%s - %s",sm->getSong()->artist, sm->getSong()->title);
	theme->song.text = informationStr;
	cairo_text_extents_t extents = theme->theme->GetTextExtents(theme->song);
	theme->song.x = (theme->song.svg_width - extents.width)/2;
	theme->theme->PrintText(&theme->song);
	}

	// Draw the cover
	{
	SDL_Rect position;
	position.x=(sm->getWidth()-sm->getSong()->coverSurf->w)/2;
	position.y=(sm->getHeight()-sm->getSong()->coverSurf->h)/2;
#ifdef USE_OPENGL
        SDL_BlitSurface(sm->getSong()->coverSurf,NULL, virtSurf, &position);
#else
        SDL_BlitSurface(sm->getSong()->coverSurf,NULL,sm->getSDLScreen(), &position);
#endif
	}
#ifdef USE_OPENGL
        SDL_GL::draw_func(  sm->getWidth(),
                            sm->getHeight(),
                            cairo_image_surface_get_data(theme->theme->getCurrent()),
                            theme_texture, GL_BGRA);

	SDL_GL::draw_func(  sm->getWidth(),
                            sm->getHeight(),
                            (unsigned char *) virtSurf->pixels,
                            bg_texture, GL_BGRA);
#else
	SDL_Surface *themeSurf = CairoToSdl::BlitToSdl(theme->theme->getCurrent());
	SDL_BlitSurface(themeSurf, NULL,sm->getSDLScreen(),NULL);
	SDL_FreeSurface(themeSurf);
#endif
}

#include <screen_songs.h>
#include <cairotosdl.h>

CScreenSongs::CScreenSongs(char * name)
{
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	screenName = name;
	songId=0;
	play = false;
	theme = new CThemeSongs();
	bg_texture = sm->getVideoDriver()->initSurface(theme->bg->getSDLSurface());
}

CScreenSongs::~CScreenSongs()
{
	delete theme;
}

void CScreenSongs::enter( void )
{
	if( CScreenManager::getSingletonPtr()->getSongs() == NULL ) {
		CScreenManager::getSingletonPtr()->setSongs(new CSongs() );
		CScreenManager::getSingletonPtr()->getSongs()->sortByArtist();
	}
}

void CScreenSongs::exit( void )
{
}

void CScreenSongs::manageEvent( SDL_Event event )
{
	int keypressed;
	CScreenManager * sm = CScreenManager::getSingletonPtr();

	switch(event.type) {
		case SDL_KEYDOWN:
			
			//Stop song preview on keystroke
			sm->getAudio()->stopMusic();
			play = false;

			keypressed = event.key.keysym.sym;
			if( keypressed == SDLK_ESCAPE || keypressed == SDLK_q ) {
				sm->activateScreen("Intro");
			} else if( keypressed == SDLK_r ) {
				if( CScreenManager::getSingletonPtr()->getSongs() != NULL )
					delete CScreenManager::getSingletonPtr()->getSongs();
				CScreenManager::getSingletonPtr()->setSongs(new CSongs() );
				CScreenManager::getSingletonPtr()->getSongs()->sortByArtist();
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
        SDL_Surface *virtSurf = theme->bg->getSDLSurface();

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
	snprintf(informationStr,1024,"(%d/%d) %s - %s",sm->getSongId()+1,sm->getSongs()->nbSongs(),sm->getSong()->artist, sm->getSong()->title);
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
        position.w=sm->getSong()->coverSurf->w;
        position.h=sm->getSong()->coverSurf->h;
        SDL_FillRect(virtSurf,&position,SDL_MapRGB(virtSurf->format, 255, 255, 255));
        SDL_BlitSurface(sm->getSong()->coverSurf,NULL, virtSurf, &position);
	}

	//Play a preview of the song (0:30-1:00, and loop)
	{
	if (!play) {
		char buff[1024];
		CSong * song = sm->getSong();
		if (song!=NULL) {
			snprintf(buff,1024,"%s/%s",song->path,song->mp3);
			sm->getAudio()->playPreview(buff);
			play = true;
		}
	}
	if (play && sm->getAudio()->isPlaying() && (sm->getAudio()->getPosition()-30000)>30000) {
		sm->getAudio()->stopMusic();
		play=false;
	}
	
	}
	
	sm->getVideoDriver()->drawSurface(bg_texture);
	sm->getVideoDriver()->drawSurface(theme->theme->getCurrent());
}

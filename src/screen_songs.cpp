#include <screen_songs.h>
#include <cairotosdl.h>
#include <iostream>
#include <sstream>

CScreenSongs::CScreenSongs(const char * name, unsigned int width, unsigned int height, std::set<std::string> const& songdirs):
  CScreen(name,width,height)
{
	if(CScreenManager::getSingletonPtr()->getSongs() == NULL) {
		CScreenManager::getSingletonPtr()->setSongs(new CSongs(songdirs));
		CScreenManager::getSingletonPtr()->getSongs()->sortByArtist();
	}
}

void CScreenSongs::enter()
{
	searchMode = false;
	searchExpr = new char[256];
	searchExpr[0] = '\0';
	songId=0;
	play = false;

	CScreenManager * sm = CScreenManager::getSingletonPtr();
	theme = new CThemeSongs(width,height);
	bg_texture = sm->getVideoDriver()->initSurface(theme->bg->getSDLSurface());
}

void CScreenSongs::exit()
{
	delete theme;
	delete[] searchExpr;
}

void CScreenSongs::manageEvent(SDL_Event event)
{
	int keypressed;
	SDLMod modifier;
	CScreenManager * sm = CScreenManager::getSingletonPtr();

	if (event.type != SDL_KEYDOWN) return;
	keypressed = event.key.keysym.sym;
	modifier   = event.key.keysym.mod;

	if(!searchMode && modifier & KMOD_CTRL && keypressed == SDLK_f) {
		fprintf(stdout,"Entering search mode\n");
		searchMode = true;
		searchExpr[0] = '\0';
		fprintf(stdout,"Search mode not yet available\n");
		fprintf(stdout,"Exiting search mode\n");
		searchMode = false;
	}

	if(searchMode) {
		if(keypressed == SDLK_ESCAPE) {
			fprintf(stdout,"Exiting search mode\n");
			searchMode = false;
			searchExpr[0] = '\0';
			// Here filter
		} else if(keypressed == SDLK_RETURN) {
			fprintf(stdout,"Exiting search mode\n");
			searchMode = false;
		} else if(keypressed >= SDLK_a && keypressed <= SDLK_z) {
			int n = strlen(searchExpr);
			if (n < 255){
				if(modifier & KMOD_SHIFT)
					searchExpr[n] = 'A' + keypressed - SDLK_a;
				else
					searchExpr[n] = 'a' + keypressed - SDLK_a;
				searchExpr[n+1] = '\0';
				// Here filter
			}
		} else if(keypressed == SDLK_BACKSPACE) {
			int n = strlen(searchExpr);
			if(n > 0) {
				searchExpr[n-1] = '\0';
				// Here filter
			}
		} else if((keypressed >= SDLK_SPACE && keypressed <= SDLK_BACKQUOTE) || (keypressed >= SDLK_WORLD_0 && keypressed <= SDLK_WORLD_95)) {
			int n = strlen(searchExpr);
			if(n < 255) {
				// using the fact that ascii == SDLK_keys
				searchExpr[n] = keypressed;
				searchExpr[n+1] = '\0';
				// Here filter
			}
		}
	} else {
		if (keypressed == SDLK_ESCAPE) {
			if (sm->getSong()) sm->getAudio()->stopMusic();
			play = false;
			sm->activateScreen("Intro");
		}
		/* FIXME: no Ctrl+R for now
		} else if(keypressed == SDLK_r && modifier & KMOD_CTRL) {
			sm->getAudio()->stopMusic();
			play = false;
			if(CScreenManager::getSingletonPtr()->getSongs() != NULL)
				delete CScreenManager::getSingletonPtr()->getSongs();
			CScreenManager::getSingletonPtr()->setSongs(new CSongs(songdirs));
			CScreenManager::getSingletonPtr()->getSongs()->sortByArtist();*/
		if (!sm->getSong()) return;
		if (keypressed == SDLK_LEFT) {
			sm->getAudio()->stopMusic();
			play = false;
			sm->setPreviousSongId();
		} else if (keypressed == SDLK_RIGHT) {
			sm->getAudio()->stopMusic();
			play = false;
			sm->setNextSongId();
		} else if (keypressed == SDLK_UP) {
			sm->getAudio()->stopMusic();
			play = false;
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
		} else if (keypressed == SDLK_DOWN) {
			sm->getAudio()->stopMusic();
			play = false;
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
		} else if (keypressed == SDLK_RETURN) {
			sm->getAudio()->stopMusic();
			play = false;
			sm->activateScreen("Sing");
		}
	}
}

const char* order[4] = {
	"Order by edition",
	"Order by genre",
	"Order by title",
	"Order by artist",
};

void CScreenSongs::draw() {
	CScreenManager * sm = CScreenManager::getSingletonPtr();

	theme->theme->clear();
	SDL_Surface *virtSurf = theme->bg->getSDLSurface();

	// Draw the "Order by" text
	{
		theme->order.text = (char*)order[sm->getSongs()->getOrder()];
		cairo_text_extents_t extents = theme->theme->GetTextExtents(theme->order);
		theme->order.x = (theme->order.svg_width - extents.width)/2;
		theme->theme->PrintText(&theme->order);
	}

	if (sm->getSong()) {

		// Draw the "Song information"
		{
			std::ostringstream oss;
			oss << "(" << sm->getSongId()+1 << "/" << sm->getSongs()->nbSongs() << ") "
			  << sm->getSong()->artist << " - " << sm->getSong()->title;
			theme->song.text = oss.str().c_str();
			cairo_text_extents_t extents = theme->theme->GetTextExtents(theme->song);
			theme->song.x = (theme->song.svg_width - extents.width)/2;
			theme->theme->PrintText(&theme->song);
		}

		// Draw the cover
		std::cout << sm->getSong()->coverSurf << std::endl;
		if (sm->getSong()->coverSurf) {
			SDL_Rect position;
			position.x=(width - sm->getSong()->coverSurf->w)/2;
			position.y=(height - sm->getSong()->coverSurf->h)/2;
			position.w=sm->getSong()->coverSurf->w;
			position.h=sm->getSong()->coverSurf->h;
			SDL_FillRect(virtSurf,&position,SDL_MapRGB(virtSurf->format, 255, 255, 255));
			SDL_BlitSurface(sm->getSong()->coverSurf,NULL, virtSurf, &position);
		}

		//Play a preview of the song (0:30-1:00, and loop)
		{
			if (!play) {
				CSong* song = sm->getSong();
				if (song!=NULL) {
					std::string file = song->path + "/" + song->mp3;
					sm->getAudio()->playPreview(file.c_str());
					play = true;
				}
			}
			if (play && sm->getAudio()->isPlaying() && (sm->getAudio()->getPosition()-30000)>30000) {
				sm->getAudio()->stopMusic();
				play=false;
			}
		}
	} else { // No songs
		// Draw the "Song information"
		{
			theme->song.text = "No songs found";
			cairo_text_extents_t extents = theme->theme->GetTextExtents(theme->song);
			theme->song.x = (theme->song.svg_width - extents.width)/2;
			theme->theme->PrintText(&theme->song);
		}
	}
	sm->getVideoDriver()->drawSurface(bg_texture);
	sm->getVideoDriver()->drawSurface(theme->theme->getCurrent());
}


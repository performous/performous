#include <screen_songs.h>
#include <cairotosdl.h>
#include <iostream>
#include <sstream>

CScreenSongs::CScreenSongs(const char * name, unsigned int width, unsigned int height, std::set<std::string> const& songdirs):
  CScreen(name, width, height), m_searching()
{
	if (CScreenManager::getSingletonPtr()->getSongs() == NULL) {
		CScreenManager::getSingletonPtr()->setSongs(new CSongs(songdirs));
	}
}

void CScreenSongs::enter() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	theme = new CThemeSongs(m_width, m_height);
	bg_texture = sm->getVideoDriver()->initSurface(theme->bg->getSDLSurface());
}

void CScreenSongs::exit() {
	m_playing.clear();
	delete theme;
}

void CScreenSongs::manageEvent(SDL_Event event) {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	if (event.type != SDL_KEYDOWN) return;
	int key = event.key.keysym.sym;
	SDLMod modifier = event.key.keysym.mod;

	if (m_searching) {
		if (key == SDLK_ESCAPE) {
			m_searching = false;
			m_search.clear();
		} else if (modifier & KMOD_CTRL && key == SDLK_f) m_search.clear();
		else if (key == SDLK_BACKSPACE && !m_search.empty()) m_search.erase(m_search.size() - 1);
		else if (key >= SDLK_a && key <= SDLK_z) m_search += (modifier & KMOD_SHIFT ? 'A' : 'a') + key - SDLK_a;
		else if ((key >= SDLK_SPACE && key <= SDLK_BACKQUOTE) || (key >= SDLK_WORLD_0 && key <= SDLK_WORLD_95)) m_search += key;
		sm->getSongs()->setFilter(m_search);
	} else if (key == SDLK_ESCAPE) {
		if (sm->getSongs()->size() > 0) sm->getAudio()->stopMusic();
		sm->activateScreen("Intro");
	}
	if (key == SDLK_r && modifier & KMOD_CTRL) { sm->getSongs()->reload(); m_searching = false; }
	if (sm->getSongs()->empty()) return;
	if (key == SDLK_s && modifier & KMOD_CTRL) sm->getAudio()->stopMusic();
	else if (key == SDLK_LEFT) sm->getSongs()->advance(-1);
	else if (key == SDLK_RIGHT) sm->getSongs()->advance(1);
	else if (key == SDLK_PAGEUP) sm->getSongs()->advance(-10);
	else if (key == SDLK_PAGEDOWN) sm->getSongs()->advance(10);
	else if (key == SDLK_UP) sm->getSongs()->sortChange(-1);
	else if (key == SDLK_DOWN) sm->getSongs()->sortChange(1);
	else if (key == SDLK_RETURN) sm->activateScreen("Sing");
	else if (!m_searching && modifier & KMOD_CTRL && key == SDLK_f) {
		m_searching = true;
		m_search.clear();
		sm->getSongs()->setFilter(m_search);
	}
}

void CScreenSongs::draw() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();

	theme->theme->clear();
	SDL_Surface *virtSurf = theme->bg->getSDLSurface();

	// Draw the "Order by" text
	{
		theme->order.text = (m_searching ? "Find: " + m_search : sm->getSongs()->sortDesc());
		cairo_text_extents_t extents = theme->theme->GetTextExtents(theme->order);
		theme->order.x = (theme->order.svg_width - extents.width)/2;
		theme->theme->PrintText(&theme->order);
	}
	// Test if there are no songs
	if (sm->getSongs()->empty()) {
		// Draw the "Song information"
		theme->song.text = "No songs found (Ctrl+R to reload)";
		cairo_text_extents_t extents = theme->theme->GetTextExtents(theme->song);
		theme->song.x = (theme->song.svg_width - extents.width)/2;
		theme->theme->PrintText(&theme->song);
	} else {
		// Draw the "Song information"
		{
			std::ostringstream oss;
			CSongs& s = *sm->getSongs();
			oss << "(" << s.currentId() + 1 << "/" << s.size() << ") " << s.current().str();
			theme->song.text = oss.str().c_str();
			cairo_text_extents_t extents = theme->theme->GetTextExtents(theme->song);
			theme->song.x = (theme->song.svg_width - extents.width)/2;
			theme->theme->PrintText(&theme->song);
		}
		// Draw the cover
		{
			SDL_Surface* surf = sm->getSong()->coverSurf;
			if (!surf) surf = sm->getSongs()->getEmptyCover();
			if (!surf) throw std::runtime_error("No cover image and no empty cover image");
			SDL_Rect position;
			position.x = (m_width - surf->w) / 2;
			position.y = (m_height - surf->h) / 2;
			position.w = surf->w;
			position.h = surf->h;
			SDL_FillRect(virtSurf, &position, SDL_MapRGB(virtSurf->format, 255, 255, 255));
			SDL_BlitSurface(surf, NULL, virtSurf, &position);
		}
		// Play a preview of the song starting from 0:30 (no loop anymore)
		CSong* song = sm->getSong();
		std::string file = song->path + "/" + song->mp3;
		if (file != m_playing) sm->getAudio()->playPreview(m_playing = file);
	}
	sm->getVideoDriver()->drawSurface(bg_texture);
	sm->getVideoDriver()->drawSurface(theme->theme->getCurrent());
}


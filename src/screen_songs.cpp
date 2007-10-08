#include <screen_songs.h>
#include <cairotosdl.h>
#include <iostream>
#include <sstream>

CScreenSongs::CScreenSongs(std::string const& name, unsigned int width, unsigned int height, std::set<std::string> const& songdirs):
  CScreen(name, width, height), m_searching()
{
	if (CScreenManager::getSingletonPtr()->getSongs() == NULL) {
		CScreenManager::getSingletonPtr()->setSongs(new CSongs(songdirs));
	}
}

void CScreenSongs::enter() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	sm->getAudio()->stopMusic();
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
	SDL_keysym keysym = event.key.keysym;
	int key = keysym.sym;
	SDLMod mod = event.key.keysym.mod;
	if (key == SDLK_r && mod & KMOD_CTRL) { sm->getSongs()->reload(); m_searching = false; }
	else if (m_searching) {
		if (key == SDLK_ESCAPE) { m_searching = false; m_search.clear(); }
		else if (key == SDLK_BACKSPACE && !m_search.empty()) m_search.erase(m_search.size() - 1);
		else if (keysym.unicode >= 0x20 && keysym.unicode < 0x7F) m_search += keysym.unicode;
		sm->getSongs()->setFilter(m_search);
	}
	else if (key == SDLK_ESCAPE || key == SDLK_q) sm->activateScreen("Intro");
	// The rest are only available when there are songs available
	else if (sm->getSongs()->empty()) return;
	else if (key == SDLK_SPACE) sm->getAudio()->togglePause();
	else if (key == SDLK_r) sm->getSongs()->random();
	else if (key == SDLK_f || keysym.unicode == '/') {
		m_searching = true;
		m_search.clear();
		sm->getSongs()->setFilter(m_search);
	}
	// These are available in both modes (search and normal)
	if (key == SDLK_RETURN && !(mod & KMOD_ALT)) sm->activateScreen("Sing");
	else if (key == SDLK_LEFT) sm->getSongs()->advance(-1);
	else if (key == SDLK_RIGHT) sm->getSongs()->advance(1);
	else if (key == SDLK_PAGEUP) sm->getSongs()->advance(-10);
	else if (key == SDLK_PAGEDOWN) sm->getSongs()->advance(10);
	else if (key == SDLK_UP) sm->getSongs()->sortChange(-1);
	else if (key == SDLK_DOWN) sm->getSongs()->sortChange(1);
}

namespace {
	void print(CThemeSongs* theme, TThemeTxt t, std::string const& text) {
		t.text = text;
		do {
			cairo_text_extents_t extents = theme->theme->GetTextExtents(t);
			t.x = (t.svg_width - extents.width)/2;
		} while (t.x < 0 && (t.fontsize -= 2) > 0);
		theme->theme->PrintText(&t);
	}
}

void CScreenSongs::draw() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	theme->theme->clear();
	SDL_Surface *virtSurf = theme->bg->getSDLSurface();
	// Draw the "Order by" text
	print(theme, theme->order, (m_searching ? "find: " + m_search : sm->getSongs()->sortDesc()));
	// Test if there are no songs
	if (sm->getSongs()->empty()) {
		print(theme, theme->song, "no songs found");
		if (!m_playing.empty()) { sm->getAudio()->stopMusic(); m_playing.clear(); }
	} else {
		CSong& song = sm->getSongs()->current();
		// Draw the "Song information"
		{
			std::ostringstream oss;
			CSongs& s = *sm->getSongs();
			oss << song.str() << "\n(" << s.currentId() + 1 << "/" << s.size() << ")";
			print(theme, theme->song, oss.str());
		}
		// Draw the cover
		{
			SDL_Surface* surf = song.getCover();
			if (!surf) surf = sm->getSongs()->getEmptyCover();
			if (!surf) throw std::logic_error("No cover image and no empty cover image");
			SDL_Rect position;
			position.x = (m_width - surf->w) / 2;
			position.y = (m_height - surf->h) / 2;
			position.w = surf->w;
			position.h = surf->h;
			SDL_FillRect(virtSurf, &position, SDL_MapRGB(virtSurf->format, 255, 255, 255));
			SDL_BlitSurface(surf, NULL, virtSurf, &position);
		}
		// Play a preview of the song
		std::string file = song.path + song.mp3;
		if (file != m_playing) sm->getAudio()->playPreview(m_playing = file);
	}
	sm->getVideoDriver()->drawSurface(bg_texture);
	sm->getVideoDriver()->drawSurface(theme->theme->getCurrent());
}


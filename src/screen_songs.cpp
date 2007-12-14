#include <screen_songs.h>
#include <cairotosdl.h>
#include <iostream>
#include <sstream>

CScreenSongs::CScreenSongs(std::string const& name, unsigned int width, unsigned int height, std::set<std::string> const& songdirs):
  CScreen(name, width, height),
  m_searching(),
  m_emptyCover(CScreenManager::getSingletonPtr()->getThemePathFile("no_cover.png"), CScreenManager::getSingletonPtr()->getWidth() / 800.0 * 256.0, CScreenManager::getSingletonPtr()->getHeight() / 600.0 * 256.0),
  m_currentCover(NULL)
{
	if (CScreenManager::getSingletonPtr()->getSongs() == NULL) {
		CScreenManager::getSingletonPtr()->setSongs(new Songs(songdirs));
	}
}

void CScreenSongs::enter() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	sm->getAudio()->stopMusic();
	theme.reset(new CThemeSongs(m_width, m_height));
	bg_texture = sm->getVideoDriver()->initSurface(theme->bg->getSDLSurface());
}

void CScreenSongs::exit() {
	m_cover.clear();
	m_playing.clear();
	theme.reset();
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
	// These are available in both modes (search and normal), if there are songs
	if (sm->getSongs()->empty()) return;
	else if (key == SDLK_RETURN) sm->activateScreen("Sing");
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
	// Draw the "Order by" text
	print(theme.get(), theme->order, (m_searching ? "find: " + m_search : sm->getSongs()->sortDesc()));
	// Test if there are no songs
	if (sm->getSongs()->empty()) {
		print(theme.get(), theme->song, "no songs found");
		if (!m_playing.empty()) { sm->getAudio()->stopMusic(); m_playing.clear(); }
	} else {
		Song& song = sm->getSongs()->current();
		// Draw the "Song information"
		{
			std::ostringstream oss;
			Songs& s = *sm->getSongs();
			oss << song.str() << "\n(" << s.currentId() + 1 << "/" << s.size() << ")";
			print(theme.get(), theme->song, oss.str());
		}
		// Draw the cover
		std::string cover = song.path + song.cover;
		if (cover != m_cover) {
			m_cover = cover;
			double width = CScreenManager::getSingletonPtr()->getWidth();
			double height = CScreenManager::getSingletonPtr()->getHeight();
			SDLSurf coverSurf(cover, width / 800.0 * 256, height / 600.0 * 256.0);
			if( coverSurf ) {
				// Free the last cached surface
				if(m_currentCover != m_emptyCover) SDL_FreeSurface(m_currentCover);
				// Increment SDL refcount to avoid to free the structure when coverSurf will
				// be destroy
				coverSurf->refcount++;
				m_currentCover = coverSurf;
			} else
				m_currentCover = m_emptyCover;
		}
		// Play a preview of the song
		std::string file = song.path + song.mp3;
		if (file != m_playing) sm->getAudio()->playPreview(m_playing = file);
	}
	sm->getVideoDriver()->drawSurface(bg_texture);
	if (m_currentCover) {
		SDL_Rect position;
		position.x = (m_width - m_currentCover->w) / 2;
		position.y = (m_height - m_currentCover->h) / 2;
		position.w = m_currentCover->w;
		position.h = m_currentCover->h;
		sm->getVideoDriver()->drawSurface(m_currentCover, position.x, position.y);
	}
	sm->getVideoDriver()->drawSurface(theme->theme->getCurrent());
}


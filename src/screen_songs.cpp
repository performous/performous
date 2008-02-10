#include <screen_songs.h>
#include <cairotosdl.h>
#include <xtime.h>
#include <iostream>
#include <sstream>

static const double IDLE_TIMEOUT = 45.0; // seconds

CScreenSongs::CScreenSongs(std::string const& name, unsigned int width, unsigned int height, std::set<std::string> const& songdirs):
  CScreen(name, width, height),
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
	m_time = seconds(now());
}

void CScreenSongs::exit() {
	m_cover.clear();
	m_playing.clear();
	theme.reset();
}

namespace {
	std::string utf8(unsigned int ucs) {
		std::string utf8;
		if (ucs < 0x80) {
			utf8 += ucs;
		} else if (ucs < 0x800) {
			utf8 += 0xC0 | (ucs >> 6);
			utf8 += 0x80 | ucs & 0x3F;
		} else if (ucs < 0x10000) {
			utf8 += 0xE0 | (ucs >> 12);
			utf8 += 0x80 | (ucs >> 6) & 0x3F;
			utf8 += 0x80 | ucs & 0x3F;
		} else {
			utf8 += 0xF0 | (ucs >> 18);
			utf8 += 0x80 | (ucs >> 12) & 0x3F;
			utf8 += 0x80 | (ucs >> 6) & 0x3F;
			utf8 += 0x80 | ucs & 0x3F;
		}
		return utf8;
	}
	void backspace(std::string& str) {
		std::string::size_type pos = str.size() - 1;
		while ((str[pos] & 0xC0) == 0x80) --pos;
		str.erase(pos);
	}
}

void CScreenSongs::manageEvent(SDL_Event event) {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	Songs& songs = *sm->getSongs();
	if (event.type != SDL_KEYDOWN) return;
	m_time = seconds(now());
	SDL_keysym keysym = event.key.keysym;
	int key = keysym.sym;
	SDLMod mod = event.key.keysym.mod;
	if (key == SDLK_r && mod & KMOD_CTRL) songs.reload();
	else if (key == SDLK_BACKSPACE && !m_search.empty()) { backspace(m_search); songs.setFilter(m_search); }
	else if (m_search.size() < 100 && keysym.unicode >= 0x20 && (keysym.unicode < 0x7F || keysym.unicode >= 0xA0))
	  songs.setFilter(m_search += utf8(keysym.unicode));
	else if (key == SDLK_ESCAPE) {
		if (m_search.empty()) sm->activateScreen("Intro");
		else { m_search.clear(); songs.setFilter(m_search); }
	}
	// The rest are only available when there are songs available
	else if (songs.empty()) return;
	else if (key == SDLK_PAUSE || (key == SDLK_p && mod && KMOD_CTRL)) sm->getAudio()->togglePause();
	else if (key == SDLK_TAB) songs.random(mod & KMOD_SHIFT);
	else if (key == SDLK_RETURN) sm->activateScreen("Sing");
	else if (key == SDLK_LEFT) songs.advance(-1);
	else if (key == SDLK_RIGHT) songs.advance(1);
	else if (key == SDLK_PAGEUP) songs.advance(-10);
	else if (key == SDLK_PAGEDOWN) songs.advance(10);
	else if (key == SDLK_UP) songs.sortChange(-1);
	else if (key == SDLK_DOWN) songs.sortChange(1);
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
	CAudio& audio = *sm->getAudio();
	Songs& songs = *sm->getSongs();
	theme->theme->clear();
	// Draw the "Order by" text
	print(theme.get(), theme->order, (m_search.empty() ? songs.sortDesc() : m_search));
	sm->getVideoDriver()->drawSurface(bg_texture);
	// Test if there are no songs
	if (songs.empty()) {
		print(theme.get(), theme->song, "no songs found");
		if (!m_playing.empty()) { audio.stopMusic(); m_playing.clear(); }
	} else {
		Song& song = songs.current();
		// Draw the "Song information"
		{
			std::ostringstream oss;
			Songs& s = *sm->getSongs();
			oss << song.str() << "\n(" << s.currentId() + 1 << "/" << s.size() << ")";
			print(theme.get(), theme->song, oss.str());
		}
		// Draw the cover
		Song& song_display = songs.near(songs.currentPosition());
		std::string cover = song_display.path + song_display.cover;
		if (cover != m_cover) {
			m_cover = cover;
			double width = CScreenManager::getSingletonPtr()->getWidth();
			double height = CScreenManager::getSingletonPtr()->getHeight();
			SDLSurf coverSurf(cover, width / 800.0 * 256, height / 600.0 * 256.0);
			m_currentCover.swap(coverSurf);
		}
		SDL_Surface* coverSurf = m_currentCover ? m_currentCover : m_emptyCover;
		if (coverSurf) {
			SDL_Rect position;
			double shift = remainder(songs.currentPosition(), 1.0);
			position.x = round((m_width - coverSurf->w) / 2 - shift * 1056);
			position.y = (m_height - coverSurf->h) / 2;
			position.w = coverSurf->w;
			position.h = coverSurf->h;
			sm->getVideoDriver()->drawSurface(coverSurf, position.x, position.y);
		}
		// Play a preview of the song
		std::string file = song.path + song.mp3;
		if (file != m_playing) audio.playPreview(m_playing = file);
	}
	sm->getVideoDriver()->drawSurface(theme->theme->getCurrent());
	if (!audio.isPaused() && seconds(now()) - m_time > IDLE_TIMEOUT) {
		m_time = seconds(now());
		if (!m_search.empty()) { m_search.clear(); songs.setFilter(m_search); }
		songs.random();
	}
}


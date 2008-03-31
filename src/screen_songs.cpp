#include <screen_songs.h>
#include <xtime.h>
#include <iostream>
#include <sstream>

static const double IDLE_TIMEOUT = 45.0; // seconds

CScreenSongs::CScreenSongs(std::string const& name, unsigned int width, unsigned int height, std::set<std::string> const& songdirs):
  CScreen(name, width, height)
{
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	if (sm->getSongs() == NULL)
		sm->setSongs(new Songs(songdirs));
	m_emptyCover.reset(new Surface(sm->getThemePathFile("no_cover.svg"),Surface::SVG));
}

void CScreenSongs::enter() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	CAudio& audio = *sm->getAudio();
	audio.stopMusic();
	audio.wait();
	static unsigned int m_volume;
	m_volume = audio.getVolume();
	audio.setVolume(m_volume);
	theme.reset(new CThemeSongs(m_width, m_height));
	m_time = seconds(now());
	m_search.text.clear();
	sm->getSongs()->setFilter(m_search.text);
}

void CScreenSongs::exit() {
	m_cover.clear();
	m_playing.clear();
	theme.reset();
}

void CScreenSongs::manageEvent(SDL_Event event) {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	Songs& songs = *sm->getSongs();
	if (event.type != SDL_KEYDOWN) return;
	m_time = seconds(now());
	SDL_keysym keysym = event.key.keysym;
	int key = keysym.sym;
	SDLMod mod = event.key.keysym.mod;
	if (key == SDLK_r && mod & KMOD_CTRL) { songs.reload(); songs.setFilter(m_search.text); }
	if (m_search.process(keysym)) songs.setFilter(m_search.text);
	else if (key == SDLK_ESCAPE) {
		if (m_search.text.empty()) sm->activateScreen("Intro");
		else { m_search.text.clear(); songs.setFilter(m_search.text); }
	}
	// The rest are only available when there are songs available
	else if (songs.empty()) return;
	else if (key == SDLK_PAUSE || (key == SDLK_p && mod && KMOD_CTRL)) sm->getAudio()->togglePause();
	else if (key == SDLK_TAB && !(mod & KMOD_ALT)) songs.randomize();
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
	print(theme.get(), theme->order, (m_search.text.empty() ? songs.sortDesc() : m_search.text));
	theme->bg->draw();
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
			m_currentCover.reset();
			try { m_currentCover.reset(new Surface(cover,Surface::MAGICK)); } catch (std::exception& e) {}
		}
		double shift = remainder(songs.currentPosition(), 1.0);
		(m_currentCover ? m_currentCover : m_emptyCover)->draw(shift, 0.0, 0.3);
		// Play a preview of the song
		std::string file = song.path + song.mp3;
		if (file != m_playing) audio.playPreview(m_playing = file);
	}
	Surface(theme->theme->getCurrent()).draw();
	if (!audio.isPaused() && seconds(now()) - m_time > IDLE_TIMEOUT) {
		m_time = seconds(now());
		if (!m_search.text.empty()) { m_search.text.clear(); songs.setFilter(m_search.text); }
		songs.random();
	}
}


#include "screen_songs.hh"
#include "xtime.hh"
#include <iostream>
#include <sstream>

static const double IDLE_TIMEOUT = 45.0; // seconds

CScreenSongs::CScreenSongs(std::string const& name, std::set<std::string> const& songdirs):
  CScreen(name)
{
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	if (!sm->getSongs()) sm->setSongs(new Songs(songdirs));
}

void CScreenSongs::enter() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	CAudio& audio = *sm->getAudio();
	audio.stopMusic();
	audio.wait();
	theme.reset(new CThemeSongs());
	m_emptyCover.reset(new Surface(sm->getThemePathFile("no_cover.svg")));
	m_time = seconds(now());
	m_search.text.clear();
	sm->getSongs()->setFilter(m_search.text);
}

void CScreenSongs::exit() {
	m_cover.clear();
	m_playing.clear();
	m_emptyCover.reset();
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
	else if (key == SDLK_SPACE || (key == SDLK_PAUSE || (key == SDLK_p && mod && KMOD_CTRL))) sm->getAudio()->togglePause();
	else if (key == SDLK_TAB && !(mod & KMOD_ALT)) songs.randomize();
	else if (key == SDLK_RETURN) sm->activateScreen("Sing");
	else if (key == SDLK_LEFT) songs.advance(-1);
	else if (key == SDLK_RIGHT) songs.advance(1);
	else if (key == SDLK_PAGEUP) songs.advance(-10);
	else if (key == SDLK_PAGEDOWN) songs.advance(10);
	else if (key == SDLK_UP) songs.sortChange(-1);
	else if (key == SDLK_DOWN) songs.sortChange(1);
}

void CScreenSongs::draw() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	CAudio& audio = *sm->getAudio();
	Songs& songs = *sm->getSongs();
	songs.update(); // Poll for new songs
	theme->bg->draw();
	std::ostringstream oss_song, oss_order;
	// Test if there are no songs
	if (songs.empty()) {
		// Format the song information text
		if (m_search.text.empty()) oss_song << "no songs found";
		else {
			oss_song << "no songs match search";
			oss_order << m_search.text;
		}
		if (!m_playing.empty()) { audio.stopMusic(); m_playing.clear(); }
	} else {
		Song& song = songs.current();
		// Format the song information text
		oss_song << song.str() << "\n";
		oss_song << "(" << songs.currentId() + 1 << "/" << songs.size() << ")";
		oss_order << (m_search.text.empty() ? songs.sortDesc() : m_search.text);
		// Draw the cover
		Song& song_display = songs.near(songs.currentPosition());
		std::string cover = song_display.path + song_display.cover;
		if (cover != m_cover) {
			m_cover = cover;
			m_currentCover.reset();
			try { m_currentCover.reset(new Surface(cover)); } catch (std::exception& e) {}
		}
		double shift = remainder(songs.currentPosition(), 1.0);
		Surface& s = *(m_currentCover ? m_currentCover : m_emptyCover);
		s.dimensions.middle(-1.3 * shift).fitInside(0.3, 0.3);
		s.draw();
		// Play a preview of the song
		std::string file = song.path + song.mp3;
		if (file != m_playing) audio.playPreview(m_playing = file);
	}
	// Draw song and order texts
	theme->song->draw(oss_song.str());
	theme->order->draw(oss_order.str());
	if (!audio.isPaused() && seconds(now()) - m_time > IDLE_TIMEOUT) {
		m_time = seconds(now());
		if (!m_search.text.empty()) { m_search.text.clear(); songs.setFilter(m_search.text); }
		songs.random();
	}
}


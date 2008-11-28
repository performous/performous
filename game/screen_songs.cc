#include "screen_songs.hh"
#include "util.hh"
#include "xtime.hh"
#include <iostream>
#include <sstream>

static const double IDLE_TIMEOUT = 45.0; // seconds

CScreenSongs::CScreenSongs(std::string const& name, Audio& audio, Songs& songs):
  CScreen(name), m_audio(audio), m_songs(songs), m_covers(20)
{}

void CScreenSongs::enter() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	m_playing.clear();
	m_playReq.clear();
	theme.reset(new CThemeSongs());
	m_emptyCover.reset(new Surface(sm->getThemePathFile("no_cover.svg")));
	m_time = seconds(now());
	m_search.text.clear();
	m_songs.setFilter(m_search.text);
}

void CScreenSongs::exit() {
	m_covers.clear();
	m_emptyCover.reset();
	theme.reset();
}

void CScreenSongs::manageEvent(SDL_Event event) {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	if (event.type != SDL_KEYDOWN) return;
	m_time = seconds(now());
	SDL_keysym keysym = event.key.keysym;
	int key = keysym.sym;
	SDLMod mod = event.key.keysym.mod;
	if (key == SDLK_r && mod & KMOD_CTRL) { m_songs.reload(); m_songs.setFilter(m_search.text); }
	if (m_search.process(keysym)) m_songs.setFilter(m_search.text);
	else if (key == SDLK_ESCAPE) {
		if (m_search.text.empty()) sm->activateScreen("Intro");
		else { m_search.text.clear(); m_songs.setFilter(m_search.text); }
	}
	// The rest are only available when there are songs available
	else if (m_songs.empty()) return;
	else if (key == SDLK_SPACE || (key == SDLK_PAUSE || (key == SDLK_p && mod && KMOD_CTRL))) m_audio.togglePause();
	else if (key == SDLK_TAB && !(mod & KMOD_ALT)) m_songs.randomize();
	else if (key == SDLK_RETURN) sm->activateScreen("Sing");
	else if (key == SDLK_LEFT) m_songs.advance(-1);
	else if (key == SDLK_RIGHT) m_songs.advance(1);
	else if (key == SDLK_PAGEUP) m_songs.advance(-10);
	else if (key == SDLK_PAGEDOWN) m_songs.advance(10);
	else if (key == SDLK_UP) m_songs.sortChange(-1);
	else if (key == SDLK_DOWN) m_songs.sortChange(1);
}

void CScreenSongs::draw() {
	m_songs.update(); // Poll for new songs
	theme->bg->draw();
	std::string music;
	std::ostringstream oss_song, oss_order;
	// Test if there are no songs
	if (m_songs.empty()) {
		// Format the song information text
		if (m_search.text.empty()) oss_song << "no songs found";
		else {
			oss_song << "no songs match search";
			oss_order << m_search.text;
		}
	} else {
		Song& song = m_songs.current();
		// Format the song information text
		oss_song << song.str() << "\n";
		oss_song << "(" << m_songs.currentId() + 1 << "/" << m_songs.size() << ")";
		oss_order << (m_search.text.empty() ? m_songs.sortDesc() : m_search.text);
		// Draw the covers
		double shift = remainder(m_songs.currentPosition(), 1.0);
		std::size_t ss = m_songs.size();
		for (int i = -2; i < 5; ++i) {
			Song& song_display = m_songs[std::size_t(round(m_songs.currentPosition() - shift) + i + 2 * ss) % ss];
			Surface* cover = NULL;
			// Fetch cover image from cache or try loading it
			try { cover = &m_covers[song_display.path + song_display.cover]; } catch (std::exception const&) {}
			Surface& s = (cover ? *cover : *m_emptyCover);
			double diff = 0.0;
			if (i == 0) diff = (0.5 - fabs(shift)) * 0.04;
			s.dimensions.middle(-0.2 + 0.17 * (i - shift)).bottom(0.3 + 0.5 * diff).fitInside(0.15 + diff, 0.15 + diff);
			s.draw();
			s.dimensions.top(0.3 + 0.5 * diff);
			s.tex = TexCoords(0, 1, 1, 0);
			glColor4f(1.0, 1.0, 1.0, 0.4);
			s.draw();
			s.tex = TexCoords();
			glColor3f(1.0, 1.0, 1.0);
		}
		music = song.path + song.mp3;
	}
	// Draw song and order texts
	theme->song->draw(oss_song.str());
	theme->order->draw(oss_order.str());
	if (!m_audio.isPaused() && seconds(now()) - m_time > IDLE_TIMEOUT) {
		m_time = seconds(now());
		if (!m_search.text.empty()) { m_search.text.clear(); m_songs.setFilter(m_search.text); }
		m_songs.random();
	}
	// Schedule playback change if the chosen song has changed
	if (music != m_playReq) { m_playReq = music; m_playTimer.setValue(0.4); }
	// Play/stop preview playback (if it is the time)
	if (music != m_playing && m_playTimer.get() == 0.0) {
		if (music.empty()) m_audio.stopMusic(); else m_audio.playPreview(music);
		m_playing = music;
	}
}


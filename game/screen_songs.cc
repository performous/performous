#include "screen_songs.hh"
#include "configuration.hh"
#include "util.hh"
#include "xtime.hh"
#include <iostream>
#include <sstream>

static const double IDLE_TIMEOUT = 45.0; // seconds

ScreenSongs::ScreenSongs(std::string const& name, Audio& audio, Songs& songs):
  Screen(name), m_audio(audio), m_songs(songs), m_covers(20), m_jukebox()
{
	m_songs.setAnimMargins(5.0, 5.0);
	m_playTimer.setTarget(getInf()); // Using this as a simple timer counting seconds
}

void ScreenSongs::enter() {
	theme.reset(new ThemeSongs());
	m_emptyCover.reset(new Surface(getThemePath("no_cover.svg")));
	m_search.text.clear();
	m_songs.setFilter(m_search.text);
	m_audio.fadeout();
	m_jukebox = false;
}

void ScreenSongs::exit() {
	m_covers.clear();
	m_emptyCover.reset();
	theme.reset();
	m_video.reset();
	m_songbg.reset();
	m_playing.clear();
	m_playReq.clear();
}

void ScreenSongs::manageEvent(SDL_Event event) {
	ScreenManager* sm = ScreenManager::getSingletonPtr();
	if (event.type != SDL_KEYDOWN) return;
	SDL_keysym keysym = event.key.keysym;
	int key = keysym.sym;
	SDLMod mod = event.key.keysym.mod;
	if (m_jukebox) {
		if (key == SDLK_ESCAPE || m_songs.empty()) m_jukebox = false;
		else if (key == SDLK_SPACE || (key == SDLK_PAUSE || (key == SDLK_p && mod & KMOD_CTRL))) m_audio.togglePause();
		else if (key == SDLK_RETURN) sm->activateScreen("Sing");
		else if (key == SDLK_LEFT) m_songs.advance(-1);
		else if (key == SDLK_RIGHT) m_songs.advance(1);
		else if (key == SDLK_PAGEUP) m_audio.seek(-30);
		else if (key == SDLK_PAGEDOWN) m_audio.seek(30);
		else if (key == SDLK_UP) m_audio.seek(5);
		else if (key == SDLK_DOWN) m_audio.seek(-5);
		return;
	}
	if (key == SDLK_r && mod & KMOD_CTRL) { m_songs.reload(); m_songs.setFilter(m_search.text); }
	if (!m_jukebox && m_search.process(keysym)) m_songs.setFilter(m_search.text);
	else if (key == SDLK_ESCAPE) {
		if (m_search.text.empty()) sm->activateScreen("Intro");
		else { m_search.text.clear(); m_songs.setFilter(m_search.text); }
	}
	// The rest are only available when there are songs available
	else if (m_songs.empty()) return;
	else if (key == SDLK_SPACE || (key == SDLK_PAUSE || (key == SDLK_p && mod & KMOD_CTRL))) m_audio.togglePause();
	else if (key == SDLK_TAB && !(mod & KMOD_ALT)) m_songs.randomize();
	else if (key == SDLK_RETURN) sm->activateScreen("Sing");
	else if (key == SDLK_LEFT) m_songs.advance(-1);
	else if (key == SDLK_RIGHT) m_songs.advance(1);
	else if (key == SDLK_PAGEUP) m_songs.advance(-10);
	else if (key == SDLK_PAGEDOWN) m_songs.advance(10);
	else if (key == SDLK_UP) m_songs.sortChange(-1);
	else if (key == SDLK_DOWN) m_songs.sortChange(1);
	else if (!m_jukebox && key == SDLK_F4) m_jukebox = true;
}

void ScreenSongs::drawJukebox() {
	double pos = m_audio.getPosition();
	double len = m_audio.getLength();
	double diff = len - pos;
	if (pos < diff) diff = pos;  // Diff from beginning instead of from end
	if (!m_songbg.get() && !m_video.get()) diff = 0.0;  // Always display song name if there is no background
	if (diff < 3.0) {
		Song& song = m_songs.current();
		// Draw the cover
		/*
		Surface* cover = NULL;
		if (!song.cover.empty()) try { cover = &m_covers[song.path + song.cover]; } catch (std::exception const&) {}
		Surface& s = (cover ? *cover : *m_emptyCover);
		s.dimensions.right(theme->song.dimensions.x1()).top(theme->song.dimensions.y1()).fitInside(0.1, 0.1); s.draw();
		*/
		// Format && draw the song information text
		std::ostringstream oss_song;
		oss_song << song.title << '\n' << song.artist;
		theme->song.draw(oss_song.str());
	}
}

void ScreenSongs::draw() {
	m_songs.update(); // Poll for new songs
	double length = m_audio.getLength();
	double time = clamp(m_audio.getPosition() - config["audio/video_delay"].f(), 0.0, length);
	if (m_songbg.get()) m_songbg->draw();
	if (m_video.get()) m_video->render(time);
	if (!m_jukebox) theme->bg.draw();
	std::string music, songbg, video;
	double videoGap = 0.0;
	std::ostringstream oss_song, oss_order;
	// Test if there are no songs
	if (m_songs.empty()) {
		// Format the song information text
		if (m_search.text.empty()) {
			oss_song << "No songs found!";
			oss_order << "Visit performous.org\n";
			oss_order << "for free songs";
		} else {
			oss_song << "no songs match search";
			oss_order << m_search.text << '\n';
		}
	} else {
		Song& song = m_songs.current();
		// Format the song information text
		oss_song << song.title << '\n' << song.artist;
		oss_order << (m_search.text.empty() ? m_songs.sortDesc() : m_search.text) << '\n';
		oss_order << "(" << m_songs.currentId() + 1 << "/" << m_songs.size() << ")";
		double spos = m_songs.currentPosition(); // This needs to be polled to run the animation
		if (!m_jukebox) {
			// Draw the covers
			std::size_t ss = m_songs.size();
			int baseidx = spos + 1.5; --baseidx; // Round correctly
			double shift = spos - baseidx;
			for (int i = -2; i < 5; ++i) {
				if (baseidx + i < 0 || baseidx + i >= int(ss)) continue;
				Song& song_display = m_songs[baseidx + i];
				Surface* cover = NULL;
				// Fetch cover image from cache or try loading it
				if (!song_display.cover.empty()) try { cover = &m_covers[song_display.path + song_display.cover]; } catch (std::exception const&) {}
				Surface& s = (cover ? *cover : *m_emptyCover);
				double diff = (i == 0 ? (0.5 - fabs(shift)) * 0.07 : 0.0);
				double y = 0.27 + 0.5 * diff;
				// Draw the cover
				s.dimensions.middle(-0.2 + 0.17 * (i - shift)).bottom(y - 0.2 * diff).fitInside(0.14 + diff, 0.14 + diff); s.draw();
				// Draw the reflection
				s.dimensions.top(y + 0.2 * diff); s.tex = TexCoords(0, 1, 1, 0); glColor4f(1.0, 1.0, 1.0, 0.4); s.draw();
				s.tex = TexCoords(); glColor3f(1.0, 1.0, 1.0); // Restore default attributes
			}
		}
		if (!song.mp3.empty()) music = song.path + song.mp3;
		if (!song.background.empty()) songbg = song.path + song.background;
		if (!song.video.empty()) { video = song.path + song.video; videoGap = song.videoGap; }
	}
	if (m_jukebox) drawJukebox();
	else {
		// Draw song and order texts
		theme->song.draw(oss_song.str());
		theme->order.draw(oss_order.str());
	}
	// Schedule playback change if the chosen song has changed
	if (music != m_playReq) { m_playReq = music; m_playTimer.setValue(0.0); }
	// Play/stop preview playback (if it is the time)
	if (music != m_playing && m_playTimer.get() > 0.4) {
		m_songbg.reset(); m_video.reset();
		if (music.empty()) m_audio.fadeout(); else m_audio.playPreview(music, m_jukebox ? 0.0 : 30.0);
		if (!songbg.empty()) try { m_songbg.reset(new Surface(songbg)); } catch (std::exception const&) {}
		if (!video.empty() && config["graphic/video"].b()) m_video.reset(new Video(video, videoGap));
		m_playing = music;
	}
	if (m_jukebox) {
		// Switch if at song end
		if (!m_audio.isPlaying() || m_audio.getPosition() + 1.4 > m_audio.getLength()) {
			m_songs.advance(1);
			// Force reload of data
			m_playing.clear();
		}
	} else if (!m_audio.isPaused() && m_playTimer.get() > IDLE_TIMEOUT) {  // Switch if song hasn't changed for IDLE_TIMEOUT seconds
		if (!m_search.text.empty()) { m_search.text.clear(); m_songs.setFilter(m_search.text); }
		m_songs.random();
	}
}


#include "screen_players.hh"
#include "util.hh"
#include "xtime.hh"
#include <iostream>
#include <sstream>

static const double IDLE_TIMEOUT = 45.0; // seconds

ScreenPlayers::ScreenPlayers(std::string const& name, Audio& audio, Players& players):
  Screen(name), m_audio(audio), m_players(players), m_covers(20)
{
	m_players.setAnimMargins(5.0, 5.0);
	m_playTimer.setTarget(getInf()); // Using this as a simple timer counting seconds
}

void ScreenPlayers::enter() {
	theme.reset(new ThemeSongs());
	m_emptyCover.reset(new Surface(getThemePath("no_cover.svg")));
	m_search.text.clear();
	m_players.setFilter(m_search.text);
	m_audio.fadeout();
}

void ScreenPlayers::exit() {
	m_covers.clear();
	m_emptyCover.reset();
	theme.reset();
	m_video.reset();
	m_songbg.reset();
	m_playing.clear();
	m_playReq.clear();
}

void ScreenPlayers::manageEvent(SDL_Event event) {
	// ScreenManager* sm = ScreenManager::getSingletonPtr();
	if (event.type != SDL_KEYDOWN) return;
	SDL_keysym keysym = event.key.keysym;
	int key = keysym.sym;
	SDLMod mod = event.key.keysym.mod;

	if (key == SDLK_r && mod & KMOD_CTRL) { m_players.reload(); m_players.setFilter(m_search.text); }
	if (m_search.process(keysym)) m_players.setFilter(m_search.text);
	else if (key == SDLK_ESCAPE) {
		if (m_search.text.empty())/*TODO*/; // sm->activateScreen("Intro");
		else { m_search.text.clear(); m_players.setFilter(m_search.text); }
	}
	// The rest are only available when there are songs available
	else if (m_players.empty()) return;
	else if (key == SDLK_SPACE || (key == SDLK_PAUSE || (key == SDLK_p && mod & KMOD_CTRL))) m_audio.togglePause();
	else if (key == SDLK_RETURN)/*TODO*/; // sm->activateScreen("Score");
	else if (key == SDLK_LEFT) m_players.advance(-1);
	else if (key == SDLK_RIGHT) m_players.advance(1);
	else if (key == SDLK_PAGEUP) m_players.advance(-10);
	else if (key == SDLK_PAGEDOWN) m_players.advance(10);
	// else if (key == SDLK_TAB && !(mod & KMOD_ALT)) m_players.randomize();
}

void ScreenPlayers::draw() {
	double length = m_audio.getLength();
	double time = clamp(m_audio.getPosition() - config["audio/video_delay"].f(), 0.0, length);
	if (m_songbg.get()) m_songbg->draw();
	if (m_video.get()) m_video->render(time);
	std::string music, songbg, video;
	double videoGap = 0.0;
	std::ostringstream oss_song, oss_order;
	// Test if there are no songs
	if (m_players.empty()) {
		// Format the song information text
		if (m_search.text.empty()) {
			oss_song << "No players found!";
			oss_order << "Visit performous.org\n";
		} else {
			// TODO: create new player
			oss_song << "no players match search";
			oss_order << m_search.text << '\n';
		}
	} else {
		PlayerItem player = m_players.current();
		// Format the player information text
		oss_song << player.name << '\n';
		oss_order << (m_search.text.empty() ? std::string("new player") : m_search.text) << '\n';
		oss_order << "(" << m_players.currentId() + 1 << "/" << m_players.size() << ")";
		double spos = m_players.currentPosition(); // This needs to be polled to run the animation

		// Draw the covers
		std::size_t ss = m_players.size();
		int baseidx = spos + 1.5; --baseidx; // Round correctly
		double shift = spos - baseidx;
		for (int i = -2; i < 5; ++i) {
			if (baseidx + i < 0 || baseidx + i >= int(ss)) continue;
			Surface* cover = NULL;
			/*
			Song& song_display = m_players[baseidx + i];
			// Fetch cover image from cache or try loading it
			if (!song_display.cover.empty()) try { cover = &m_covers[song_display.path + song_display.cover]; } catch (std::exception const&) {}
			*/
			Surface& s = (cover ? *cover : *m_emptyCover);
			double diff = (i == 0 ? (0.5 - fabs(shift)) * 0.07 : 0.0);
			double y = 0.27 + 0.5 * diff;
			// Draw the cover
			s.dimensions.middle(-0.2 + 0.17 * (i - shift)).bottom(y - 0.2 * diff).fitInside(0.14 + diff, 0.14 + diff); s.draw();
			// Draw the reflection
			s.dimensions.top(y + 0.2 * diff); s.tex = TexCoords(0, 1, 1, 0); glColor4f(1.0, 1.0, 1.0, 0.4); s.draw();
			s.tex = TexCoords(); glColor3f(1.0, 1.0, 1.0); // Restore default attributes
		}
		/*
		if (!song.music.empty()) music = song.music[0]; // FIXME: support multiple tracks
		if (!song.background.empty()) songbg = song.path + song.background;
		if (!song.video.empty()) { video = song.path + song.video; videoGap = song.videoGap; }
		*/
	}

	// Draw song and order texts
	theme->song.draw(oss_song.str());
	theme->order.draw(oss_order.str());

		// Schedule playback change if the chosen song has changed
	if (music != m_playReq) { m_playReq = music; m_playTimer.setValue(0.0); }
	// Play/stop preview playback (if it is the time)
	if (music != m_playing && m_playTimer.get() > 0.4) {
		m_songbg.reset(); m_video.reset();
		if (music.empty()) m_audio.fadeout(); else m_audio.playPreview(music, 30.0);
		if (!songbg.empty()) try { m_songbg.reset(new Surface(songbg)); } catch (std::exception const&) {}
		if (!video.empty() && config["graphic/video"].b()) m_video.reset(new Video(video, videoGap));
		m_playing = music;
	} else if (!m_audio.isPaused() && m_playTimer.get() > IDLE_TIMEOUT) {  // Switch if song hasn't changed for IDLE_TIMEOUT seconds
		if (!m_search.text.empty()) { m_search.text.clear(); m_players.setFilter(m_search.text); }
	}
}


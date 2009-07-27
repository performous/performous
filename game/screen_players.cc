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
	m_highscore.reset(new HighScore(m_song->path, "High.sco"));
	try {
		m_highscore->load();
	} catch (HighScoreException const& hi) {
		std::cerr << "high.sco:" << hi.line() << " " << hi.what() << std::endl;
	}

	m_score_text[0].reset(new SvgTxtThemeSimple(getThemePath("sing_score_text.svg"), config["graphic/text_lod"].f()));
	m_score_text[1].reset(new SvgTxtThemeSimple(getThemePath("sing_score_text.svg"), config["graphic/text_lod"].f()));
	m_score_text[2].reset(new SvgTxtThemeSimple(getThemePath("sing_score_text.svg"), config["graphic/text_lod"].f()));
	m_score_text[3].reset(new SvgTxtThemeSimple(getThemePath("sing_score_text.svg"), config["graphic/text_lod"].f()));
	m_player_icon.reset(new Surface(getThemePath("sing_pbox.svg")));
	theme.reset(new ThemeSongs());
	m_emptyCover.reset(new Surface(getThemePath("no_cover.svg"))); // TODO use persons head
	m_search.text.clear();
	m_players.setFilter(m_search.text);
	m_audio.fadeout();
}

void ScreenPlayers::exit() {
	m_player_icon.reset();
	m_score_text[0].reset();
	m_score_text[1].reset();
	m_score_text[2].reset();
	m_score_text[3].reset();

	m_covers.clear();
	m_emptyCover.reset();
	theme.reset();
	m_video.reset();
	m_songbg.reset();
	m_playing.clear();
	m_playReq.clear();

	try {
		m_highscore->save();
	} catch (HighScoreException const& hi) {
		std::cerr << "high.sco:" << hi.line() << " " << hi.what() << std::endl;
	}
	m_highscore.reset();
}

void ScreenPlayers::manageEvent(SDL_Event event) {
	ScreenManager* sm = ScreenManager::getSingletonPtr();
	if (event.type != SDL_KEYDOWN) return;
	SDL_keysym keysym = event.key.keysym;
	int key = keysym.sym;
	SDLMod mod = event.key.keysym.mod;

	if (key == SDLK_r && mod & KMOD_CTRL) { m_players.reload(); m_players.setFilter(m_search.text); }
	if (m_search.process(keysym)) m_players.setFilter(m_search.text);
	else if (key == SDLK_ESCAPE) {
		if (m_search.text.empty()) sm->activateScreen("Songs");
		else { m_search.text.clear(); m_players.setFilter(m_search.text); }
	}
	else if (key == SDLK_RETURN)
	{
		if (m_players.empty())
		{
			m_players.addPlayer(m_search.text);
			m_players.setFilter(m_search.text);
			m_players.update();
			// the current player is the new created one
		}
		m_highscore->addNewHighscore(m_players.current().name, m_players.scores.front());
		m_players.scores.pop_front();

		if (m_players.scores.empty() || !m_highscore->reachedNewHighscore(m_players.scores.front()))
		{
			// no more highscore, we are now finished
			sm->activateScreen("Songs");
		} else {
			m_search.text.clear();
			m_players.setFilter("");
			// add all players which reach highscore because if score is very near or same it might be
			// frustrating for second one that he cannot enter, so better go for next one...
		}
	}
	// The rest are only available when there are songs available
	else if (m_players.empty()) return;
	else if (key == SDLK_SPACE || (key == SDLK_PAUSE || (key == SDLK_p && mod & KMOD_CTRL))) m_audio.togglePause();
	else if (key == SDLK_LEFT) m_players.advance(-1);
	else if (key == SDLK_RIGHT) m_players.advance(1);
	else if (key == SDLK_PAGEUP) m_players.advance(-10);
	else if (key == SDLK_PAGEDOWN) m_players.advance(10);
}

/**Draw the scores in the bottom*/
void ScreenPlayers::drawScores() {
	// Score display
	{
		unsigned int i = 0;
		for (std::list<Player>::const_iterator p = m_players.cur.begin(); p != m_players.cur.end(); ++p, ++i) {
			float act = p->activity();
			if (act == 0.0f) continue;
			glColor4f(p->m_color.r, p->m_color.g, p->m_color.b,act);
			m_player_icon->dimensions.left(-0.5 + 0.01 + 0.25 * i).fixedWidth(0.075)
				.screenBottom(-0.025);
			m_player_icon->draw();
			m_score_text[i%4]->render((boost::format("%04d") % p->getScore()).str());
			m_score_text[i%4]->dimensions().middle(-0.350 + 0.01 + 0.25 * i).fixedHeight(0.075)
				.screenBottom(-0.025);
			m_score_text[i%4]->draw();
			glColor4f(1.0, 1.0, 1.0, 1.0);
		}
	}
}

void ScreenPlayers::draw() {
	m_players.update(); // Poll for new players
	double length = m_audio.getLength();
	double time = clamp(m_audio.getPosition() - config["audio/video_delay"].f(), 0.0, length);
	if (m_songbg.get()) m_songbg->draw();
	if (m_video.get()) m_video->render(time);
	theme->bg.draw();
	std::string music, songbg, video;
	double videoGap = 0.0;
	std::ostringstream oss_song, oss_order;
	// Test if there are no songs
	if (m_players.empty()) {
		// Format the song information text
		if (m_search.text.empty()) {
			oss_song << "No players found!";
			oss_order << "Check players.txt in current\n";
			oss_order << "directory for players";
		} else {
			oss_song << "Press enter to create player!";
			oss_order << m_search.text << '\n';
		}
	} else {
		// Format the player information text
		oss_song << "You reached " << m_players.scores.front() << " points!\n";
		oss_order << "Please enter your Name!\n"
			<< "Name: " << m_players.current().name << '\n'
			<< "Search Text: "
			<< (m_search.text.empty() ? std::string("please enter") : m_search.text)
			<< '\n';
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
	drawScores();
}


#include "screen_sing.hh"

#include "util.hh"
#include "record.hh"
#include "configuration.hh"
#include "screen_players.hh"
#include "fs.hh"
#include "database.hh"
#include "video.hh"
#include "guitargraph.hh"
#include "glutil.hh"

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <utility>

namespace {
	static const double QUIT_TIMEOUT = 20.0; // Return to songs screen after 20 seconds in score screen
}

void ScreenSing::enter() {
	theme.reset(new ThemeSing());
	bool foundbg = false;
	if (!m_song->background.empty()) {
		try {
			m_background.reset(new Surface(m_song->path + m_song->background, true));
			foundbg = true;
		} catch (std::exception& e) {
			std::cerr << e.what() << std::endl;
		}
	}
	if (!m_song->video.empty() && config["graphic/video"].b()) {
		m_video.reset(new Video(m_song->path + m_song->video, m_song->videoGap));
		foundbg = true;
	}
	if (foundbg == false) {
		try {
			std::string bgpath = m_backgrounds.getRandom();
			m_background.reset(new Surface(bgpath, true));
		} catch (std::exception& e) {
			std::cerr << e.what() << std::endl;
		}		
	}
	m_pause_icon.reset(new Surface(getThemePath("sing_pause.svg")));
	m_help.reset(new Surface(getThemePath("instrumenthelp.svg")));
	m_progress.reset(new ProgressBar(getThemePath("sing_progressbg.svg"), getThemePath("sing_progressfg.svg"), ProgressBar::HORIZONTAL, 0.01f, 0.01f, true));
	m_progress->dimensions.fixedWidth(0.4).left(-0.5).screenTop();
	theme->timer.dimensions.screenTop(0.5 * m_progress->dimensions.h());
	boost::ptr_vector<Analyzer>& analyzers = m_capture.analyzers();
	m_engine.reset(new Engine(m_audio, *m_song, analyzers.begin(), analyzers.end(), m_database));
	m_layout_singer.reset(new LayoutSinger(*m_song, m_database, theme));
	// I know some purists would hang me for this loop
	if( !m_song->track_map.empty() ) {
		// Here we load alternatively guitar/bass tracks until no guitar controler is available
		// then we load all the drums tracks until no drum controler is available (and place them in second position)
		bool no_guitar = false;
		bool no_guitar2 = false;
		bool no_guitar3 = false;
		bool no_bass = false;
		while (1) {
			try {
				Instruments::iterator it = m_instruments.end();
				m_instruments.insert(it, new GuitarGraph(m_audio, *m_song, "guitar"));
			} catch (std::runtime_error&) {no_guitar = true;}
			try {
				Instruments::iterator it = m_instruments.end();
				m_instruments.insert(it, new GuitarGraph(m_audio, *m_song, "coop guitar"));
			} catch (std::runtime_error&) {no_guitar2 = true;}
			try {
				Instruments::iterator it = m_instruments.end();
				m_instruments.insert(it, new GuitarGraph(m_audio, *m_song, "bass"));
			} catch (std::runtime_error&) {no_bass = true;}
			try {
				Instruments::iterator it = m_instruments.end();
				m_instruments.insert(it, new GuitarGraph(m_audio, *m_song, "rhythm guitar"));
			} catch (std::runtime_error&) {no_guitar3 = true;}
			if( no_guitar && no_guitar2 && no_guitar3 && no_bass ) break;
		}
		while(1) {
			try {
				Instruments::iterator it = m_instruments.end();
				if (m_instruments.size() > 1) it = m_instruments.begin() + 1; // Drums go after the first guitar
				m_instruments.insert(it, new GuitarGraph(m_audio, *m_song, "drums"));
			} catch (std::runtime_error&) { break; }
		}
	}
	// Load dance tracks
	if ( !m_song->danceTracks.empty() ) {
		while(1) {
			try {
				m_dancers.push_back(new DanceGraph(m_audio, *m_song));
				break; // REMOVEME (when input assignement is correctly implemented)
			} catch (std::runtime_error&) { break; }
		}
	}
	double setup_delay = m_dancers.size() != 0 ? -5.0 : (m_instruments.empty() ? -1.0 : -8.0);
	m_audio.playMusic(m_song->music, false, 0.0, setup_delay); // Startup delay for instruments is longer than for singing only
}

void ScreenSing::instrumentLayout(double time) {
	for (Instruments::iterator it = m_instruments.begin(); it != m_instruments.end();) {
		if (it->dead(time)) {
			std::cout << it->getTrack() << " was thrown out after " << time << " inactive" << std::endl;
			it = m_instruments.erase(it);
		} else ++it;
	}
	double iw = std::min(0.5, 1.0 / m_instruments.size());
	for (Instruments::size_type i = 0, s = m_instruments.size(); i < s; ++i) {
		m_instruments[i].position((0.5 + i - 0.5 * s) * iw, iw);
	}
	typedef std::pair<unsigned, double> CountSum;
	std::map<std::string, CountSum> volume; // stream id to (count, sum)
	for (Instruments::iterator it = m_instruments.begin(); it != m_instruments.end(); ++it) {
		it->engine();
		CountSum& cs = volume[it->getTrackIndex()];
		cs.first++;
		cs.second += it->correctness();
		it->draw(time);
	}
	if (time < -0.5) {
		glColor4f(1.0f, 1.0f, 1.0f, clamp(-1.0 - 2.0 * time));
		m_help->draw();
		glColor3f(1.0f, 1.0f, 1.0f);
	}
	// Set volume levels (averages of all instruments playing that track)
	for( std::map<std::string,std::string>::iterator it = m_song->music.begin() ; it != m_song->music.end() ; ++it ) {
		double level = 1.0;
		if( volume.find(it->first) == volume.end() ) {
			m_audio.streamFade(it->first, level);
		} else {
			CountSum cs = volume[it->first];
			if (cs.first > 0) level = cs.second / cs.first;
			if (m_song->music.size() <= 1) level = std::max(0.333, level);
			m_audio.streamFade(it->first, level);
		}
	}
}

void ScreenSing::danceLayout(double time) {
	double iw = std::min(0.5, 1.0 / m_dancers.size());
	Dancers::size_type i = 0;
	for (Dancers::iterator it = m_dancers.begin(); it != m_dancers.end(); ++it, ++i) {
		it->position((0.5 + i - 0.5 * m_dancers.size()) * iw, iw);
		it->engine();
		it->draw(time);
	}
	if (time < -0.5) {
		glColor4f(1.0f, 1.0f, 1.0f, clamp(-1.0 - 2.0 * time));
		// TODO: help?
		glColor3f(1.0f, 1.0f, 1.0f);
	}
}

void ScreenSing::exit() {
	m_score_window.reset();
	m_instruments.clear();
	m_dancers.clear();
	m_layout_singer.reset();
	m_engine.reset();
	m_help.reset();
	m_pause_icon.reset();
	m_video.reset();
	m_background.reset();
	theme.reset();
}

void ScreenSing::activateNextScreen()
{
	ScreenManager* sm = ScreenManager::getSingletonPtr();

	m_database.addSong(m_song);
	if (m_database.scores.empty() || !m_database.reachedHiscore(m_song)) {
		// if no highscore reached..
		sm->activateScreen("Songs");
		return;
	}

	// Score window visible -> Enter quits to Players Screen
	Screen* s = sm->getScreen("Players");
	ScreenPlayers* ss = dynamic_cast<ScreenPlayers*> (s);
	assert(ss);
	ss->setSong(m_song);
	sm->activateScreen("Players");
}

void ScreenSing::manageEvent(SDL_Event event) {
	if (event.type == SDL_KEYDOWN) {
		double time = m_audio.getPosition();
		Song::Status status = m_song->status(time);
		m_quitTimer.setValue(QUIT_TIMEOUT);
		bool seekback = false;
		int key = event.key.keysym.sym;
		if (key == SDLK_ESCAPE || key == SDLK_q) {
			// In ScoreWindow ESC goes to Players, otherwise insta-quit to Songs
			if (m_score_window.get() && key == SDLK_ESCAPE) { activateNextScreen(); return; }
			ScreenManager* sm = ScreenManager::getSingletonPtr();
			sm->activateScreen("Songs");
			return;
		}
		else if (key == SDLK_RETURN) {
			if (m_score_window.get()) { activateNextScreen(); return; } // Score window visible -> Enter quits
			else if (status == Song::FINISHED && m_song->track_map.empty()) {
				m_engine->kill(); // kill the engine thread (to avoid consuming memory)
				m_score_window.reset(new ScoreWindow(m_instruments, m_database)); // Song finished, but no score window -> show it
			}
		}
		else if (key == SDLK_SPACE && m_score_window.get()) { activateNextScreen(); return; } // Score window visible -> Space quits
		else if (key == SDLK_SPACE || key == SDLK_PAUSE) m_audio.togglePause();
		if (m_score_window.get()) return;
		// The rest are only available when score window is not displayed and when there are no instruments
		if (key == SDLK_RETURN && status == Song::INSTRUMENTAL_BREAK && m_song->track_map.empty()) {
			double diff = m_layout_singer->lyrics_begin() - 3.0 - time;
			if (diff > 0.0) m_audio.seek(diff);
		}
		else if (key == SDLK_F4) m_audio.toggleSynth(m_song->notes);
		else if (key == SDLK_F5) --config["audio/video_delay"];
		else if (key == SDLK_F6) ++config["audio/video_delay"];
		else if (key == SDLK_F7) --config["audio/round-trip"];
		else if (key == SDLK_F8) ++config["audio/round-trip"];
		else if (key == SDLK_F9) ++config["game/karaoke_mode"];
		else if (key == SDLK_F10) ++config["game/pitch"];
		else if (key == SDLK_HOME) m_audio.seekPos(0.0);
		else if (key == SDLK_LEFT) { m_audio.seek(-5.0); seekback = true; }
		else if (key == SDLK_RIGHT) m_audio.seek(5.0);
		else if (key == SDLK_UP) m_audio.seek(30.0);
		else if (key == SDLK_DOWN) { m_audio.seek(-30.0); seekback = true; }
		else if (key == SDLK_k && event.key.keysym.mod & KMOD_SHIFT) { m_audio.streamFade("vocals", 1.0); }
		else if (key == SDLK_k && !(event.key.keysym.mod & KMOD_SHIFT)) { m_audio.streamFade("vocals", 0.0); }
		else if (key == SDLK_r && event.key.keysym.mod & KMOD_CTRL) {
			exit(); m_song->reload(); enter();
			m_audio.seek(time);
		}
		// Some things must be reset after seeking backwards
		if (seekback) {
			m_layout_singer->reset();
		}
	} else if (event.type == SDL_JOYBUTTONDOWN) {
		int button = event.jbutton.button;
		if (button == 9 /* START */) m_audio.togglePause();
		if (button == 8 /* SELECT */) {
			ScreenManager::getSingletonPtr()->activateScreen("Songs");
			return;
		}
	}
}

namespace {

	const double arMin = 1.33;
	const double arMax = 2.35;

	void fillBG() {
		Dimensions dim(arMin);
		dim.fixedWidth(1.0);
		glutil::Begin block(GL_QUADS);
		glVertex2f(dim.x1(), dim.y1());
		glVertex2f(dim.x2(), dim.y1());
		glVertex2f(dim.x2(), dim.y2());
		glVertex2f(dim.x1(), dim.y2());
	}

}

void ScreenSing::draw() {
	// Get the time in the song
	double length = m_audio.getLength();
	double time = m_audio.getPosition();
	time -= config["audio/video_delay"].f();
	double songPercent = clamp(time / length);

	// Rendering starts
	{
		double ar = arMax;
		if (m_background) {
			ar = m_background->dimensions.ar();
			if (ar > arMax || (m_video && ar > arMin)) fillBG();  // Fill white background to avoid black borders
			m_background->draw();
		} else fillBG();
		if (m_video) { m_video->render(time); double tmp = m_video->dimensions().ar(); if (tmp > 0.0) ar = tmp; }
		ar = clamp(ar, arMin, arMax);
		double offset = 0.5 / ar + 0.2;
		theme->bg_bottom.dimensions.fixedWidth(1.0).bottom(offset);
		theme->bg_bottom.draw();
		theme->bg_top.dimensions.fixedWidth(1.0).top(-offset);
		theme->bg_top.draw();
	}

	if( !m_dancers.empty() ) {
		danceLayout(time);
		//m_layout_singer->draw(time, LayoutSinger::LEFT);
	} else if( m_instruments.empty() ) {
		m_layout_singer->draw(time, LayoutSinger::BOTTOM);
	} else {
		instrumentLayout(time);
		m_layout_singer->draw(time, LayoutSinger::MIDDLE);
	}

	Song::Status status = m_song->status(time);

	// Compute and draw the timer and the progressbar
	{
		unsigned t = clamp(time, 0.0, length);
		m_progress->draw(songPercent);
		std::string statustxt = (boost::format("%02u:%02u") % (t / 60) % (t % 60)).str();
		if (!m_score_window.get() && m_song->track_map.empty() && m_song->danceTracks.empty()) {
			if (status == Song::INSTRUMENTAL_BREAK) statustxt += "   ENTER to skip instrumental break";
			if (status == Song::FINISHED && !config["game/karaoke_mode"].b()) statustxt += "   Remember to wait for grading!";
		}
		theme->timer.draw(statustxt);
	}

	if (config["game/karaoke_mode"].b()) {
		if (!m_audio.isPlaying()) {
			ScreenManager* sm = ScreenManager::getSingletonPtr();
			sm->activateScreen("Songs");
			return;
		}
	} else {
		if (m_score_window.get()) {
			// Score window has been created (we are near the end)
			if (m_score_window->empty()) {  // No players to display scores for
				if (!m_audio.isPlaying()) { activateNextScreen(); return; }
			} else {  // Window being displayed
				if (m_quitTimer.get() == 0.0 && !m_audio.isPaused()) { activateNextScreen(); return; }
				m_score_window->draw();
			}
		}
		else if (!m_audio.isPlaying() || (status == Song::FINISHED && m_audio.getLength() - time < 3.0)) {
			// Time to create the score window
			m_quitTimer.setValue(QUIT_TIMEOUT);
			m_engine->kill(); // kill the engine thread (to avoid consuming memory)
			m_score_window.reset(new ScoreWindow(m_instruments, m_database));
		}
	}
		
	if (m_audio.isPaused()) {
		m_pause_icon->dimensions.middle().center().fixedWidth(.25);
		m_pause_icon->draw();
	}
}

ScoreWindow::ScoreWindow(Instruments& instruments, Database& database):
  m_database(database),
  m_pos(0.8, 2.0),
  m_bg(getThemePath("score_window.svg")),
  m_scoreBar(getThemePath("score_bar_bg.svg"), getThemePath("score_bar_fg.svg"), ProgressBar::VERTICAL, 0.0, 0.0, false),
  m_score_text(getThemePath("score_txt.svg")),
  m_score_rank(getThemePath("score_rank.svg"))
{
	m_pos.setTarget(0.0);
	m_database.scores.clear();
	for (std::list<Player>::iterator p = m_database.cur.begin(); p != m_database.cur.end();) {
		ScoreItem item;
		item.score = p->getScore();
		item.track = "Vocals";
		item.track_simple = "vocals";
		item.color = glutil::Color(p->m_color.r, p->m_color.g, p->m_color.b);
		
		if (item.score < 500) { p = m_database.cur.erase(p); continue; }
		m_database.scores.push_back(item);
		++p;
	}
	for (Instruments::iterator it = instruments.begin(); it != instruments.end();) {
		ScoreItem item;
		item.score = it->getScore();
		item.track_simple = it->getTrack();
		item.track = it->getTrack() + " - " + it->getDifficultyString();
		item.track[0] = toupper(item.track[0]);
		if (item.score < 100) { it = instruments.erase(it); continue; }
		
		if (item.track_simple == "drums") item.color = glutil::Color(0.1f, 0.1f, 0.1f);
		else if (item.track_simple == "bass") item.color = glutil::Color(0.5f, 0.3f, 0.1f);
		else item.color = glutil::Color(1.0f, 0.0f, 0.0f);
		
		m_database.scores.push_back(item);
		++it;
	}

	if (m_database.scores.empty())
		m_rank = "No player!";
	else {
		m_database.scores.sort();
		m_database.scores.reverse(); // top should be first
		int topScore = m_database.scores.front().score;
		if (m_database.scores.front().track_simple == "vocals") {
			if (topScore > 8000) m_rank = "Hit singer";
			else if (topScore > 6000) m_rank = "Lead singer";
			else if (topScore > 4000) m_rank = "Rising star";
			else if (topScore > 2000) m_rank = "Amateur";
			else m_rank = "Tone deaf";
		} else {
			if (topScore > 8000) m_rank = "Virtuoso";
			else if (topScore > 6000) m_rank = "Rocker";
			else if (topScore > 4000) m_rank = "Rising star";
			else if (topScore > 2000) m_rank = "Amateur";
			else m_rank = "Tone deaf";
		}
	}
	m_bg.dimensions.middle().center();
	m_scoreBar.dimensions.fixedWidth(0.09);
}

void ScoreWindow::draw() {
	glutil::PushMatrix block;
	glTranslatef(0.0, m_pos.get(), 0.0);
	m_bg.draw();
	const double spacing = 0.1 + 0.1 / m_database.scores.size();
	unsigned i = 0;

	for (Database::cur_scores_t::const_iterator p = m_database.scores.begin(); p != m_database.scores.end(); ++p, ++i) {
		int score = p->score;
		glColor4fv(p->color);
		double x = -0.12 + spacing * (0.5 + i - 0.5 * m_database.scores.size());
		m_scoreBar.dimensions.middle(x).bottom(0.20);
		m_scoreBar.draw(score / 10000.0);
		m_score_text.render(boost::lexical_cast<std::string>(score));
		m_score_text.dimensions().middle(x).top(0.24).fixedHeight(0.05);
		m_score_text.draw();
		m_score_text.render(p->track_simple);
		m_score_text.dimensions().middle(x).top(0.20).fixedHeight(0.05);
		m_score_text.draw();
		glColor3f(1.0f, 1.0f, 1.0f);
	}
	m_score_rank.draw(m_rank);
}


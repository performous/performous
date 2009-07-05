#include "screen_sing.hh"

#include "util.hh"
#include "configuration.hh"
#include "xtime.hh"
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include "songs.hh"
#include <iostream>
#include <iomanip>

namespace {
	static const double QUIT_TIMEOUT = 20.0; // Return to songs screen after 20 seconds in score screen
}

void ScreenSing::enter() {
	Song& song = m_songs.current();
	std::string file = song.path + song.mp3;
	m_audio.playMusic(file.c_str());
	theme.reset(new ThemeSing());
	if (!song.background.empty()) {
		try {
			m_background.reset(new Surface(song.path + song.background, true));
		} catch (std::exception& e) {
			std::cerr << e.what() << std::endl;
		}
	}
	if (!song.video.empty() && config["graphic/video"].b()) m_video.reset(new Video(song.path + song.video, song.videoGap));
	m_pause_icon.reset(new Surface(getThemePath("sing_pause.svg")));
	m_score_text[0].reset(new SvgTxtThemeSimple(getThemePath("sing_score_text.svg"), config["graphic/text_lod"].f()));
	m_score_text[1].reset(new SvgTxtThemeSimple(getThemePath("sing_score_text.svg"), config["graphic/text_lod"].f()));
	m_player_icon.reset(new Surface(getThemePath("sing_pbox.svg")));
	m_progress.reset(new ProgressBar(getThemePath("sing_progressbg.svg"), getThemePath("sing_progressfg.svg"), ProgressBar::HORIZONTAL, 0.01f, 0.01f, true));
	m_progress->dimensions.fixedWidth(0.4).left(-0.5).screenTop();
	theme->timer.dimensions.screenTop(0.5 * m_progress->dimensions.h());
	m_lyricit = song.notes.begin();
	boost::ptr_vector<Analyzer>& analyzers = m_capture.analyzers();
	m_engine.reset(new Engine(m_audio, m_songs.current(), analyzers.begin(), analyzers.end()));
	m_noteGraph.reset(new NoteGraph(song));
}

void ScreenSing::exit() {
	m_score_window.reset();
	m_lyrics.clear();
	m_noteGraph.reset();
	m_engine.reset();
	m_pause_icon.reset();
	m_player_icon.reset();
	m_video.reset();
	m_background.reset();
	m_score_text[0].reset();
	m_score_text[1].reset();
	theme.reset();
}

void ScreenSing::manageEvent(SDL_Event event) {
	if (event.type == SDL_KEYDOWN) {
		double time = m_audio.getPosition();
		Song::Status status = m_songs.current().status(time);
		m_quitTimer.setValue(QUIT_TIMEOUT);
		bool seekback = false;
		ScreenManager* sm = ScreenManager::getSingletonPtr();
		int key = event.key.keysym.sym;
		if (key == SDLK_ESCAPE || key == SDLK_q) sm->activateScreen("Songs");
		else if (key == SDLK_RETURN) {
			if (m_score_window.get()) sm->activateScreen("Songs");  // Score window visible -> Enter quits
			else if (status == Song::FINISHED) m_score_window.reset(new ScoreWindow(*m_engine)); // Song finished, but no score window -> show it
		}
		else if (key == SDLK_SPACE || key == SDLK_PAUSE) m_audio.togglePause();
		if (m_score_window.get()) return;
		// The rest are only available when score window is not displayed
		if (key == SDLK_RETURN && status == Song::INSTRUMENTAL_BREAK) {
			double diff = m_lyricit->begin - 3.0 - time;
			if (diff > 0.0) m_audio.seek(diff);
		}
		else if (key == SDLK_F4) m_audio.toggleSynth(m_songs.current().notes);
		else if (key == SDLK_F5) --config["audio/video_delay"];
		else if (key == SDLK_F6) ++config["audio/video_delay"];
		else if (key == SDLK_F7) --config["audio/round-trip"];
		else if (key == SDLK_F8) ++config["audio/round-trip"];
		else if (key == SDLK_F9) ++config["game/karaoke_mode"];
		else if (key == SDLK_F10) ++config["game/pitch"];
		else if (key == SDLK_HOME) m_audio.seek(-m_audio.getPosition());
		else if (key == SDLK_LEFT) { m_audio.seek(-5.0); seekback = true; }
		else if (key == SDLK_RIGHT) m_audio.seek(5.0);
		else if (key == SDLK_UP) m_audio.seek(30.0);
		else if (key == SDLK_DOWN) { m_audio.seek(-30.0); seekback = true; }
		else if (key == SDLK_r && event.key.keysym.mod & KMOD_CTRL) {
			exit(); m_songs.current().reload(); enter();
			m_audio.seek(time);
		}
		// Some things must be reset after seeking backwards
		if (seekback) {
			m_lyricit = m_songs.current().notes.begin();
			m_lyrics.clear();
		}
	}
}

namespace {

	const double arMin = 1.33;
	const double arMax = 2.35;

	void fillBG() {
		Dimensions dim(arMin);
		dim.fixedWidth(1.0);
		glBegin(GL_QUADS);
		glVertex2f(dim.x1(), dim.y1());
		glVertex2f(dim.x2(), dim.y1());
		glVertex2f(dim.x2(), dim.y2());
		glVertex2f(dim.x1(), dim.y2());
		glEnd();		
	}

}

void ScreenSing::draw() {
	ScreenManager* sm = ScreenManager::getSingletonPtr();
	Song& song = m_songs.current();
	// Get the time in the song
	double length = m_audio.getLength();
	double time = clamp(m_audio.getPosition() - config["audio/video_delay"].f(), 0.0, length);
	double songPercent = time / length;

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

	if (!config["game/karaoke_mode"].b()) drawNonKaraoke(time);

	// Compute and draw lyrics
	{
		const double basepos = -0.1;
		const double linespacing = 0.06;
		bool dirty;
		do {
			dirty = false;
			if (!m_lyrics.empty() && m_lyrics[0].expired(time)) {
				// Add extra spacing to replace the removed row
				if (m_lyrics.size() > 1) m_lyrics[1].extraspacing.move(m_lyrics[0].extraspacing.get() + 1.0);
				m_lyrics.pop_front();
				dirty = true;
			}
			if (!dirty && m_lyricit != song.notes.end() && m_lyricit->begin < time + 4.0) {
				m_lyrics.push_back(LyricRow(m_lyricit, song.notes.end()));
				dirty = true;
			}
		} while (dirty);
		double pos = basepos;
		for (size_t i = 0; i < m_lyrics.size(); ++i, pos += linespacing) {
			pos += m_lyrics[i].extraspacing.get() * linespacing;
			if (i == 0) m_lyrics[0].draw(theme->lyrics_now, time, pos);
			else if (i == 1) m_lyrics[1].draw(theme->lyrics_next, time, pos);
		}
	}

	Song::Status status = song.status(time);

	// Compute and draw the timer and the progressbar
	{
		m_progress->draw(songPercent);
		std::string statustxt = (boost::format("%02u:%02u") % (unsigned(time) / 60) % (unsigned(time) % 60)).str();
		if (!m_score_window.get()) {
			if (status == Song::INSTRUMENTAL_BREAK) statustxt += "   ENTER to skip instrumental break";
			if (status == Song::FINISHED && !config["game/karaoke_mode"].b()) statustxt += "   Remember to wait for grading!";
		}
		theme->timer.draw(statustxt);
	}

	if (config["game/karaoke_mode"].b()) {
		if (!m_audio.isPlaying()) sm->activateScreen("Songs");
	} else {
		if (m_score_window.get()) {
			if (m_quitTimer.get() == 0.0 && !m_audio.isPaused()) { sm->activateScreen("Songs"); return; }
			m_score_window->draw();
		}
		else if (!m_audio.isPlaying() || (status == Song::FINISHED && m_audio.getLength() - time < 3.0)) {
			m_quitTimer.setValue(QUIT_TIMEOUT);
			m_score_window.reset(new ScoreWindow(*m_engine));
		}
	}
		
	if (m_audio.isPaused()) {
		m_pause_icon->dimensions.middle().center().fixedWidth(.25);
		m_pause_icon->draw();
	}
}

void ScreenSing::drawNonKaraoke(double time) {
	std::list<Player> const& players = m_engine->getPlayers();
	m_noteGraph->draw(time, players); // Draw notes and pitch waves

	// Score display
	{
		unsigned int i = 0;
		for (std::list<Player>::const_iterator p = players.begin(); p != players.end(); ++p, ++i) {
			float act = p->activity();
			if (act == 0.0f) continue;
			glColor4f(p->m_color.r, p->m_color.g, p->m_color.b,act);
			m_player_icon->dimensions.left(-0.5 + 0.01 + 0.25 * i).fixedWidth(0.075).screenTop(0.055);
			m_player_icon->draw();
			m_score_text[i%2]->render((boost::format("%04d") % p->getScore()).str());
			m_score_text[i%2]->dimensions().middle(-0.350 + 0.01 + 0.25 * i).fixedHeight(0.075).screenTop(0.055);
			m_score_text[i%2]->draw();
			glColor4f(1.0, 1.0, 1.0, 1.0);
		}
	}
}

ScoreWindow::ScoreWindow(Engine& e):
  m_pos(0.8, 2.0),
  m_bg(getThemePath("score_window.svg")),
  m_scoreBar(getThemePath("score_bar_bg.svg"), getThemePath("score_bar_fg.svg"), ProgressBar::VERTICAL, 0.0, 0.0, false),
  m_score_text(getThemePath("score_txt.svg")),
  m_score_rank(getThemePath("score_rank.svg")),
  m_players(e.getPlayers())
{
	m_pos.setTarget(0.0);
	unsigned int topScore = 0;
	e.kill(); // kill the engine thread (to avoid consuming memory)
	for (std::list<Player>::iterator p = m_players.begin(); p != m_players.end();) {
		unsigned int score = p->getScore();
		if (score < 500) { p = m_players.erase(p); continue; }
		if (score > topScore) topScore = score;
		++p;
	}
	if (m_players.empty()) m_rank = "No singer!";
	else if (topScore > 8000) m_rank = "Hit singer";
	else if (topScore > 6000) m_rank = "Lead singer";
	else if (topScore > 4000) m_rank = "Rising star";
	else if (topScore > 2000) m_rank = "Amateur";
	else m_rank = "Tone deaf";
	m_bg.dimensions.middle().center();
	m_scoreBar.dimensions.fixedWidth(0.1);
}

void ScoreWindow::draw() {
	struct PushMatrixBlock { PushMatrixBlock() { glPushMatrix(); } ~PushMatrixBlock() { glPopMatrix(); } } b;
	glTranslatef(0.0, m_pos.get(), 0.0);
	m_bg.draw();
	const double spacing = 0.1 + 0.1 / m_players.size();
	unsigned i = 0;

	for (std::list<Player>::const_iterator p = m_players.begin(); p != m_players.end(); ++p, ++i) {
		int score = p->getScore();
		glColor3f(p->m_color.r, p->m_color.g, p->m_color.b);
		double x = -0.12 + spacing * (0.5 + i - 0.5 * m_players.size());
		m_scoreBar.dimensions.middle(x).bottom(0.24);
		m_scoreBar.draw(score / 10000.0);
		m_score_text.render(boost::lexical_cast<std::string>(score));
		m_score_text.dimensions().middle(x).top(0.24).fixedHeight(0.05);
		m_score_text.draw();
		glColor3f(1.0f, 1.0f, 1.0f);
	}
	m_score_rank.draw(m_rank);
}


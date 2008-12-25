#include "screen_sing.hh"

#include "util.hh"
#include "xtime.hh"
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include "songs.hh"
#include <iostream>
#include <iomanip>

namespace {
	static const double QUIT_TIMEOUT = 30.0; // Return to songs screen after 30 seconds in score screen
}

ScreenSing::SongStatus ScreenSing::songStatus() const {
	Song& song = m_songs.current();
	if (m_songit == song.notes.end()) return FINISHED;
	if (m_lyrics.empty() && m_lyricit != song.notes.end()) return INSTRUMENTAL_BREAK;
	return NORMAL;
}

void ScreenSing::enter() {
	ScreenManager* sm = ScreenManager::getSingletonPtr();
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
	if (!song.video.empty()) m_video.reset(new Video(song.path + song.video));
	m_wave.reset(new Texture(sm->getThemePathFile("wave.png")));
	m_notelines.reset(new Texture(sm->getThemePathFile("notelines.svg")));
	m_notebar.reset(new Texture(sm->getThemePathFile("notebar.svg")));
	m_notebar_hl.reset(new Texture(sm->getThemePathFile("notebar.png")));
	m_notebarfs.reset(new Texture(sm->getThemePathFile("notebarfs.svg")));
	m_notebarfs_hl.reset(new Texture(sm->getThemePathFile("notebarfs-hl.png")));
	m_pause_icon.reset(new Surface(sm->getThemePathFile("sing_pause.svg")));
	m_score_text[0].reset(new SvgTxtThemeSimple(sm->getThemePathFile("sing_score_text.svg")));
	m_score_text[1].reset(new SvgTxtThemeSimple(sm->getThemePathFile("sing_score_text.svg")));
	m_player_icon.reset(new Surface(sm->getThemePathFile("sing_pbox.svg")));
	m_notebargold.reset(new Texture(sm->getThemePathFile("notebargold.svg")));
	m_notebargold_hl.reset(new Texture(sm->getThemePathFile("notebargold.png")));
	m_progress.reset(new ProgressBar(sm->getThemePathFile("sing_progressbg.svg"), sm->getThemePathFile("sing_progressfg.svg"), ProgressBar::HORIZONTAL, 0.01, 0.01, true));
	m_progress->dimensions.fixedWidth(0.4).left(-0.5).screenTop();
	theme->timer->dimensions.screenTop(0.5 * m_progress->dimensions.h());
	m_lyricit = m_songit = song.notes.begin();
	m_notealpha = 0.0f;
	m_nlTop.setTarget(song.noteMax, true);
	m_nlBottom.setTarget(song.noteMin, true);
	boost::ptr_vector<Analyzer>& analyzers = m_capture.analyzers();
	m_engine.reset(new Engine(m_audio, m_songs.current(), analyzers.begin(), analyzers.end()));
}

void ScreenSing::exit() {
	m_score_window.reset();
	m_lyrics.clear();
	m_engine.reset();
	m_audio.stopMusic();
	m_notebargold_hl.reset();
	m_notebargold.reset();
	m_pause_icon.reset();
	m_player_icon.reset();
	m_notebarfs_hl.reset();
	m_notebarfs.reset();
	m_notebar_hl.reset();
	m_notebar.reset();
	m_notelines.reset();
	m_wave.reset();
	m_video.reset();
	m_background.reset();
	m_score_text[0].reset();
	m_score_text[1].reset();
	theme.reset();
}

void ScreenSing::manageEvent(SDL_Event event) {
	if (event.type == SDL_KEYDOWN) {
		m_quitTimer.setValue(QUIT_TIMEOUT);
		bool seekback = false;
		ScreenManager* sm = ScreenManager::getSingletonPtr();
		int key = event.key.keysym.sym;
		if (key == SDLK_ESCAPE || key == SDLK_q || (key == SDLK_RETURN && songStatus() == FINISHED)) {
			// Enter at end of song display score window, except if score window is already displayed
			if (key != SDLK_RETURN || m_score_window.get()) sm->activateScreen("Songs");
			else m_score_window.reset(new ScoreWindow(sm, *m_engine));
		}
		else if (key == SDLK_SPACE || key == SDLK_PAUSE) m_audio.togglePause();
		if (m_score_window.get()) return;
		// The rest are only available when score window is not displayed
		if (key == SDLK_RETURN && songStatus() == INSTRUMENTAL_BREAK) {
			double diff = m_songit->begin - 3.0 - m_audio.getPosition();
			if (diff > 0.0) m_audio.seek(diff);
		} // XXX: switch/case
		else if (key == SDLK_F4) m_audio.toggleSynth(m_songs.current().notes);
		else if (key == SDLK_F5) m_latencyAV -= 0.02;
		else if (key == SDLK_F6) m_latencyAV += 0.02;
		else if (key == SDLK_F7) m_engine->setLatencyAR(m_engine->getLatencyAR() - 0.02);
		else if (key == SDLK_F8) m_engine->setLatencyAR(m_engine->getLatencyAR() + 0.02);
		else if (key == SDLK_HOME) m_audio.seek(-m_audio.getPosition());
		else if (key == SDLK_LEFT) { m_audio.seek(-5.0); seekback = true; }
		else if (key == SDLK_RIGHT) m_audio.seek(5.0);
		else if (key == SDLK_UP) m_audio.seek(30.0);
		else if (key == SDLK_DOWN) { m_audio.seek(-30.0); seekback = true; }
		else if (key == SDLK_r && event.key.keysym.mod & KMOD_CTRL) {
			double pos = m_audio.getPosition();
			m_songs.current().reload();
			exit(); enter();
			m_audio.seek(pos);
		}
		m_latencyAV = clamp(round(m_latencyAV * 1000.0) / 1000.0, -0.5, 0.5);
		if (key == SDLK_F5 || key == SDLK_F6 || key == SDLK_F7 || key == SDLK_F8) std::cout << "AV latency: " << m_latencyAV << ", AR latency: " << m_engine->getLatencyAR() << std::endl;
		// Some things must be reset after seeking backwards
		if (seekback) {
			m_lyricit = m_songit = m_songs.current().notes.begin();
			m_lyrics.clear();
		}
	}
}

namespace {
	void drawNotebar(Texture const& texture, double x, double y, double w, double h) {
		UseTexture tblock(texture);
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0.0f, 0.0f); glVertex2f(x, y);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(x, y + h);
		if (w >= 2.0 * h) {
			glTexCoord2f(0.5f, 0.0f); glVertex2f(x + h, y);
			glTexCoord2f(0.5f, 1.0f); glVertex2f(x + h, y + h);
			glTexCoord2f(0.5f, 0.0f); glVertex2f(x + w - h, y);
			glTexCoord2f(0.5f, 1.0f); glVertex2f(x + w - h, y + h);
		} else {
			float crop = 0.25f * w / h;
			glTexCoord2f(crop, 0.0f); glVertex2f(x + 0.5 * w, y);
			glTexCoord2f(crop, 1.0f); glVertex2f(x + 0.5 * w, y + h);
			glTexCoord2f(1.0f - crop, 0.0f); glVertex2f(x + 0.5 * w, y);
			glTexCoord2f(1.0f - crop, 1.0f); glVertex2f(x + 0.5 * w, y + h);
		}
		glTexCoord2f(1.0f, 0.0f); glVertex2f(x + w, y);
		glTexCoord2f(1.0f, 1.0f); glVertex2f(x + w, y + h);
		glEnd();
	}

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
	double time = clamp(m_audio.getPosition() - m_latencyAV, 0.0, length);
	double songPercent = time / length;

	// Rendering starts
	{
		double ar = arMax;
		if (m_background) {
			ar = m_background->dimensions.ar();
			if (ar > arMax || (m_video && ar > arMin)) fillBG();  // Fill white background to avoid black borders
			m_background->draw();
		} else fillBG();
		if (m_video) { m_video->render(time - song.videoGap); double tmp = m_video->dimensions().ar(); if (tmp > 0.0) ar = tmp; }
		ar = clamp(ar, arMin, arMax);
		double offset = 0.5 / ar + 0.2;
		theme->bg_bottom->dimensions.fixedWidth(1.0).bottom(offset);
		theme->bg_bottom->draw();
		theme->bg_top->dimensions.fixedWidth(1.0).top(-offset);
		theme->bg_top->draw();
	}
	const double baseLine = -0.2;
	const double pixUnit = 0.2;
	// Update m_songit (which note to start the rendering from)
	while (m_songit != song.notes.end() && (m_songit->type == Note::SLEEP || m_songit->end < time - (baseLine + 0.5) / pixUnit)) ++m_songit;

	// Automatically zooming notelines
	{
		int low = song.noteMax;
		int high = song.noteMin;
		int low2 = song.noteMax;
		int high2 = song.noteMin;
		for (Notes::const_iterator it = m_songit; it != song.notes.end() && it->begin < time + 15.0; ++it) {
			if (it->type == Note::SLEEP) continue;
			if (it->note < low) low = it->note;
			if (it->note > high) high = it->note;
			if (it->begin > time + 8.0) continue;
			if (it->note < low2) low2 = it->note;
			if (it->note > high2) high2 = it->note;
		}
		if (low2 <= high2) {
			m_nlTop.setRange(high2, high);
			m_nlBottom.setRange(low, low2);
		}
	}
	double max = m_nlTop.get() + 7.0;
	double min = m_nlBottom.get() - 7.0;
	double noteUnit = -0.5 / std::max(24.0, max - min);
	double baseY = -0.5 * (min + max) * noteUnit;
	double baseX = baseLine - time * pixUnit;

	// Draw note lines
	if (m_songit == song.notes.end() || m_songit->begin > time + 3.0) m_notealpha -= 0.02f;
	else if (m_notealpha < 1.0f) m_notealpha += 0.02f;
	std::list<Player> players = m_engine->getPlayers();
	if (m_notealpha <= 0.0f) {
		m_notealpha = 0.0f;
	} else {
		glColor4f(1.0, 1.0, 1.0, m_notealpha);
		m_notelines->draw(Dimensions().stretch(1.0, (max - min - 13) * noteUnit), TexCoords(0.0, (-min - 7.0) / 12.0f, 1.0, (-max + 6.0) / 12.0f));

		// Draw notes XXX
		{
			for (Notes::const_iterator it = m_songit; it != song.notes.end() && it->begin < time - (baseLine - 0.5) / pixUnit; ++it) {
				if (it->type == Note::SLEEP) continue;
				double alpha = it->power;
				Texture* t1;
				Texture* t2;
				switch (it->type) {
				  case Note::NORMAL: t1 = m_notebar.get(); t2 = m_notebar_hl.get(); break;
				  case Note::GOLDEN: t1 = m_notebargold.get(); t2 = m_notebargold_hl.get(); break;
				  case Note::FREESTYLE:  // Freestyle notes use custom handling
					{
						Dimensions dim;
						dim.middle(baseX + 0.5 * (it->begin + it->end) * pixUnit).center(baseY + it->note * noteUnit).stretch((it->end - it->begin) * pixUnit, -noteUnit * 12.0);
						float xoffset = 0.1 * time / m_notebarfs->ar();
						m_notebarfs->draw(dim, TexCoords(xoffset, 0.0, xoffset + dim.ar() / m_notebarfs->ar(), 1.0));
						if (alpha > 0.0) {
							float xoffset = rand() / double(RAND_MAX);
							m_notebarfs_hl->draw(dim, TexCoords(xoffset, 0.0, xoffset + dim.ar() / m_notebarfs_hl->ar(), 1.0));
						}
					}
					continue;
				  default: throw std::logic_error("Unknown note type: don't know how to render");
				}
				double x = baseX + it->begin * pixUnit + noteUnit; // left x coordinate: begin minus border (side borders -noteUnit wide)
				double y = baseY + (it->note + 1) * noteUnit; // top y coordinate (on the one higher note line)
				double w = (it->end - it->begin) * pixUnit - noteUnit * 2.0; // width: including borders on both sides
				double h = -noteUnit * 2.0; // height: 0.5 border + 1.0 bar + 0.5 border = 2.0
				drawNotebar(*t1, x, y, w, h);
				if (alpha > 0.0) {
					glColor4f(1.0f, 1.0f, 1.0f, alpha * m_notealpha);
					drawNotebar(*t2, x, y, w, h);
					glColor4f(1.0f, 1.0f, 1.0f, m_notealpha);
				}
			}
		}

		// Pitch graph XXX
		UseTexture tblock(*m_wave);
		
		for (std::list<Player>::const_iterator p = players.begin(); p != players.end(); ++p) {
			glColor4f(p->m_color.r, p->m_color.g, p->m_color.b, m_notealpha);
			float const texOffset = 2.0 * time; // Offset for animating the wave texture
			Player::pitch_t const& pitch = p->m_pitch;
			size_t const beginIdx = std::max(0.0, time - 0.5 / pixUnit) / Engine::TIMESTEP; // At which pitch idx to start displaying the wave
			size_t const endIdx = pitch.size();
			double oldval = getNaN();
			size_t idx = beginIdx;
			// Go back until silence (NaN freq) to allow proper wave phase to be calculated
			if (beginIdx < endIdx) while (idx > 0 && pitch[idx].first == pitch[idx].first) --idx;
			// Start processing
			float tex = texOffset;
			double t = idx * Engine::TIMESTEP;

			for (; idx < endIdx; ++idx, t += Engine::TIMESTEP) {
				double const freq = pitch[idx].first;
				// If freq is NaN, we have nothing to process
				if (freq != freq) { tex = texOffset; oldval = getNaN(); continue; }
				tex = tex + freq * 0.001; // Wave phase (texture coordinate)
				if (idx < beginIdx) continue; // Skip graphics rendering if out of screen
				bool prev = idx > beginIdx && pitch[idx - 1].first > 0.0;
				bool next = idx < endIdx - 1 && pitch[idx + 1].first > 0.0;
				// If neither previous or next frames have proper frequency, ignore this one too
				if (!prev && !next) { oldval = getNaN(); continue; }
				double x = -0.2 + (t - time) * pixUnit;
				// Find the currently playing note or the next playing note (or the last note?)
				std::size_t i = 0;
				while (i < song.notes.size() && t > song.notes[i].end) ++i;
				Note const& n = song.notes[i];
				double diff = n.diff(song.scale.getNote(freq));
				double val = n.note + diff;
				double y = baseY + val * noteUnit;
				double thickness = clamp(1.0 + pitch[idx].second / 60.0) + 0.5;
				thickness *= 1.0 + 0.2 * std::sin(tex - 2.0 * texOffset); // Further animation :)
				thickness *= -noteUnit;
				// If pitch change is too fast, terminate and begin a new one
				if (prev && std::abs(oldval - val) > 1.0) {
					glEnd();
					prev = false;
				}
				if (!prev) glBegin(GL_TRIANGLE_STRIP);
				if (prev && next) {
					glTexCoord2f(tex, 0.0f); glVertex2f(x, y - thickness);
					glTexCoord2f(tex, 1.0f); glVertex2f(x, y + thickness);
				} else {
					glTexCoord2f(tex, 0.0f); glVertex2f(x, y);
				}
				if (!next) glEnd();
				oldval = val;
			}
		}
		glColor3f(1.0, 1.0, 1.0);
	}

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
			if (i == 0) m_lyrics[0].draw(*theme->lyrics_now, time, pos);
			else if (i == 1) m_lyrics[1].draw(*theme->lyrics_next, time, pos);
		}
	}

	// Compute and draw the timer and the progressbar
	{
		m_progress->draw(songPercent);
		std::string status = (boost::format("%02u:%02u") % (unsigned(time) / 60) % (unsigned(time) % 60)).str();
		if (!m_score_window.get()) {
			SongStatus s = songStatus();
			if (s == INSTRUMENTAL_BREAK) status += "   ENTER to skip instrumental break";
			if (s == FINISHED) status += "   Remember to wait for grading!";
		}
		theme->timer->draw(status);
	}

	if (m_score_window.get()) {
		if (m_quitTimer.get() == 0.0 && !m_audio.isPaused()) { sm->activateScreen("Songs"); return; }
		m_score_window->draw();
	}
	else if (!m_audio.isPlaying() || (m_songit == song.notes.end() && m_audio.getLength() - time < 3.0)) {
		m_quitTimer.setValue(QUIT_TIMEOUT);
		m_score_window.reset(new ScoreWindow(sm, *m_engine));
	}
		
	if (m_audio.isPaused()) {
		m_pause_icon->dimensions.middle().center().fixedWidth(.25);
		m_pause_icon->draw();
	}
}

ScoreWindow::ScoreWindow(ScreenManager const* sm, Engine& e):
  m_bg(sm->getThemePathFile("score_window.svg")),
  m_scoreBar(sm->getThemePathFile("score_bar_bg.svg"), sm->getThemePathFile("score_bar_fg.svg"), ProgressBar::VERTICAL, 0.0, 0.0, false),
  m_score_text(sm->getThemePathFile("score_txt.svg")),
  m_score_rank(sm->getThemePathFile("score_rank.svg")),
  m_players(e.getPlayers())
{
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


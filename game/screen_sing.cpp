#include "screen_sing.hh"
#include "xtime.hh"

#include <boost/format.hpp>
#include "sdl_helper.hh"
#include "songs.hh"
#include <iostream>
#include <iomanip>

const double Engine::TIMESTEP = 0.01; // FIXME: Move this elsewhere

void CScreenSing::enter() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	Song& song = sm->getSongs()->current();
	theme.reset(new CThemeSing());
	if (!song.background.empty()) { try { m_background.reset(new Surface(song.path + song.background)); } catch (std::exception& e) { std::cerr << e.what() << std::endl; } }
#define TRYLOAD(field, class) if (!song.field.empty()) { try { m_##field.reset(new class(song.path + song.field)); } catch (std::exception& e) { std::cerr << e.what() << std::endl; } }
	TRYLOAD(video, Video)
#undef TRYLOAD
	if (!m_wave) m_wave.reset(new Texture(sm->getThemePathFile("wave.png")));
	if (!m_notelines) m_notelines.reset(new Texture(sm->getThemePathFile("notelines.svg")));
	if (!m_notebar) m_notebar.reset(new Texture(sm->getThemePathFile("notebar.svg")));
	if (!m_notebar_hl) m_notebar_hl.reset(new Texture(sm->getThemePathFile("notebar.png")));
	if (!m_notebarfs) m_notebarfs.reset(new Texture(sm->getThemePathFile("notebarfs.svg")));
	if (!m_notebarfs_hl) m_notebarfs_hl.reset(new Texture(sm->getThemePathFile("notebarfs-hl.svg")));
	if (!m_notebargold) m_notebargold.reset(new Texture(sm->getThemePathFile("notebargold.svg")));
	if (!m_notebargold_hl) m_notebargold_hl.reset(new Texture(sm->getThemePathFile("notebargold.png")));
	std::string file = song.path + song.mp3;
	std::cout << "Now playing: " << file << std::endl;
	CAudio& audio = *sm->getAudio();
	audio.playMusic(file.c_str());
	m_engine.reset(new Engine(audio, m_analyzers.begin(), m_analyzers.end()));
	lyrics.reset(new Lyrics(song.notes));
	playOffset = 0.0;
	m_songit = song.notes.begin();
	audio.wait(); // Until playback starts
	m_notealpha = 0.0f;
	min = song.noteMin - 7.0;
	max = song.noteMax + 7.0;
}

void CScreenSing::exit() {
	CScreenManager::getSingletonPtr()->getAudio()->stopMusic();
	m_video.reset();
	m_background.reset();
	m_sentence.clear();
	lyrics.reset();
	theme.reset();
	m_notelines.reset();
	m_wave.reset();
	m_engine.reset();
}

void CScreenSing::manageEvent(SDL_Event event) {
	if (event.type == SDL_KEYDOWN) {
		CScreenManager* sm = CScreenManager::getSingletonPtr();
		CAudio& audio = *sm->getAudio();
		int key = event.key.keysym.sym;
		if (key == SDLK_ESCAPE || key == SDLK_q || (key == SDLK_RETURN && m_sentence.empty())) sm->activateScreen(m_sentence.empty() ? /*FIXME:"Score"*/ "Songs" : "Songs");
		else if (key == SDLK_SPACE || key == SDLK_PAUSE) sm->getAudio()->togglePause();
		else if (key == SDLK_PLUS) playOffset += 0.02;
		else if (key == SDLK_MINUS) playOffset -= 0.02;
		else if (key == SDLK_HOME) audio.seek(-audio.getPosition());
		else if (key == SDLK_RETURN && !m_sentence.empty()) {
			double diff = m_sentence[0].begin - 3.0 - audio.getPosition();
			if (diff > 0.0) audio.seek(diff);
		}
		else if (key == SDLK_LEFT) audio.seek(-5.0);
		else if (key == SDLK_RIGHT) audio.seek(5.0);
		else if (key == SDLK_UP) audio.seek(30.0);
		else if (key == SDLK_DOWN) audio.seek(-30.0);
		else if (key == SDLK_r && event.key.keysym.mod & KMOD_CTRL) {
			double pos = audio.getPosition();
			if (!m_sentence.empty()) pos = std::min(pos, m_sentence[0].begin - 0.4);
			sm->getSongs()->current().reload();
			exit(); enter();
			boost::thread::sleep(now() + 0.3); // HACK: Wait until the song is loaded
			audio.seek(pos);
		}
		m_songit = sm->getSongs()->current().notes.begin(); // Must be done after seeking backwards, doesn't hurt to do it always
	}
}

namespace {
	void drawRectangleOpenGL(double x, double y, double w, double h,
	  float _r, float _g, float _b, float _a)
	{
		glColor4f(_r, _g, _b, _a);
		glBegin(GL_QUADS);
		glVertex2f(x  ,y  ); glVertex2f(x  ,y+h);
		glVertex2f(x+w,y+h); glVertex2f(x+w,y  );
		glEnd();
		glColor4f(1.0, 1.0, 1.0, 1.0);
	}

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
}

void CScreenSing::draw() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	if (!sm->getAudio()->isPlaying()) {
		sm->activateScreen("Songs"/* FIXME:"Score"*/);
		return;
	}
	Song& song = sm->getSongs()->current();
	double oldfontsize;
	// Get the time in the song
	double time = sm->getAudio()->getPosition() + 0.02; // Compensate avg. lag to display
	time = std::max(0.0, time + playOffset);
	/* Auto-analysis code
	while (m_songit != song.notes.end() && time > 0.3 * m_songit->begin + 0.7 * m_songit->end) {
		if (m_songit->type == Note::SLEEP) {
			std::cout << "# ---" << std::endl;
		} else {
			Analyzer::tones_t const& tones = m_analyzer.getTones();
			std::cout << "# " << m_songit->syllable << ":" << std::fixed << std::setprecision(0);
			for (Analyzer::tones_t::const_iterator it = tones.begin(); it != tones.end(); ++it) {
				if (it->freq < 70.0) continue;
				if (it->freq > 600.0) break;
				std::cout << " " << song.scale.getNoteId(it->freq) << "(" << it->db << ")";
			}
			std::cout << std::endl;
		}
		++m_songit;
	}
	*/

	double songPercent = time / sm->getAudio()->getLength();
	// Rendering starts
	if (m_background) m_background->draw();
	if (m_video) m_video->render(time - song.videoGap);
	theme->bg->draw();
	theme->p1box->draw();
	theme->p2box->draw();
	// Compute and draw the timer and the progressbar
	theme->timer->draw((boost::format("%02u:%02u") % (unsigned(time) / 60) % (unsigned(time) % 60)).str());
	/*
	theme->progressfg.width = theme->progressfg.final_width * songPercent;
	drawRectangleOpenGL(
		theme->progressfg.x,theme->progressfg.y,
		theme->progressfg.width/800.,theme->progressfg.height/600.,
		theme->progressfg.fill_col.r, theme->progressfg.fill_col.g, theme->progressfg.fill_col.b, theme->progressfg.fill_col.a);
	*/
	const double baseLine = -0.2;
	const double pixUnit = 0.2;
	// Update m_songit (which note to start the rendering from)
	while (m_songit != song.notes.end() && (m_songit->type == Note::SLEEP || m_songit->end < time - (baseLine + 0.5) / pixUnit)) ++m_songit;
	{
		// Automatically zooming notelines
		int low = song.noteMax;
		int high = song.noteMin;
		for (Song::notes_t::const_iterator it = m_songit; it != song.notes.end() && it->begin < time + 5.0; ++it) {
			if (it->type == Note::SLEEP) continue;
			if (it->note < low) low = it->note;
			if (it->note > high) high = it->note;
		}
		if (low <= high) {
			double err = min + 7.0 - low;
			if (err > 0.0) min -= err * 0.007 + 0.02;
			else if (err < -3.0) min += 0.04;
			err = max - 7.0 - high;
			if (err < 0.0) max += -err * 0.007 + 0.02;
			else if (err > 3.0) max -= 0.04;
		}
	}
	m_sentence = lyrics->getCurrentSentence();
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
		// Draw notes
		{
			for (Song::notes_t::const_iterator it = m_songit; it != song.notes.end() && it->begin < time - (baseLine - 0.5) / pixUnit; ++it) {
				if (it->type == Note::SLEEP) continue;
				Texture* t1;
				Texture* t2;
				switch (it->type) {
				  case Note::FREESTYLE: t1 = m_notebarfs.get(); t2 = m_notebarfs_hl.get(); break;
				  case Note::GOLDEN: t1 = m_notebargold.get(); t2 = m_notebargold_hl.get(); break;
				  default: t1 = m_notebar.get(); t2 = m_notebar_hl.get(); break;
				}
				double y_pixel,x_pixel,h_pixel,w_pixel;
				h_pixel = -noteUnit * 2.0; // Times two for borders
				y_pixel = baseY + it->note * noteUnit - 0.5 * h_pixel;
				x_pixel = baseX + it->begin * pixUnit - 0.5 * h_pixel; // h_pixel for borders
				w_pixel = (it->end - it->begin) * pixUnit + h_pixel; // h_pixel for borders
				drawNotebar(*t1, x_pixel, y_pixel, w_pixel, h_pixel);
				double alpha = it->power;
				if (alpha > 0.0) {
					glColor4f(1.0f, 1.0f, 1.0f, alpha * m_notealpha);
					drawNotebar(*t2, x_pixel, y_pixel, w_pixel, h_pixel);
					glColor4f(1.0f, 1.0f, 1.0f, m_notealpha);
				}
			}
		}
		// Pitch graph
		UseTexture tblock(*m_wave);
		for (std::list<Player>::const_iterator p = players.begin(); p != players.end(); ++p) {
			glColor4f(p->m_color.r, p->m_color.g, p->m_color.b, m_notealpha);
			float const texOffset = 2.0 * time; // Offset for animating the wave texture
			Player::pitch_t const& pitch = p->m_pitch;
			size_t const beginIdx = std::max(0.0, time - 0.5 / pixUnit) / Engine::TIMESTEP; // At which pitch idx to start displaying the wave
			size_t const endIdx = pitch.size();
			double oldval = std::numeric_limits<double>::quiet_NaN();
			size_t idx = beginIdx;
			// Go back until silence (NaN freq) to allow proper wave phase to be calculated
			while (idx > 0 && pitch[idx].first == pitch[idx].first) --idx;
			// Start processing
			float tex = texOffset;
			double t = idx * Engine::TIMESTEP;
			for (; idx < endIdx; ++idx, t += Engine::TIMESTEP) {
				double const freq = pitch[idx].first;
				// If freq is NaN, we have nothing to process
				if (freq != freq) { tex = texOffset; oldval = std::numeric_limits<double>::quiet_NaN(); continue; }
				tex = tex + freq * 0.001; // Wave phase (texture coordinate)
				if (idx < beginIdx) continue; // Skip graphics rendering if out of screen
				bool prev = idx > beginIdx && pitch[idx - 1].first > 0.0;
				bool next = idx < endIdx - 1 && pitch[idx + 1].first > 0.0;
				// If neither previous or next frames have proper frequency, ignore this one too
				if (!prev && !next) { oldval = std::numeric_limits<double>::quiet_NaN(); continue; }
				double x = -0.2 + (t - time) * pixUnit;
				// Find the currently playing note or the next playing note (or the last note?)
				std::size_t i = 0;
				while (i < song.notes.size() && t > song.notes[i].end) ++i;
				Note const& n = song.notes[i];
				double diff = n.diff(song.scale.getNote(freq));
				double val = n.note + diff;
				double y = baseY + val * noteUnit;
				double thickness = (std::max(0.0, std::min(1.0, 1.0 + pitch[idx].second / 60.0))) + 0.5;
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
	// Compute and draw lyrics
	{
		lyrics->updateSentences(time);
		std::vector<std::string> sentenceNextSentence = lyrics->getSentenceNext();
		std::vector<std::string> sentencePast = lyrics->getSentencePast();
		std::vector<std::string> sentenceNow = lyrics->getSentenceNow();
		std::vector<std::string> sentenceFuture = lyrics->getSentenceFuture();

		std::vector<TZoomText> sentenceNextSentenceZ;
		std::vector<TZoomText> sentenceWholeZ;

		for (unsigned int i = 0 ; i < sentenceNextSentence.size(); i++) {
			TZoomText tmp;
			tmp.factor = 1.0;
			tmp.string = sentenceNextSentence[i];
			sentenceNextSentenceZ.push_back(tmp);
		}
		for (unsigned int i = 0 ; i < sentencePast.size(); i++) {
			TZoomText tmp;
			tmp.factor = 1.0;
			tmp.string = sentencePast[i];
			sentenceWholeZ.push_back(tmp);
		}
		for (unsigned int i = 0 ; i < sentenceNow.size(); i++) {
			TZoomText tmp;
			tmp.factor = 1.5;
			tmp.string = sentenceNow[i];
			sentenceWholeZ.push_back(tmp);
		}
		for (unsigned int i = 0 ; i < sentenceFuture.size(); i++) {
			TZoomText tmp;
			tmp.factor = 1.0;
			tmp.string = sentenceFuture[i];
			sentenceWholeZ.push_back(tmp);
		}
		theme->lyrics_now->draw(sentenceWholeZ);
		theme->lyrics_next->draw(sentenceNextSentenceZ);
	}
	//draw score
	theme->score1->draw((boost::format("%04d") % players.begin()->getScore()).str());
	theme->score2->draw((boost::format("%04d") % players.rbegin()->getScore()).str());
	// Surface(theme->theme->getCurrent()).draw(); // Render progress bar, score calculator, time, etc. - OPTIMIZE: This part is very slow and needs to be optimized
}

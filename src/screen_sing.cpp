#include "screen_sing.hh"
#include "xtime.hh"

#include <boost/format.hpp>
#include "sdl_helper.hh"
#include "songs.hh"
#include "pitch_graph.hh"
#include <iostream>
#include <iomanip>

const double Engine::TIMESTEP = 0.01; // FIXME: Move this elsewhere

void CScreenSing::enter() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	Song& song = sm->getSongs()->current();
	theme.reset(new CThemeSing());
	if (!song.background.empty()) { try { m_background.reset(new Surface(song.path + song.background,Surface::MAGICK)); } catch (std::exception& e) { std::cerr << e.what() << std::endl; } }
#define TRYLOAD(field, class) if (!song.field.empty()) { try { m_##field.reset(new class(song.path + song.field)); } catch (std::exception& e) { std::cerr << e.what() << std::endl; } }
	TRYLOAD(video, Video)
#undef TRYLOAD
	if (!m_notelines) m_notelines.reset(new Surface(sm->getThemePathFile("notelines.svg"),Surface::SVG));
	if (!m_wave) m_wave.reset(new Surface(sm->getThemePathFile("wave.png"),Surface::MAGICK));
	std::string file = song.path + song.mp3;
	std::cout << "Now playing: " << file << std::endl;
	CAudio& audio = *sm->getAudio();
	audio.playMusic(file.c_str());
	m_engine.reset(new Engine(audio, m_analyzers.begin(), m_analyzers.end()));
	lyrics.reset(new Lyrics(song.notes));
	playOffset = 0.0;
	song.reset();
	m_songit = song.notes.begin();
	audio.wait(); // Until playback starts
	m_notealpha = 0.0f;
}

void CScreenSing::exit() {
	CScreenManager::getSingletonPtr()->getAudio()->stopMusic();
	m_video.reset();
	m_background.reset();
	m_sentence.clear();
	lyrics.reset();
	theme.reset();
	pitchGraph.clear();
	m_notelines.reset();
	m_wave.reset();
	m_engine.reset();
}

void CScreenSing::manageEvent(SDL_Event event) {
	if (event.type == SDL_KEYDOWN) {
		CScreenManager* sm = CScreenManager::getSingletonPtr();
		CAudio& audio = *sm->getAudio();
		int key = event.key.keysym.sym;
		if (key == SDLK_ESCAPE || key == SDLK_q || (key == SDLK_RETURN && m_sentence.empty())) sm->activateScreen(m_sentence.empty() ? "Score" : "Songs");
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

void CScreenSing::draw() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	if (!sm->getAudio()->isPlaying()) {
		sm->activateScreen("Score");
		return;
	}
	Song& song = sm->getSongs()->current();
	double oldfontsize;
	theme->theme->clear();
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
	// Here we compute all about the lyrics
	lyrics->updateSentences(time);
	std::string sentenceNextSentence = lyrics->getSentenceNext();
	std::string sentencePast = lyrics->getSentencePast();
	std::string sentenceNow = lyrics->getSentenceNow();
	std::string sentenceFuture = lyrics->getSentenceFuture();
	std::string sentenceWhole = lyrics->getSentenceWhole();
	// Rendering starts
	if (m_background) m_background->draw();
	if (m_video) m_video->render(time - song.videoGap);
	theme->bg->draw();
	theme->p1box->draw();
	// Compute and draw the timer and the progressbar
	theme->timertxt.text = (boost::format("%02u:%02u") % (unsigned(time) / 60) % (unsigned(time) % 60)).str();
	theme->theme->PrintText(&theme->timertxt);
	theme->progressfg.width = theme->progressfg.final_width * songPercent;
	drawRectangleOpenGL(
		theme->progressfg.x,theme->progressfg.y,
		theme->progressfg.width,theme->progressfg.height,
		theme->progressfg.fill_col.r, theme->progressfg.fill_col.g, theme->progressfg.fill_col.b, theme->progressfg.fill_col.a);
	//draw score
	theme->p1score.text = (boost::format("%04d") % song.getScore()).str();
	theme->theme->PrintText(&theme->p1score);
	/*
	// draw the sang note TODO: themed sang note
	{
		TThemeTxt tmptxt = theme->timertxt; // use timertxt as template
		tmptxt.text = song.scale.getNoteStr(freq);
		tmptxt.x = 600;
		tmptxt.fontsize = 25;
		theme->theme->PrintText(&tmptxt);
	}
	*/
	double pixUnit = 0.3;
	m_sentence = lyrics->getCurrentSentence();
	double min = song.noteMin - 7.0;
	double max = song.noteMax + 7.0;
	double noteUnit = -0.5 / std::max(32.0, max - min);
	double baseY = -0.5 * (min + max) * noteUnit;
	const double baseLine = -0.2;
	double baseX = baseLine - time * pixUnit;
	// Update m_songit (which note to start the rendering from)
	while (m_songit != song.notes.end() && (m_songit->type == Note::SLEEP || m_songit->end < time - (baseLine + 0.5) / pixUnit)) ++m_songit;
	// Draw note lines
	if (m_songit == song.notes.end() || m_songit->begin > time + 3.0) m_notealpha -= 0.02f;
	else if (m_notealpha < 1.0f) m_notealpha += 0.02f;
	if (m_notealpha <= 0.0f) {
		m_notealpha = 0.0f;
	} else {
		glColor4f(1.0, 1.0, 1.0, m_notealpha);
		m_notelines->dimensions.stretch(1.0, (max - min - 13) * noteUnit);
		m_notelines->tex.y2 = (-max + 6.0) / 12.0f;
		m_notelines->tex.y1 = (-min - 7.0) / 12.0f;
		m_notelines->draw();
		// Draw notes
		for (Song::notes_t::const_iterator it = m_songit; it != song.notes.end() && it->begin < time - (baseLine - 0.5) / pixUnit; ++it) {
			if (it->type == Note::SLEEP) continue;
			float r,g,b,a;
			switch (it->type) {
			  case Note::FREESTYLE: r = 0.6; g = 1.0; b = 0.6; a = 1.0; break;
			  case Note::GOLDEN: r = 1.0; g = 0.8; b = 0.0; a = 1.0; break;
			  default: r = 0.8; g = 0.8; b = 1.0; a = 1.0;
			}
			double y_pixel,x_pixel,h_pixel,w_pixel;
			h_pixel = -noteUnit;
			y_pixel = baseY + it->note * noteUnit - 0.5 * h_pixel;
			x_pixel = baseX + it->begin * pixUnit;
			w_pixel = (it->end - it->begin) * pixUnit;
			drawRectangleOpenGL(x_pixel,y_pixel,w_pixel,h_pixel,r, g, b, a);
		}
		// Pitch graph
		Surface::Use texture(*m_wave);
		std::list<Player> players = m_engine->getPlayers();
		for (std::list<Player>::const_iterator p = players.begin(); p != players.end(); ++p) {
			glColor4f(p->m_color.r, p->m_color.g, p->m_color.b, m_notealpha);
			float tex = 0.0;
			double t = std::max(0.0, time - 0.5 / pixUnit);
			Player::pitch_t const& pitch = p->m_pitch;
			size_t beginIdx = t / Engine::TIMESTEP;
			size_t endIdx = pitch.size();
			for (size_t idx = beginIdx; idx < endIdx; ++idx, t += Engine::TIMESTEP) {
				if (pitch[idx] != pitch[idx]) continue;
				bool prev = idx > beginIdx && pitch[idx - 1].first > 0.0;
				bool next = idx < endIdx - 1 && pitch[idx + 1].first > 0.0;
				if (!prev && !next) break;
				double x = -0.2 + (t - time) * pixUnit;
				// Find the currently playing note or the next playing note (or the last note?)
				std::size_t i = 0;
				while (i < song.notes.size() && t > song.notes[i].end) ++i;
				Note const& n = song.notes[i];
				double freq = pitch[idx].first;
				double y = baseY + (n.note + n.diff(song.scale.getNote(freq))) * noteUnit;
				double thickness = (std::max(0.0, std::min(1.0, 1.0 + pitch[idx].second / 60.0))) + 0.5;
				thickness *= -noteUnit;
				tex += freq * 0.001;
				// If pitch change is too fast, terminate and begin a new one
				if (prev && std::abs(pitch[idx - 1].first / pitch[idx].first - 1.0) > 0.02) {
					glEnd();
					prev = false;
				}
				if (!prev) { tex = 0.0; glBegin(GL_TRIANGLE_STRIP); }
				if (prev && next) {
					glTexCoord2f(tex, 0.0f); glVertex2f(x, y - thickness);
					glTexCoord2f(tex, 1.0f); glVertex2f(x, y + thickness);
				} else {
					glTexCoord2f(tex, 0.0f); glVertex2f(x, y);
				}
				if (!next) glEnd();
			}
		}
	}
	glColor3f(1.0, 1.0, 1.0);

	// Render the lyrics - OPTIMIZE: This part is very slow and needs to be optimized
	TThemeTxt tmptxt = theme->lyricspast;
	tmptxt.text = sentenceWhole;
	{
		cairo_text_extents_t extents = theme->theme->GetTextExtents(tmptxt);
		theme->lyricspast.x = (theme->lyricspast.svg_width - extents.width)/2;
	}
	oldfontsize = theme->lyricspast.fontsize;
	while (theme->lyricspast.x < 0) {
		theme->lyricspast.fontsize -= 2;
		theme->lyricshighlight.fontsize -= 2;
		theme->lyricsfuture.fontsize -= 2;
		tmptxt = theme->lyricspast;
		tmptxt.text = sentenceWhole;
		cairo_text_extents_t extents = theme->theme->GetTextExtents(tmptxt);
		theme->lyricspast.x = (theme->lyricspast.svg_width - extents.width)/2;
	}
	theme->lyricspast.extents.x_advance = 0;
	theme->lyricshighlight.extents.x_advance= 0;
	
	if (!sentencePast.empty()) {
		theme->lyricspast.text = sentencePast;
		theme->theme->PrintText(&theme->lyricspast);
	}
	if (!sentenceNow.empty()) {
		Note* n = lyrics->getCurrentNote();
		if (!n) throw std::logic_error("sentenceNow is not empty but current note is NULL");
		double phase = (time - n->begin) / (n->end - n->begin);
		double factor = std::min(1.2, std::max(1.0, 1.2 - 0.2 * phase));
		theme->lyricshighlight.x = theme->lyricspast.x + theme->lyricspast.extents.x_advance;
		theme->lyricshighlight.text = sentenceNow;
		theme->lyricshighlight.scale = factor;
		theme->theme->PrintText(&theme->lyricshighlight);
	}
	if (!sentenceFuture.empty()) {
		theme->lyricsfuture.text = sentenceFuture;
		theme->lyricsfuture.x = theme->lyricspast.x + theme->lyricspast.extents.x_advance + theme->lyricshighlight.extents.x_advance;
		theme->theme->PrintText(&theme->lyricsfuture);
	}

	if (!sentenceNextSentence.empty()) {
		theme->lyricsnextsentence.text = sentenceNextSentence;
		theme->lyricsnextsentence.extents = theme->theme->GetTextExtents(theme->lyricsnextsentence);
		theme->lyricsnextsentence.x = (theme->lyricsnextsentence.svg_width - theme->lyricsnextsentence.extents.width)/2;
		while (theme->lyricsnextsentence.x < 0) {
			theme->lyricsnextsentence.fontsize -= 2;
			theme->lyricsnextsentence.extents = theme->theme->GetTextExtents(theme->lyricsnextsentence);
			theme->lyricsnextsentence.x = (theme->lyricsnextsentence.svg_width - theme->lyricsnextsentence.extents.width)/2;
		}
		theme->theme->PrintText(&theme->lyricsnextsentence);
	}

	theme->lyricspast.fontsize = oldfontsize;
	theme->lyricshighlight.fontsize = oldfontsize;
	theme->lyricsfuture.fontsize = oldfontsize;
	theme->lyricsnextsentence.fontsize = oldfontsize;

	Surface(theme->theme->getCurrent()).draw(); // Render progress bar, score calculator, time, etc. - OPTIMIZE: This part is very slow and needs to be optimized
}

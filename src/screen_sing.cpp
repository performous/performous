#include <screen_sing.h>
#include <xtime.h>

#include <boost/format.hpp>
#include <sdl_helper.h>
#include <songs.h>
#include <pitch_graph.h>
#include <iostream>
#include <iomanip>

void CScreenSing::enter() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	Song& song = sm->getSongs()->current();
	theme.reset(new CThemeSing());
	if (!song.background.empty()) { try { m_background.reset(new Surface(song.path + song.background,Surface::MAGICK)); } catch (std::exception& e) { std::cerr << e.what() << std::endl; } }
#define TRYLOAD(field, class) if (!song.field.empty()) { try { m_##field.reset(new class(song.path + song.field)); } catch (std::exception& e) { std::cerr << e.what() << std::endl; } }
	TRYLOAD(video, Video)
#undef TRYLOAD
	std::string file = song.path + song.mp3;
	std::cout << "Now playing: " << file << std::endl;
	CAudio& audio = *sm->getAudio();
	audio.playMusic(file.c_str());
	lyrics.reset(new Lyrics(song.notes));
	playOffset = 0.0;
	song.reset();
	m_songit = song.notes.begin();
	audio.wait(); // Until playback starts
}

void CScreenSing::exit() {
	CScreenManager::getSingletonPtr()->getAudio()->stopMusic();
	m_video.reset();
	m_background.reset();
	m_sentence.clear();
	lyrics.reset();
	theme.reset();
	pitchGraph.clear();
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
	}
}

void drawRectangleOpenGL( double _x, double _y, double _w, double _h, float _r, float _g, float _b, float _a) {
	static float r=-1,g=-1,b=-1,a=-1;
	double m_width = 800.;
	double m_height = 600.;

	double x = _x/(m_width*1.0)-0.5;
	double y = (_y/(m_height*1.0)-0.5)*((m_height*1.0)/(m_width*1.0));
	double w = _w/(m_width*1.0);
	double h = _h/(m_width*1.0);
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
	const_cast<Analyzer&>(m_analyzer).process(); // FIXME: do in game engine thread
	Tone const* tone = m_analyzer.findTone();
	double freq = (tone ? tone->freq : 0.0);
	float resFactorX = 800.0 / 800.0; // FIXME!!
	float resFactorY = 600.0 / 600.0; // FIXME!!
	float resFactorAvg = (resFactorX + resFactorY) / 2.0;
	double oldfontsize;
	theme->theme->clear();
	// Get the time in the song
	double time = sm->getAudio()->getPosition();
	time = std::max(0.0, time + playOffset);
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
	double songPercent = time / sm->getAudio()->getLength();
	// Update scoring
	song.update(time, freq);
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
	// draw the sang note TODO: themed sang note
	{
		TThemeTxt tmptxt = theme->timertxt; // use timertxt as template
		tmptxt.text = song.scale.getNoteStr(freq);
		tmptxt.x = 600;
		tmptxt.fontsize = 25;
		theme->theme->PrintText(&tmptxt);
	}
	// compute and draw the text
	double sentenceBegin = m_sentence.empty() ? 0.0 : m_sentence[0].begin;
	double sentenceDuration = 0.0;
	double pixUnit = 0.0;
	m_sentence = lyrics->getCurrentSentence();
	if (!m_sentence.empty()) {
		if (sentenceBegin != m_sentence[0].begin) {
			pitchGraph.clear();
			sentenceBegin = m_sentence[0].begin;
		}
		sentenceDuration = m_sentence.back().end - sentenceBegin;
		pixUnit = (600.0 * resFactorX) / (sentenceDuration * 1.0);
	} else {
		pitchGraph.clear();
	}
	// Compute and draw the "to start" cursor
	if (time < sentenceBegin) {
		double wait = sentenceBegin - time;
		double value = 4.0 * wait / sentenceDuration;
		if (value > 1.0) value = wait > 1.0 ? 0.0 : 1.0;
		theme->tostartfg.height = theme->tostartfg.final_height * value;
		drawRectangleOpenGL(
			theme->tostartfg.x,theme->tostartfg.y,
			theme->tostartfg.width,theme->tostartfg.height,
			theme->tostartfg.fill_col.r, theme->tostartfg.fill_col.g, theme->tostartfg.fill_col.b, theme->tostartfg.fill_col.a);
	}
	int min = song.noteMin - 7;
	int max = song.noteMax + 7;
	double m_width = 800.0, m_height = 600.0; // FIXME!!!
	double noteUnit = -0.5 * m_height / std::max(32, max - min);
	double baseY = 0.5 * m_height - 0.5 * (min + max) * noteUnit;
	// Draw note lines
	if (!m_sentence.empty()) {
		float r,g,b,a;
		double y_pixel,x_pixel,h_pixel,w_pixel;
		for (int n = song.noteMin; n <= song.noteMax; ++n) {
			if (n % 12) {
				r = 0.5; g = 0.5; b = 0.5; a = 0.5;
			} else {
				r = 0.8; g = 0.3; b = 0.8; a = 0.5;
			}
			y_pixel = baseY + n * noteUnit;
			x_pixel = 0;
			w_pixel = m_width;
			h_pixel = (song.scale.isSharp(n) ? 0.5 : 1.5) * resFactorAvg;
			drawRectangleOpenGL(x_pixel,y_pixel,w_pixel,h_pixel,r, g, b, a);
		}
	}
	int state = 0;
	double baseX = 100.0 * resFactorX - sentenceBegin * pixUnit;
	for (unsigned int i = 0; i < m_sentence.size(); ++i) {
		float r,g,b,a;
		double y_pixel,x_pixel,h_pixel,w_pixel;
		h_pixel = -noteUnit;

		if (m_sentence[i].begin > time) state = 3;
		if (state == 0 && m_sentence[i].end > time) state = 1;
		y_pixel = baseY + m_sentence[i].note * noteUnit - 0.5 * h_pixel;
		double begin = (state == 2 ? time : m_sentence[i].begin);
		double end = (state == 1 ? time : m_sentence[i].end);
		x_pixel = baseX + begin * pixUnit;
		w_pixel = (end - begin) * pixUnit;
		if (state < 2) {
			r = 0.7; g = 0.7; b = 0.7; a = 1.0;
		} else {
			switch (m_sentence[i].type) {
			  case Note::FREESTYLE:
				r = 0.6; g = 1.0; b = 0.6; a = 1.0;
				break;
			  case Note::GOLDEN:
				r = 1.0; g = 0.8; b = 0.0; a = 1.0;
				break;
			  default:
				r = 0.8; g = 0.8; b = 1.0; a = 1.0;
			}
		}
		drawRectangleOpenGL(x_pixel,y_pixel,w_pixel,h_pixel,r, g, b, a);

		if (state == 1) { --i; state = 2; }
	}
	glColor3f(1.0, 1.0, 1.0);

	if (!m_sentence.empty()) {
		double graphTime = (baseX + time * pixUnit) / m_width;
		if (freq == 0.0) {
			pitchGraph.renderPitch(0.0, graphTime, 0.0);
		} else {
			unsigned int i = 0;
			// Find the currently playing note or the next playing note (or the last note?)
			while (i < m_sentence.size() && time > m_sentence[i].end) ++i;
			Note const& n = m_sentence[i];
			double volume = std::max(0.0, 1.0 + m_analyzer.getPeak() / 40.0);
			pitchGraph.renderPitch((baseY + (n.note + n.diff(song.scale.getNote(freq))) * noteUnit) / m_height, graphTime, volume);
		}
	}
	
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

	Surface(theme->theme->getCurrent()).draw();
	Surface(pitchGraph.getCurrent()).draw();
}

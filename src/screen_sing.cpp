#include <screen_sing.h>
#include <xtime.h>

#include <boost/format.hpp>
#include <sdl_helper.h>
#include <songs.h>
#include <pitch_graph.h>
#include <cairotosdl.h>
#include <iostream>
#include <iomanip>

CScreenSing::CScreenSing(std::string const& name, unsigned int width, unsigned int height, Analyzer const& analyzer):
  CScreen(name,width,height), m_analyzer(analyzer), pitchGraph(width, height)
{
}

void CScreenSing::enter() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	Song& song = sm->getSongs()->current();
	theme.reset(new CThemeSing(m_width, m_height));
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
	float resFactorX = m_width / 800.0;
	float resFactorY = m_height / 600.0;
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
	glClear(GL_COLOR_BUFFER_BIT);
	if (m_background) m_background->draw(0.5,0.5,0.5+m_width,0.5+m_height);
	if (m_video) m_video->render(time - song.videoGap, m_width, m_height);
	theme->bg->draw(0.5,0.5,0.5+m_width,0.5+m_height);
	// Compute and draw the timer and the progressbar
	theme->timertxt.text = (boost::format("%02u:%02u") % (unsigned(time) / 60) % (unsigned(time) % 60)).str();
	theme->theme->PrintText(&theme->timertxt);
	theme->progressfg.width = theme->progressfg.final_width * songPercent;
	theme->theme->DrawRect(theme->progressfg); 
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
		pixUnit = (m_width - 200.0 * resFactorX) / (sentenceDuration * 1.0);
	} else {
		pitchGraph.clear();
	}
	// Compute and draw the "to start" cursor
	if (time < sentenceBegin) {
		double wait = sentenceBegin - time;
		double value = 4.0 * wait / sentenceDuration;
		if (value > 1.0) value = wait > 1.0 ? 0.0 : 1.0;
		theme->tostartfg.height = theme->tostartfg.final_height * value;
		theme->theme->DrawRect(theme->tostartfg);
	}
	int min = song.noteMin - 7;
	int max = song.noteMax + 7;
	double noteUnit = -0.5 * m_height / std::max(32, max - min);
	double baseY = 0.5 * m_height - 0.5 * (min + max) * noteUnit;
	// Theme this
	TThemeRect linerect;
	linerect.svg_width = m_width;
	linerect.svg_height = m_height;
	linerect.x = 0;
	linerect.width = m_width;
	linerect.height = 0.0;
	linerect.fill_col.r = 0.0;
	linerect.fill_col.g = 0.0;
	linerect.fill_col.b = 0.0;
	linerect.fill_col.a = 0.0;
	linerect.final_height = 0;
	linerect.final_width  = 0;
	linerect.stroke_col.a = 0.7;
	// Draw note lines
	if (!m_sentence.empty()) {
		for (int n = song.noteMin; n <= song.noteMax; ++n) {
			linerect.stroke_width = (song.scale.isSharp(n) ? 0.5 : 1.5) * resFactorAvg;
			if (n % 12) {
				linerect.stroke_col.r = 0.5;
				linerect.stroke_col.g = 0.5;
				linerect.stroke_col.b = 0.5;
			} else {
				linerect.stroke_col.r = 0.8;
				linerect.stroke_col.g = 0.3;
				linerect.stroke_col.b = 0.8;
			}
			linerect.y = baseY + n * noteUnit;
			theme->theme->DrawRect(linerect);
		}
	}
	// Theme this
	TThemeRect tmprect;
	tmprect.stroke_col.r = tmprect.stroke_col.g = tmprect.stroke_col.b = 0.5;
	tmprect.stroke_col.a = 0.8;
	tmprect.stroke_width = resFactorAvg;
	tmprect.svg_width = m_width;
	tmprect.svg_height = m_height;
	tmprect.height = -noteUnit;
	tmprect.final_height = 0;
	tmprect.final_width  = 0;
	int state = 0;
	double baseX = 100.0 * resFactorX - sentenceBegin * pixUnit;
	for (unsigned int i = 0; i < m_sentence.size(); ++i) {
		if (m_sentence[i].begin > time) state = 3;
		if (state == 0 && m_sentence[i].end > time) state = 1;
		tmprect.y = baseY + m_sentence[i].note * noteUnit - 0.5 * tmprect.height;
		double begin = (state == 2 ? time : m_sentence[i].begin);
		double end = (state == 1 ? time : m_sentence[i].end);
		tmprect.x = baseX + begin * pixUnit;
		tmprect.width = (end - begin) * pixUnit;
		if (state < 2) {
			tmprect.fill_col.r = 0.7;
			tmprect.fill_col.g = 0.7;
			tmprect.fill_col.b = 0.7;
			tmprect.fill_col.a = 1.0;
		} else {
			switch (m_sentence[i].type) {
			  case Note::FREESTYLE:
				tmprect.fill_col.r = 0.6;
				tmprect.fill_col.g = 1.0;
				tmprect.fill_col.b = 0.6;
				tmprect.fill_col.a = 1.0;
				break;
			  case Note::GOLDEN:
				tmprect.fill_col.r = 1.0;
				tmprect.fill_col.g = 0.8;
				tmprect.fill_col.b = 0.0;
				tmprect.fill_col.a = 1.0;
				break;
			  default:
				tmprect.fill_col.r = 0.8;
				tmprect.fill_col.g = 0.8;
				tmprect.fill_col.b = 1.0;
				tmprect.fill_col.a = 1.0;
			}
		}
		theme->theme->DrawRect(tmprect);
		if (state == 1) { --i; state = 2; }
	}

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

	Surface(theme->theme->getCurrent()).draw(0, 0, 0);
	Surface(pitchGraph.getCurrent()).draw(0, 0, 0);
}

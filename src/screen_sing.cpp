#include "screen_sing.hh"
#include "xtime.hh"

#include <boost/format.hpp>
#include "sdl_helper.hh"
#include "songs.hh"
#include "pitch_graph.hh"
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
	if (!m_notelines) m_notelines.reset(new Surface(sm->getThemePathFile("notelines.svg"),Surface::SVG));
	std::string file = song.path + song.mp3;
	std::cout << "Now playing: " << file << std::endl;
	CAudio& audio = *sm->getAudio();
	audio.playMusic(file.c_str());
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

void drawRectangleOpenGL( double x, double y, double w, double h,
		float _r, float _g, float _b, float _a,
		double s_size=0.0, float _sr=0.0, float _sg=0.0, float _sb=0.0, float _sa=0.0) {
	glColor4f(_r, _g, _b, _a);
	glBegin(GL_QUADS);
	glVertex2f(x  ,y  ); glVertex2f(x  ,y+h);
	glVertex2f(x+w,y+h); glVertex2f(x+w,y  );
	glEnd();
	if( s_size != 0.0 ) {
		double sx = s_size;
		double sy = s_size;
	}
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
	double oldfontsize;
	theme->theme->clear();
	// Get the time in the song
	double time = sm->getAudio()->getPosition();
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
	double sentenceBegin = m_sentence.empty() ? 0.0 : m_sentence[0].begin;
	double pixUnit = 0.3;
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
		if (!m_sentence.empty()) {
			m_notelines->dimensions.stretch(1.0, (max - min - 13) * noteUnit);
			m_notelines->tex.y2 = (-max + 6.0) / 12.0f;
			m_notelines->tex.y1 = (-min - 7.0) / 12.0f;
			m_notelines->draw();
		}
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
			drawRectangleOpenGL(x_pixel,y_pixel,w_pixel,h_pixel,r, g, b, a,2.0,0.0,0.0,0.0,0.5);
		}
		glColor3f(1.0, 1.0, 1.0);
	}
/* Doesn't work correctly with scrolling notes, multiplayer, etc (old pitch graph stuff), to be removed
	if (!m_sentence.empty()) {
		double graphTime = (baseX + time * pixUnit);
		double graphTime2 = (baseX + time * pixUnit);
		if (freq == 0.0) {
			pitchGraph.renderPitch(0.0, graphTime, 0.0);
			if (song.pitchPitchGraph.size() > 1){
				double pitch=song.pitchPitchGraph[song.pitchPitchGraph.size()-1];
				double volume = std::max(0.0, 1.0 + m_analyzer.getPeak() / 40.0);
				song.updatePitchGraph(graphTime2, pitch, volume, 0);
			}
		} else {
			unsigned int i = 0;
			// Find the currently playing note or the next playing note (or the last note?)
			while (i < m_sentence.size() && time > m_sentence[i].end) ++i;
			Note const& n = m_sentence[i];
			double volume = std::max(0.0, 1.0 + m_analyzer.getPeak() / 40.0);
			//double pitch = (baseY + (n.note + n.diff(song.scale.getNote(freq))) * noteUnit);// / m_height;
			double pitch = (baseY + (n.note + n.diff(song.scale.getNote(freq))) * noteUnit);
			pitchGraph.renderPitch(0.5 + (baseY + (n.note + n.diff(song.scale.getNote(freq))) * noteUnit), 0.5 + graphTime, volume);
			//drawRectangleOpenGL(graphTime2+5, pitch-5, 20, 20, 0.2, 52.0/255.0, 181.0/255.0, 164.0/255.0, 1.0);
			song.updatePitchGraph(graphTime2, pitch, volume, 1);
		}
	}
*/

/*  Not currently functional (pitch graph code)
	unsigned int ii;
	for(ii=1; ii < song.timePitchGraph.size(); ii++)
	{
		double volume = 10+(5*song.volumePitchGraph[ii]);
		if ((song.drawPitchGraph[ii-1] || song.drawPitchGraph[ii])){
			double graphWidth = song.timePitchGraph[ii]-song.timePitchGraph[ii-1];
			drawRectangleOpenGL(song.timePitchGraph[ii]-graphWidth, song.pitchPitchGraph[ii]-(volume/2)+10, graphWidth, volume, 0.2, 52.0/255.0, 101.0/255.0, 164.0/255.0, 1.0);
//			drawRectangleOpenGL(song.timePitchGraph[ii]-graphWidth, song.pitchPitchGraph[ii]+10, graphWidth, 1, 0.2, 2.0/255.0, 254.0/255.0, 254.0/255.0, 1.0);
			if ((ii+1) < song.timePitchGraph.size()){
				if (song.pitchPitchGraph[ii] < song.pitchPitchGraph[ii+1]){
					double graphHeight = (song.pitchPitchGraph[ii+1]-song.pitchPitchGraph[ii])/50;
				} else {
					double graphHeight = (song.pitchPitchGraph[ii]-song.pitchPitchGraph[ii+1])/50;
				}
			}
		}
	}
*/

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

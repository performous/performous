#include "screen_practice.hh"

CScreenPractice::CScreenPractice(std::string const& name, boost::ptr_vector<Analyzer>& analyzers):
  CScreen(name), m_analyzers(analyzers)
{}

void CScreenPractice::enter() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	theme.reset(new CThemePractice());
	for (int i = 0; i < m_analyzers.size(); ++i) {
		ProgressBar* b;
		m_vumeters.push_back(b = new ProgressBar(sm->getThemePathFile("vumeter_bg.svg"), sm->getThemePathFile("vumeter_fg.svg"), ProgressBar::VERTICAL, 0.136, 0.023));
		b->dimensions.screenBottom().left(-0.4 + i * 0.3).fixedWidth(0.04);
	}
}

void CScreenPractice::exit()
{
	m_vumeters.clear();
	theme.reset();
}

void CScreenPractice::manageEvent(SDL_Event event)
{
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	if (event.type == SDL_KEYDOWN) {
		int key = event.key.keysym.sym;
		if (key == SDLK_ESCAPE || key == SDLK_q) sm->activateScreen("Intro");
		else if (key == SDLK_SPACE || key == SDLK_PAUSE) sm->getAudio()->togglePause();
	}
}

void CScreenPractice::draw() {
	theme->bg->draw();
	bool text = false;
	for (int i = 0; i < m_analyzers.size(); ++i) {
		Analyzer& analyzer = m_analyzers[i];
		analyzer.process();
		Tone const* tone = analyzer.findTone();
		double freq = (tone ? tone->freq : 0.0);
		MusicalScale scale;
		// getPeak returns 0.0 when clipping, negative values when not that loud.
		// Normalizing to [0,1], where 0 is -40 dB or less.
		m_vumeters[i].draw(std::min(0.0, std::max(-1.0, analyzer.getPeak() / 40.0))+1.0);
		if (freq != 0.0) {
			Analyzer::tones_t tones = analyzer.getTones();
			for (Analyzer::tones_t::const_iterator t = tones.begin(); t != tones.end(); ++t) {
				if (t->age < Tone::MINAGE) continue;
				int note = scale.getNoteId(t->freq);
				if (note < 0) continue;
				int octave = note / 12 - 1;
				double noteOffset = scale.getNoteNum(note);
				bool sharp = scale.isSharp(note);
				noteOffset += octave*7;
				noteOffset += 0.4 * scale.getNoteOffset(t->freq);
				float posXnote = -0.25 + 0.3 * i + 0.002 * t->stabledb;
				float posYnote = .075-noteOffset*0.015;

				theme->note->dimensions.left(posXnote).center(posYnote);
				theme->note->draw();
				if (sharp) {
					theme->sharp->dimensions.right(posXnote).center(posYnote);
					theme->sharp->draw();
				}
			}
			if (!text) {
				theme->note_txt->draw(scale.getNoteStr(freq));
				text = true;
			}
		}
	}
}

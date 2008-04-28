#include <screen_practice.h>

CScreenPractice::CScreenPractice(std::string const& name, Analyzer const& analyzer):
  CScreen(name), m_analyzer(analyzer)
{}

void CScreenPractice::enter() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	m_surf_note.reset(new Surface(sm->getThemePathFile("practice_note.svg"),Surface::SVG));
	m_surf_sharp.reset(new Surface(sm->getThemePathFile("practice_sharp.svg"),Surface::SVG));
	m_surf_note->dimensions.fixedHeight(0.03);
	m_surf_sharp->dimensions.fixedHeight(0.09);
	theme.reset(new CThemePractice());
}

void CScreenPractice::exit()
{
	theme.reset();
	m_surf_note.reset();
	m_surf_sharp.reset();
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
	const_cast<Analyzer&>(m_analyzer).process(); // FIXME: do in game engine thread
	Tone const* tone = m_analyzer.findTone();
	double freq = (tone ? tone->freq : 0.0);
	MusicalScale scale;
	// getPeak returns 0.0 when clipping, negative values when not that loud.
	// Normalizing to [-1.0, 0.0], where -1.0 is -40 dB or less.
	// FIXME: m_peak->draw(std::min(0.0, std::max(-1.0, m_analyzer.getPeak() / 40.0))+1.0);
	if (freq != 0.0) {
		theme->theme->clear();
		std::string text = scale.getNoteStr(freq);
		theme->notetxt.text = const_cast<char*>(text.c_str());
		theme->theme->PrintText(&theme->notetxt);
		Analyzer::tones_t tones = m_analyzer.getTones();
		for (Analyzer::tones_t::const_iterator t = tones.begin(); t != tones.end(); ++t) {
			if (t->age < Tone::MINAGE) continue;
			int note = scale.getNoteId(t->freq);
			if (note < 0) continue;
			int octave = note / 12 - 1;
			double noteOffset = scale.getNoteNum(note);
			bool sharp = scale.isSharp(note);
			noteOffset += octave*7;
			noteOffset += 0.4 * scale.getNoteOffset(t->freq);
			float posXnote = 0.7 + 0.01 * t->stabledb;
			float posYnote = .075-noteOffset*0.015;
			m_surf_note->dimensions.left(posXnote).center(posYnote);
			m_surf_note->draw();
			if (sharp) {
				m_surf_sharp->dimensions.right(posXnote).center(posYnote);
				m_surf_sharp->draw();
			}
		}
		Surface(theme->theme->getCurrent()).draw();
	}
}

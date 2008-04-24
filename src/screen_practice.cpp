#include <screen_practice.h>

CScreenPractice::CScreenPractice(std::string const& name, Analyzer const& analyzer):
  CScreen(name), m_analyzer(analyzer)
{}

void CScreenPractice::enter() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	m_surf_note.reset(new Surface(sm->getThemePathFile("practice_note.svg"),Surface::SVG));
	m_surf_sharp.reset(new Surface(sm->getThemePathFile("practice_sharp.svg"),Surface::SVG));
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

void CScreenPractice::draw()
{
	double m_width = 800.0, m_height = 600.0; // FIXME!!!
	float resFactorX = m_width/800.;
	float resFactorY = m_height/600.;

	const_cast<Analyzer&>(m_analyzer).process(); // FIXME: do in game engine thread
	Tone const* tone = m_analyzer.findTone();
	double freq = (tone ? tone->freq : 0.0);
	MusicalScale scale;

	theme->theme->clear();
	theme->bg->draw();

	// getPeak returns 0.0 when clipping, negative values when not that loud.
	// Normalizing to [-1.0, 0.0], where -1.0 is -40 dB or less.
	double peak = std::min(0.0, std::max(-1.0, m_analyzer.getPeak() / 40.0))+1.0;

	theme->peak.width = theme->peak.final_width* peak;
	theme->theme->DrawRect(theme->peak); 

	if (freq != 0.0) {
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
			double noteOffsetX = -600.0 - 10.0 * t->stabledb;
			int posXnote = (m_width-noteOffsetX*resFactorX) / 2.0;
			int posYnote = (320.-noteOffset*12.5)*resFactorY;
			m_surf_note->draw(
				(posXnote*1.)/m_width-0.5,
				(posYnote*1.)/m_height-0.5,
				40./m_width,
				0);
			if (sharp) {
				int posXsharp = (m_width-(noteOffsetX + 60.0)*resFactorX) / 2.0;
				int posYsharp = (320.-noteOffset*12.5)*resFactorY;
				m_surf_sharp->draw(
					(posXsharp*1.)/m_width-0.5,
					(posYsharp*1.)/m_height-0.5,
					20./m_width,
					0);
			}
		}
	}
	Surface(theme->theme->getCurrent()).draw();
}

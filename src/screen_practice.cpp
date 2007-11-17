#include <screen_practice.h>

CScreenPractice::CScreenPractice(std::string const& name, unsigned int width, unsigned int height, Analyzer const& analyzer):
  CScreen(name,width,height), m_analyzer(analyzer)
{}

CScreenPractice::~CScreenPractice() {}

static int loadSVG(std::string const& filename, double w, double h, CairoSVG*& ptr) {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	ptr = new CairoSVG(sm->getThemePathFile(filename).c_str(), w, h);
	return sm->getVideoDriver()->initSurface(ptr->getSDLSurface());
}

void CScreenPractice::enter() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	double unitX = m_width/800.;
	double unitY = m_height/600.;
	texture_note = loadSVG("practice_note.svg", 40 * unitX, 25 * unitY, cairo_svg_note);
	texture_sharp = loadSVG("practice_sharp.svg", 25 * unitX, 75 * unitY, cairo_svg_sharp);

	theme = new CThemePractice(m_width, m_height);
	bg_texture = sm->getVideoDriver()->initSurface(theme->bg->getSDLSurface());
}

void CScreenPractice::exit()
{
	delete theme;
	delete cairo_svg_note;
	delete cairo_svg_sharp;
}

void CScreenPractice::manageEvent(SDL_Event event)
{
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	if (event.type == SDL_KEYDOWN) {
		int key = event.key.keysym.sym;
		if (key == SDLK_ESCAPE || key == SDLK_q) sm->activateScreen("Intro");
		else if (key == SDLK_SPACE) sm->getAudio()->togglePause();
	}
}

void CScreenPractice::draw()
{
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	float resFactorX = m_width/800.;
	float resFactorY = m_height/600.;

	const_cast<Analyzer&>(m_analyzer).process(); // FIXME: do in game engine thread
	Tone const* tone = m_analyzer.findTone();
	double freq = (tone ? tone->freq : 0.0);
	MusicalScale scale;

	theme->theme->clear();
	sm->getVideoDriver()->drawSurface(bg_texture);

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
			int posYnote = (340.-noteOffset*12.5)*resFactorY;
			sm->getVideoDriver()->drawSurface(texture_note,posXnote,posYnote);
			if (sharp) {
				int posXsharp = (m_width-(noteOffsetX + 60.0)*resFactorX) / 2.0;
				int posYsharp = (315.-noteOffset*12.5)*resFactorY;
				sm->getVideoDriver()->drawSurface(texture_sharp,posXsharp,posYsharp);
			}
		}
	}
	sm->getVideoDriver()->drawSurface(theme->theme->getCurrent());
}

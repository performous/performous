#include <screen_practice.h>

CScreenPractice::CScreenPractice(const char* name, unsigned int width, unsigned int height, Analyzer const& analyzer): CScreen(name,width,height), m_analyzer(analyzer)
{
}

CScreenPractice::~CScreenPractice()
{
}

static int loadSVG(std::string const& filename, double w, double h, CairoSVG*& ptr) {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	ptr = new CairoSVG(sm->getThemePathFile(filename).c_str(), w, h);
	return sm->getVideoDriver()->initSurface(ptr->getSDLSurface());
}

void CScreenPractice::enter() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	double unitX = width/800.;
	double unitY = height/600.;
	texture_note = loadSVG("practice_note.svg", 40 * unitX, 25 * unitY, cairo_svg_note);
	texture_sharp = loadSVG("practice_sharp.svg", 25 * unitX, 75 * unitY, cairo_svg_sharp);
	texture_peak = loadSVG("practice_peak.svg", 800 * unitX, 10 * unitY, cairo_svg_peak);

	theme = new CThemePractice(width, height);
	bg_texture = sm->getVideoDriver()->initSurface(theme->bg->getSDLSurface());
}

void CScreenPractice::exit()
{
	delete theme;
	delete cairo_svg_note;
	delete cairo_svg_sharp;
	delete cairo_svg_peak;
}

void CScreenPractice::manageEvent(SDL_Event event)
{
	int keypressed;
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	switch(event.type) {
		case SDL_KEYDOWN:
			keypressed = event.key.keysym.sym;
			if(keypressed == SDLK_ESCAPE || keypressed == SDLK_q) {
				sm->activateScreen("Intro");
			}
	}
}

void CScreenPractice::draw()
{
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	float resFactorX = width/800.;
	float resFactorY = height/600.;

	float freq = m_analyzer.getFreq();
	MusicalScale scale;

	theme->theme->clear();
	sm->getVideoDriver()->drawSurface(bg_texture);

	// getPeak returns 0.0 when clipping, negative values when not that loud.
	// Normalizing to [-1.0, 0.0], where -1.0 is -40 dB or less.
	double peak = std::min(0.0, std::max(-1.0, m_analyzer.getPeak() / 40.0));
	sm->getVideoDriver()->drawSurface(texture_peak, peak * 800 * resFactorX, 590.0 * resFactorY);

	if(freq != 0.0) {
		std::string text = scale.getNoteStr(freq);
		theme->notetxt.text = const_cast<char*>(text.c_str());
		theme->theme->PrintText(&theme->notetxt);
		std::vector<Tone> tones = m_analyzer.getTones();
		for (size_t i = 0; i < tones.size(); ++i) {
			if(tones[i].freq() == 0.0)
				continue;
			int note = scale.getNoteId(tones[i].freq());
			int posXnote;
			int posYnote;
			int posXsharp;
			int posYsharp;
			int octave = note/12 - 1;
			bool sharp=false;
			double noteOffset=0;
			switch(note%12) {
				case 0:
					sharp=false;
					noteOffset=0;
					break;
				case 1:
					sharp=true;
					noteOffset=0;
					break;
				case 2:
					sharp=false;
					noteOffset=1;
					break;
				case 3:
					sharp=true;
					noteOffset=1;
					break;
				case 4:
					sharp=false;
					noteOffset=2;
					break;
				case 5:
					sharp=false;
					noteOffset=3;
					break;
				case 6:
					sharp=true;
					noteOffset=3;
					break;
				case 7:
					sharp=false;
					noteOffset=4;
					break;
				case 8:
					sharp=true;
					noteOffset=4;
					break;
				case 9:
					sharp=false;
					noteOffset=5;
					break;
				case 10:
					sharp=true;
					noteOffset=5;
					break;
				case 11:
					sharp=false;
					noteOffset=6;
					break;
			}
		
			noteOffset += octave*7;
			noteOffset += 0.4 * scale.getNoteOffset(tones[i].freq());
			double noteOffsetX = -600.0 - 10.0 * tones[i].db();
			posXnote = (int) ((width-noteOffsetX*resFactorX)/2.);
			posYnote = (int) ((340.-noteOffset*12.5)*resFactorY);
			sm->getVideoDriver()->drawSurface(texture_note,posXnote,posYnote);
			if(sharp) {
				posXsharp = (int) ((width-(noteOffsetX + 60.0)*resFactorX)/2.);
				posYsharp = (int) ((315.-noteOffset*12.5)*resFactorY);
				sm->getVideoDriver()->drawSurface(texture_sharp,posXsharp,posYsharp);
			}
		}
	}
	sm->getVideoDriver()->drawSurface(theme->theme->getCurrent());
}

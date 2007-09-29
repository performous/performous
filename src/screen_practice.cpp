#include <screen_practice.h>

CScreenPractice::CScreenPractice(const char* name, unsigned int width, unsigned int height, Analyzer const& analyzer): CScreen(name,width,height), m_analyzer(analyzer)
{
}

CScreenPractice::~CScreenPractice()
{
}

void CScreenPractice::enter( void )
{
	CScreenManager * sm = CScreenManager::getSingletonPtr();

	char * theme_path = new char[1024];
	float resFactorX = width/800.;
	float resFactorY = height/600.;


	sm->getThemePathFile(theme_path,"practice_note.svg");
	cairo_svg_note = new CairoSVG(theme_path,(int)(40.*resFactorX),(int)(25.*resFactorY));
	texture_note = sm->getVideoDriver()->initSurface(cairo_svg_note->getSDLSurface());

	sm->getThemePathFile(theme_path,"practice_sharp.svg");
	cairo_svg_sharp = new CairoSVG(theme_path,(int)(25.*resFactorX),(int)(75.*resFactorY));
	texture_sharp = sm->getVideoDriver()->initSurface(cairo_svg_sharp->getSDLSurface());

	delete[] theme_path;

        theme = new CThemePractice(width,height);
	bg_texture = sm->getVideoDriver()->initSurface(theme->bg->getSDLSurface());
}

void CScreenPractice::exit( void )
{
	delete theme;
	delete cairo_svg_note;
	delete cairo_svg_sharp;
}

void CScreenPractice::manageEvent( SDL_Event event )
{
	int keypressed;
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	switch(event.type) {
		case SDL_KEYDOWN:
			keypressed = event.key.keysym.sym;
			if( keypressed == SDLK_ESCAPE || keypressed == SDLK_q ) {
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

	// FIXME: proper VU bar instead of a note sign...
	// getPeak returns 0.0 when clipping, negative values when not that loud. -40.0 can be considered silent, goes down to about -80 dB with high quality sound card when mic is muted.
	//sm->getVideoDriver()->drawSurface(texture_note, (800.0 + 20.0 * m_analyzer.getPeak()) * resFactorX, 0);

	if(freq != 0.0) {
		std::string text = scale.getNoteStr(freq);
		theme->notetxt.text = const_cast<char*>(text.c_str());
		theme->theme->PrintText(&theme->notetxt);
		std::vector<Tone> tones = m_analyzer.getTones();
		for (size_t i = 0; i < tones.size(); ++i) {
			if( tones[i].freq() == 0.0 )
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
			if( sharp ) {
				posXsharp = (int) ((width-(noteOffsetX + 60.0)*resFactorX)/2.);
				posYsharp = (int) ((315.-noteOffset*12.5)*resFactorY);
				sm->getVideoDriver()->drawSurface(texture_sharp,posXsharp,posYsharp);
			}
		}
	}
	sm->getVideoDriver()->drawSurface(theme->theme->getCurrent());
}

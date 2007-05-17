#include <screen_practice.h>

CScreenPractice::CScreenPractice(char * name)
{
	screenName = name;
}

CScreenPractice::~CScreenPractice()
{
}

void CScreenPractice::enter( void )
{
	CScreenManager * sm = CScreenManager::getSingletonPtr();

	char * theme_path = new char[1024];
	float resFactorX = sm->getWidth()/800.;
	float resFactorY = sm->getHeight()/600.;


	sm->getThemePathFile(theme_path,"practice_note.svg");
	cairo_svg_note = new CairoSVG(theme_path,(int)(40.*resFactorX),(int)(25.*resFactorY));
	texture_note = sm->getVideoDriver()->initSurface(cairo_svg_note->getSDLSurface());

	sm->getThemePathFile(theme_path,"practice_sharp.svg");
	cairo_svg_sharp = new CairoSVG(theme_path,(int)(25.*resFactorX),(int)(75.*resFactorY));
	texture_sharp = sm->getVideoDriver()->initSurface(cairo_svg_sharp->getSDLSurface());

	delete[] theme_path;

        theme = new CThemePractice();
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

void CScreenPractice::draw( void )
{
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	float resFactorX = sm->getWidth()/800.;
	float resFactorY = sm->getHeight()/600.;

	CRecord * record    = sm->getRecord();
	float freq = record->getFreq();
	int note = record->getNoteId();

        theme->theme->clear();
	sm->getVideoDriver()->drawSurface(bg_texture);

	if(freq != 0.0) {
		int posXnote;
		int posYnote;
		int posXsharp;
		int posYsharp;
		int octave = note/12;
		bool sharp;
		unsigned char noteOffset;
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
        	theme->notetxt.text = record->getNoteStr(note);
        	theme->theme->PrintText(&theme->notetxt);

		noteOffset += octave*7;

		posXnote = (int) ((sm->getWidth()-40.*resFactorX)/2.);
		posYnote = (int) ((340.-noteOffset*12.5)*resFactorY);
		sm->getVideoDriver()->drawSurface(texture_note,posXnote,posYnote);
		if( sharp ) {
			posXsharp = (int) ((sm->getWidth()-100.*resFactorX)/2.);
			posYsharp = (int) ((315.-noteOffset*12.5)*resFactorY);
			sm->getVideoDriver()->drawSurface(texture_sharp,posXsharp,posYsharp);
		}
	}
	sm->getVideoDriver()->drawSurface(theme->theme->getCurrent());
}

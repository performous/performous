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

	delete[] theme_path;

        theme = new CThemePractice();
	bg_texture = sm->getVideoDriver()->initSurface(theme->bg->getSDLSurface());
}

void CScreenPractice::exit( void )
{
	delete theme;
	delete cairo_svg_note;
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
	int posX = (int) ((sm->getWidth()-40.*resFactorX)/2.);
	int posY = 140;

	CRecord * record    = sm->getRecord();
	float freq = record->getFreq();
	int note = record->getNoteId();

        theme->theme->clear();
	sm->getVideoDriver()->drawSurface(bg_texture);

	if(freq != 0.0) {
        	theme->notetxt.text = record->getNoteStr(note);
        	theme->theme->PrintText(&theme->notetxt);
	}
	sm->getVideoDriver()->drawSurface(theme->theme->getCurrent());
	sm->getVideoDriver()->drawSurface(texture_note,posX,posY);
}

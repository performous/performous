#ifndef __SCREENPRACTICE_H__
#define __SCREENPRACTICE_H__

#include "../config.h"

#include <screen.h>
#include <cairosvg.h>
#include <theme.h>

class CScreenPractice : public CScreen {
	public:
	CScreenPractice( char * name );
	~CScreenPractice();
	void enter(void);
	void exit(void);
	void manageEvent( SDL_Event event );
	void draw(void);
	private:
        CThemePractice *theme;
	CairoSVG * cairo_svg_note;
        unsigned int texture_note;
        unsigned int bg_texture;
};

#endif

#ifndef __SCREENPRACTICE_H__
#define __SCREENPRACTICE_H__

#include "../config.h"

#include <screen.h>
#include <cairosvg.h>

class CScreenPractice : public CScreen {
	public:
	CScreenPractice( char * name );
	~CScreenPractice();
	void enter(void);
	void exit(void);
	void manageEvent( SDL_Event event );
	void draw(void);
	private:
	CairoSVG * cairo_svg;
        unsigned int texture;
};

#endif

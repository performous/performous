#ifndef __SCREENINTRO_H__
#define __SCREENINTRO_H__

#include "../config.h"

#include <screen.h>
#include <cairosvg.h>

class CScreenIntro : public CScreen {
	public:
	CScreenIntro( const char * name, unsigned int width, unsigned int height );
	~CScreenIntro();
	void enter(void);
	void exit(void);
	void manageEvent( SDL_Event event );
	void draw(void);
	private:
	CairoSVG * cairo_svg;
        unsigned int texture;
};

#endif

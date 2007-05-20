#ifndef __SCREENSCORE_H__
#define __SCREENSCORE_H__

#include "../config.h"

#include <screen.h>
#include <cairosvg.h>
#include <theme.h>

class CScreenScore : public CScreen {
	public:
	CScreenScore( char * name );
	~CScreenScore();
	void enter(void);
	void exit(void);
	void manageEvent( SDL_Event event );
	void draw(void);
	private:
	CairoSVG * cairo_svg;
	CThemeScore *theme;
	unsigned int bg_texture;
};

#endif

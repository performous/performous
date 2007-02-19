#ifndef __SCREENINTRO_H__
#define __SCREENINTRO_H__

#include "../config.h"

#include <screen.h>
#include <cairosvg.h>
#include <video_driver.h>

class CScreenIntro : public CScreen {
	public:
	CScreenIntro( char * name );
	~CScreenIntro();
	void manageEvent( SDL_Event event );
	void draw(void);
	private:
	CairoSVG * cairo_svg;
        CVideoDriver * video_driver;
};

#endif

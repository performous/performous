#ifndef __SCREENSONGS_H__
#define __SCREENSONGS_H__

#include "../config.h"

#include <screen.h>
#include <songs.h>
#include <cairosvg.h>

class CScreenSongs : public CScreen {
	public:
	CScreenSongs( char * name );
	~CScreenSongs();
	void manageEvent( SDL_Event event );
	void draw(void);
	private:
	CairoSVG * cairo_svg;
	int songId;
};

#endif

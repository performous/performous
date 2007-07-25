#ifndef __SCREENSONGS_H__
#define __SCREENSONGS_H__

#include "../config.h"

#include <screen.h>
#include <songs.h>
#include <theme.h>

class CScreenSongs : public CScreen {
	public:
	CScreenSongs( const char * name );
	~CScreenSongs();
	void enter(void);
	void exit(void);
	void manageEvent( SDL_Event event );
	void draw(void);
	private:
	CThemeSongs * theme;
	int songId;
	bool play;
	bool searchMode;
	char * searchExpr;
        unsigned int bg_texture;
};

#endif

#ifndef __SCREENSONGS_H__
#define __SCREENSONGS_H__

#include <screen.h>
#include <texture.h>
#include <songs.h>

class CScreenSongs : public CScreen {
	public:
	CScreenSongs( char * name );
	~CScreenSongs();
	void manageEvent( SDL_Event event );
	void draw(void);
	private:
	CSdlTexture * titleTex;
	int songId;
};

#endif

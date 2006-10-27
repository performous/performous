#ifndef __SCREENSING_H__
#define __SCREENSING_H__

#include <screen.h>
#include <smpeg/smpeg.h>

class CScreenSing : public CScreen {
	public:
	CScreenSing( char * name );
	~CScreenSing();
	void manageEvent( SDL_Event event );
	void draw(void);
	private:
	SDL_Surface * title;
	SDL_Surface * videoSurf;
	bool play;
	bool finished;
	unsigned int start;
	SMPEG *mpeg;
	SMPEG_Info info;
};

#endif

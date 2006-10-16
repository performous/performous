#ifndef __SCREENSING_H__
#define __SCREENSING_H__

#include <screen.h>

class CScreenSing : public CScreen {
	public:
	CScreenSing( char * name );
	~CScreenSing();
	void manageEvent( SDL_Event event );
	void draw(void);
	private:
	SDL_Surface * title;
	bool play;
	bool finished;
	unsigned int start;
};

#endif

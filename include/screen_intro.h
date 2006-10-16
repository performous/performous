#ifndef __SCREENINTRO_H__
#define __SCREENINTRO_H__

#include <screen.h>

class CScreenIntro : public CScreen {
	public:
	CScreenIntro( char * name );
	~CScreenIntro();
	void manageEvent( SDL_Event event );
	void draw(void);
	private:
	SDL_Surface *title;
	int cursor;
};

#endif

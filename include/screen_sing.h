#ifndef __SCREENSING_H__
#define __SCREENSING_H__

#include <screen.h>
#include <texture.h>

class CScreenSing : public CScreen {
	public:
	CScreenSing( char * name );
	~CScreenSing();
	void manageEvent( SDL_Event event );
	void draw(void);
	private:
	CSdlTexture * titleTex;
	bool play;
	bool finished;
	unsigned int start;
	unsigned int currentSyllable;
	unsigned int currentSentence;
};

#endif

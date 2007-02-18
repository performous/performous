#ifndef __VIDEODRIVER_H__
#define __VIDEODRIVER_H__

#include "../config.h"

class CVideoDriver {
	public:
	CVideoDriver();
	~CVideoDriver();
	SDL_Surface * init(int width, int height);
	void blank( void );
	void swap( void );
	private:
	SDL_Surface * screen;
};

#endif

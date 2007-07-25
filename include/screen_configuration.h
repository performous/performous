#ifndef __SCREENCONFIGURATION_H__
#define __SCREENCONFIGURATION_H__

#include "../config.h"

#include <screen.h>
#include <cairosvg.h>
#include <configuration.h>
#include <theme.h>

class CScreenConfiguration : public CScreen {
	public:
	CScreenConfiguration( const char * name );
	~CScreenConfiguration();
	void enter(void);
	void exit(void);
	void manageEvent( SDL_Event event );
	void draw(void);
	private:
        CThemeConfiguration *theme;
        unsigned int bg_texture;
	std::vector <CConfiguration *> configuration;
	unsigned int selected;
};

#endif

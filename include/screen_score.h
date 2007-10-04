#ifndef __SCREENSCORE_H__
#define __SCREENSCORE_H__

#include "../config.h"

#include <screen.h>
#include <cairosvg.h>
#include <theme.h>

class CScreenScore: public CScreen {
  public:
	CScreenScore(std::string const& name, unsigned int width, unsigned int height);
	~CScreenScore();
	void enter();
	void exit();
	void manageEvent(SDL_Event event);
	void draw();
  private:
	CairoSVG* cairo_svg;
	CThemeScore* theme;
	unsigned int bg_texture;
};

#endif

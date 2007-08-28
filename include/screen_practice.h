#ifndef __SCREENPRACTICE_H__
#define __SCREENPRACTICE_H__

#include "../config.h"

#include <screen.h>
#include <cairosvg.h>
#include <theme.h>

class CScreenPractice : public CScreen {
  public:
	CScreenPractice(const char* name, unsigned int width, unsigned int height, CFft const& fft);
	~CScreenPractice();
	void enter(void);
	void exit(void);
	void manageEvent( SDL_Event event );
	void draw(void);
  private:
	CFft const& m_fft;
	CThemePractice *theme;
	CairoSVG * cairo_svg_note;
	CairoSVG * cairo_svg_sharp;
	unsigned int texture_note;
	unsigned int texture_sharp;
	unsigned int bg_texture;
};

#endif

#ifndef __SCREENPRACTICE_H__
#define __SCREENPRACTICE_H__

#include "../config.h"

#include <boost/scoped_ptr.hpp>
#include <screen.h>
#include <cairosvg.h>
#include <theme.h>

class CScreenPractice : public CScreen {
  public:
	CScreenPractice(std::string const& name, unsigned int width, unsigned int height, Analyzer const& analyzer);
	~CScreenPractice();
	void enter();
	void exit();
	void manageEvent( SDL_Event event );
	void draw();
  private:
	Analyzer const& m_analyzer;
	boost::scoped_ptr<CThemePractice> theme;
	boost::scoped_ptr<CairoSVG> cairo_svg_note;
	boost::scoped_ptr<CairoSVG> cairo_svg_sharp;
	unsigned int texture_note;
	unsigned int texture_sharp;
	unsigned int bg_texture;
};

#endif

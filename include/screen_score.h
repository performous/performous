#ifndef __SCREENSCORE_H__
#define __SCREENSCORE_H__

#include "../config.h"

#include <boost/scoped_ptr.hpp>
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
	double m_time;
	boost::scoped_ptr<CThemeScore> theme;
};

#endif

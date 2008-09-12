#ifndef __SCREENSCORE_H__
#define __SCREENSCORE_H__

#include <boost/scoped_ptr.hpp>
#include "screen.hh"
#include "theme.hh"

class CScreenScore: public CScreen {
  public:
	CScreenScore(std::string const& name): CScreen(name) {}
	void enter();
	void exit();
	void manageEvent(SDL_Event event);
	void draw();
  private:
	double m_time;
	boost::scoped_ptr<CThemeScore> theme;
};

#endif

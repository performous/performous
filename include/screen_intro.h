#ifndef __SCREENINTRO_H__
#define __SCREENINTRO_H__

#include "../config.h"

#include <screen.h>
#include <surface.h>
#include <boost/scoped_ptr.hpp>

class CScreenIntro : public CScreen {
  public:
	CScreenIntro(std::string const& name);
	void enter();
	void exit();
	void manageEvent(SDL_Event event);
	void draw();
  private:
	boost::scoped_ptr<Surface> background;
};

#endif

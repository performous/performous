#ifndef __SCREENINTRO_H__
#define __SCREENINTRO_H__

#include "screen.hh"
#include "surface.hh"
#include <boost/scoped_ptr.hpp>

class CScreenIntro : public CScreen {
  public:
	CScreenIntro(std::string const& name, Audio& audio);
	void enter();
	void exit();
	void manageEvent(SDL_Event event);
	void draw();
  private:
	Audio& m_audio;
	boost::scoped_ptr<Surface> background;
};

#endif

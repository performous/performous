#ifndef __SCREENINTRO_H__
#define __SCREENINTRO_H__

#include "dialog.hh"
#include "record.hh"
#include "screen.hh"
#include "surface.hh"
#include <boost/scoped_ptr.hpp>

/// intro screen
class CScreenIntro : public CScreen {
  public:
	/// constructor
	CScreenIntro(std::string const& name, Audio& audio, Capture& capture);
	void enter();
	void exit();
	void manageEvent(SDL_Event event);
	void draw();

  private:
	Audio& m_audio;
	Capture& m_capture;
	boost::scoped_ptr<Surface> background;
	boost::scoped_ptr<Dialog> m_dialog;
};

#endif

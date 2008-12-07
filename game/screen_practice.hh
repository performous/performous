#ifndef __SCREENPRACTICE_H__
#define __SCREENPRACTICE_H__

#include <boost/scoped_ptr.hpp>
#include "screen.hh"
#include "theme.hh"
#include "opengl_text.hh"
#include "progressbar.hh"

class CScreenPractice : public CScreen {
  public:
	CScreenPractice(std::string const& name, Audio& audio, Capture& capture);
	void enter();
	void exit();
	void manageEvent( SDL_Event event );
	void draw();
  private:
	Audio& m_audio;
	Capture& m_capture;
	boost::ptr_vector<ProgressBar> m_vumeters;
	boost::scoped_ptr<CThemePractice> theme;
};

#endif

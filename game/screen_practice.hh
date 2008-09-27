#ifndef __SCREENPRACTICE_H__
#define __SCREENPRACTICE_H__

#include <boost/scoped_ptr.hpp>
#include "screen.hh"
#include "theme.hh"
#include "opengl_text.hh"

class CScreenPractice : public CScreen {
  public:
	CScreenPractice(std::string const& name, Analyzer const& analyzer);
	void enter();
	void exit();
	void manageEvent( SDL_Event event );
	void draw();
  private:
	Analyzer const& m_analyzer;
	boost::scoped_ptr<CThemePractice> theme;
};

#endif

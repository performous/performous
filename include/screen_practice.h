#ifndef __SCREENPRACTICE_H__
#define __SCREENPRACTICE_H__

#include "../config.h"

#include <boost/scoped_ptr.hpp>
#include <screen.h>
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
	boost::scoped_ptr<Surface> m_surf_note;
	boost::scoped_ptr<Surface> m_surf_sharp;
};

#endif

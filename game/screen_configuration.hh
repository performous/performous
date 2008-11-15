#ifndef __SCREENCONFIGURATION_H__
#define __SCREENCONFIGURATION_H__

#include "screen.hh"
#include "configuration.hh"
#include "theme.hh"
#include <boost/scoped_ptr.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include "opengl_text.hh"

class CScreenConfiguration: public CScreen {
  public:
	CScreenConfiguration(std::string const& name, Audio& m_audio);
	void enter();
	void exit();
	void manageEvent(SDL_Event event);
	void draw();
  private:
	Audio& m_audio;
	boost::scoped_ptr<CThemeConfiguration> theme;
	boost::ptr_vector<CConfiguration> configuration;
	unsigned int selected;
};

#endif

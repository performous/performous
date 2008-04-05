#ifndef __SCREENCONFIGURATION_H__
#define __SCREENCONFIGURATION_H__

#include "../config.h"

#include <screen.h>
#include <configuration.h>
#include <theme.h>
#include <boost/scoped_ptr.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

class CScreenConfiguration: public CScreen {
  public:
	CScreenConfiguration(std::string const& name);
	void enter();
	void exit();
	void manageEvent(SDL_Event event);
	void draw();
  private:
	boost::scoped_ptr<CThemeConfiguration> theme;
	boost::ptr_vector<CConfiguration> configuration;
	unsigned int selected;
};

#endif

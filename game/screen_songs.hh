#ifndef __SCREENSONGS_H__
#define __SCREENSONGS_H__

#include "textinput.hh"
#include <boost/scoped_ptr.hpp>
#include "screen.hh"
#include "surface.hh"
#include "sdl_helper.hh"
#include "songs.hh"
#include "theme.hh"
#include "opengl_text.hh"

class CScreenSongs : public CScreen {
  public:
	CScreenSongs(std::string const& name, std::set<std::string> const& songdirs);
	void enter();
	void exit();
	void manageEvent(SDL_Event event);
	void draw();
  private:
	boost::scoped_ptr<CThemeSongs> theme;
	double m_time;
	std::string m_cover;
	std::string m_playing;
	TextInput m_search;
	boost::scoped_ptr<Surface> m_emptyCover;
	boost::scoped_ptr<Surface> m_currentCover;
};

#endif


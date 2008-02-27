#ifndef __SCREENSONGS_H__
#define __SCREENSONGS_H__

#include <textinput.h>
#include <boost/scoped_ptr.hpp>
#include <screen.h>
#include <sdl_helper.h>
#include <songs.h>
#include <theme.h>

class CScreenSongs : public CScreen {
  public:
	CScreenSongs(std::string const& name, unsigned int width, unsigned int height, std::set<std::string> const& songdirs);
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
	unsigned int bg_texture;
	SDLSurf m_emptyCover;
	SDLSurf m_currentCover;
};

#endif


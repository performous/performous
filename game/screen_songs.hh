#ifndef __SCREENSONGS_H__
#define __SCREENSONGS_H__

#include "animvalue.hh"
#include "cachemap.hh"
#include "textinput.hh"
#include <boost/scoped_ptr.hpp>
#include "screen.hh"
#include "surface.hh"
#include "songs.hh"
#include "theme.hh"
#include "opengl_text.hh"

class CAudio;

class CScreenSongs : public CScreen {
  public:
	CScreenSongs(std::string const& name, Audio& audio, Songs& songs);
	void enter();
	void exit();
	void manageEvent(SDL_Event event);
	void draw();
  private:
	Audio& m_audio;
	Songs& m_songs;
	boost::scoped_ptr<CThemeSongs> theme;
	double m_time;
	std::string m_playing;
	std::string m_playReq;
	AnimValue m_playTimer;
	TextInput m_search;
	boost::scoped_ptr<Surface> m_emptyCover;
	Cachemap<std::string, Surface> m_covers;
};

#endif


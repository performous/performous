#ifndef PERFORMOUS_SCREEN_SONGS_HH
#define PERFORMOUS_SCREEN_SONGS_HH

#include <boost/scoped_ptr.hpp>
#include "animvalue.hh"
#include "cachemap.hh"
#include "opengl_text.hh"
#include "screen.hh"
#include "surface.hh"
#include "songs.hh"
#include "textinput.hh"
#include "theme.hh"
#include "video.hh"

class CAudio;

/// song chooser screen
class CScreenSongs : public CScreen {
  public:
	/// constructor
	CScreenSongs(std::string const& name, Audio& audio, Songs& songs);
	void enter();
	void exit();
	void manageEvent(SDL_Event event);
	void draw();

  private:
	Audio& m_audio;
	Songs& m_songs;
	boost::scoped_ptr<Surface> m_songbg;
	boost::scoped_ptr<Video> m_video;
	boost::scoped_ptr<CThemeSongs> theme;
	std::string m_playing;
	std::string m_playReq;
	AnimValue m_playTimer;
	TextInput m_search;
	boost::scoped_ptr<Surface> m_emptyCover;
	Cachemap<std::string, Surface> m_covers;
};

#endif


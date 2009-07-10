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
class ScreenSongs : public Screen {
  public:
	/// constructor
	ScreenSongs(std::string const& name, Audio& audio, Songs& songs);
	void enter();
	void exit();
	void manageEvent(SDL_Event event);
	void draw();
	void drawJukebox(); ///< draw the songbrowser in jukebox mode (fullscreen, full previews, ...)

  private:
	Audio& m_audio;
	Songs& m_songs;
	boost::scoped_ptr<Surface> m_songbg;
	boost::scoped_ptr<Video> m_video;
	boost::scoped_ptr<ThemeSongs> theme;
	std::string m_playing;
	std::string m_playReq;
	AnimValue m_playTimer;
	TextInput m_search;
	boost::scoped_ptr<Surface> m_emptyCover;
	Cachemap<std::string, Surface> m_covers;
	bool m_jukebox;
};

#endif


#pragma once

#include <boost/scoped_ptr.hpp>
#include "animvalue.hh"
#include "cachemap.hh"
#include "opengl_text.hh"
#include "screen.hh"
#include "surface.hh"
#include "textinput.hh"
#include "theme.hh"
#include "video.hh"

#include "screen_songs.hh"

class Song;
class Songs;
class Audio;
class Players;
class Database;

/// song chooser screen
class ScreenHiscore : public ScreenSongs {
  public:
	/// constructor: songs is just used to pass to ScreenSongs
	ScreenHiscore(std::string const& name, Audio& audio, Songs& songs, Database& database);

	void enter();
	void exit();
	void activateNextScreen();

	void manageEvent(SDL_Event event);
	void draw();
	void drawScores();

	void setSong (boost::shared_ptr<Song> song_)
	{
		m_song = song_;
	}

  private:
	Database& m_database;
	Players& m_players;
	boost::scoped_ptr<Surface> m_player_icon;
	boost::scoped_ptr<SvgTxtThemeSimple> m_score_text[4];
	boost::shared_ptr<Song> m_song;
};

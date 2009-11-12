#pragma once

#include <boost/scoped_ptr.hpp>
#include "animvalue.hh"
#include "cachemap.hh"
#include "screen.hh"
#include "textinput.hh"
#include "theme.hh"
#include "video.hh"
#include "hiscore.hh"
#include "layout_singer.hh"

class Audio;
class Database;
class Players;

/** song chooser screen.
  Database is passed as argument, but only the players is stored
  and used everywhere.
 */
class ScreenPlayers : public Screen {
  public:
	/// constructor
	ScreenPlayers(std::string const& name, Audio& audio, Database& database);
	void enter();
	void exit();
	void manageEvent(SDL_Event event);
	void draw();

	void setSong (boost::shared_ptr<Song> song_)
	{
		m_song = song_;
	}

  private:
	Audio& m_audio;
	Database& m_database;
	Players& m_players;
	boost::shared_ptr<Song> m_song; /// Pointer to the current song
	boost::scoped_ptr<Surface> m_songbg;
	boost::scoped_ptr<Video> m_video;
	boost::scoped_ptr<ThemeSongs> theme;
	std::string m_playing;
	std::string m_playReq;
	AnimValue m_playTimer;
	TextInput m_search;
	boost::scoped_ptr<Surface> m_emptyCover;
	Cachemap<std::string, Surface> m_covers;
	boost::scoped_ptr<LayoutSinger> m_layout_singer;
};

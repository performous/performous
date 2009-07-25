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
#include "players.hh"
#include "highscore.hh"

class CAudio;

/// song chooser screen
class ScreenPlayers : public Screen {
  public:
	/// constructor
	ScreenPlayers(std::string const& name, Audio& audio, Players& players);
	void enter();
	void exit();
	void manageEvent(SDL_Event event);
	void draw();
	void drawScores();

	void setSong (boost::shared_ptr<Song> song_)
	{
		m_song = song_;
	}

  private:
	Audio& m_audio;
	Players& m_players;
	boost::shared_ptr<Song> m_song; /// Pointer to the current song
	boost::scoped_ptr<HighScore> m_highscore;
	boost::scoped_ptr<Surface> m_songbg;
	boost::scoped_ptr<Video> m_video;
	boost::scoped_ptr<ThemeSongs> theme;
	std::string m_playing;
	std::string m_playReq;
	AnimValue m_playTimer;
	TextInput m_search;
	boost::scoped_ptr<Surface> m_emptyCover;
	Cachemap<std::string, Surface> m_covers;
	boost::scoped_ptr<Surface> m_player_icon;
	boost::scoped_ptr<SvgTxtThemeSimple> m_score_text[4];
};

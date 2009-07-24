#pragma once

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/scoped_ptr.hpp>
#include <deque>
#include "layout_singer.hh"
#include "animvalue.hh"
#include "engine.hh"
#include "guitargraph.hh"
#include "notegraph.hh"
#include "screen.hh"
#include "theme.hh"
#include "video.hh"
#include "surface.hh"
#include "opengl_text.hh"
#include "progressbar.hh"

#include "screen_players.hh"

/// shows score at end of song
class ScoreWindow {
  public:
	/// constructor
	ScoreWindow(Engine & e, Song const& song);
	/// draws ScoreWindow
	void draw();

  private:
	Song const& m_song;
	AnimValue m_pos;
	Surface m_bg;
	ProgressBar m_scoreBar;
	SvgTxtThemeSimple m_score_text;
	SvgTxtTheme m_score_rank;
	std::list<Player> m_players;
	std::string m_rank;
};

/// class for actual singing screen
class ScreenSing: public Screen {
  public:
	/// constructor
	ScreenSing(std::string const& name, Audio& audio, Capture& capture):
	  Screen(name), m_audio(audio), m_capture(capture), m_latencyAV()
	{}
	void enter();
	void exit();
	void manageEvent(SDL_Event event);
	void draw();

	void setSong (boost::shared_ptr<Song> song_)
	{
		m_song = song_;
	}

  private:
	void drawScores();
	Audio& m_audio;
	boost::shared_ptr<Song> m_song; /// Pointer to the current song
	boost::scoped_ptr<ScoreWindow> m_score_window;
	Capture& m_capture;
	boost::scoped_ptr<ProgressBar> m_progress;
	boost::scoped_ptr<Surface> m_background;
	boost::scoped_ptr<Video> m_video;
	boost::scoped_ptr<Surface> m_pause_icon;
	boost::scoped_ptr<Surface> m_player_icon;
	boost::scoped_ptr<SvgTxtThemeSimple> m_score_text[4];
	boost::scoped_ptr<Engine> m_engine;
	boost::scoped_ptr<LayoutSinger> m_layout_singer;
	boost::scoped_ptr<GuitarGraph> m_guitarGraph;
	double m_latencyAV;  // Latency between audio and video output (do not confuse with latencyAR)
	boost::scoped_ptr<ThemeSing> theme;
	AnimValue m_quitTimer;
	AnimValue m_startTimer;
};


#pragma once

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/scoped_ptr.hpp>
#include <deque>
#include "layout_singer.hh"
#include "animvalue.hh"
#include "engine.hh"
#include "guitargraph.hh"
#include "screen.hh"
#include "backgrounds.hh"
#include "theme.hh"
#include "video.hh"
#include "surface.hh"
#include "opengl_text.hh"
#include "progressbar.hh"

#include "screen_players.hh"

class Players;
class Audio;
class Capture;

/// shows score at end of song
class ScoreWindow {
  public:
	/// constructor
	ScoreWindow(Engine & e, Players & players);
	/// draws ScoreWindow
	void draw();
	bool empty() { return m_players.cur.empty(); }
  private:
	Players & m_players;
	AnimValue m_pos;
	Surface m_bg;
	ProgressBar m_scoreBar;
	SvgTxtThemeSimple m_score_text;
	SvgTxtTheme m_score_rank;
	std::string m_rank;
};

/// class for actual singing screen
class ScreenSing: public Screen {
  public:
	/// constructor
	ScreenSing(std::string const& name, Audio& audio, Capture& capture, Players& players, Backgrounds& bgs):
	  Screen(name), m_audio(audio), m_capture(capture), m_players(players), m_backgrounds(bgs), m_latencyAV()
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
	/**Activates Songs Screen or Players Screen.
	  This depends on
	  - the configuration (is Hiscore enabled)
	  - did a player reach a new hiscore
	  - is the hiscore file writable
	  */
	void activateNextScreen();
	void instrumentLayout(double time);
	Audio& m_audio;
	Capture& m_capture;
	Players& m_players;
	Backgrounds& m_backgrounds;
	boost::shared_ptr<Song> m_song; /// Pointer to the current song
	boost::scoped_ptr<ScoreWindow> m_score_window;
	boost::scoped_ptr<ProgressBar> m_progress;
	boost::scoped_ptr<Surface> m_background;
	boost::scoped_ptr<Video> m_video;
	boost::scoped_ptr<Surface> m_pause_icon;
	boost::scoped_ptr<Surface> m_help;
	boost::scoped_ptr<Engine> m_engine;
	boost::scoped_ptr<LayoutSinger> m_layout_singer;
	typedef boost::ptr_vector<GuitarGraph> Instruments;
	Instruments m_instruments;
	double m_latencyAV;  // Latency between audio and video output (do not confuse with latencyAR)
	boost::shared_ptr<ThemeSing> theme;
	AnimValue m_quitTimer;
};


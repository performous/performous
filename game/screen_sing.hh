#pragma once

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/scoped_ptr.hpp>
#include <deque>
#include "layout_singer.hh"
#include "animvalue.hh"
#include "engine.hh"
#include "instrumentgraph.hh"
#include "screen.hh"
#include "backgrounds.hh"
#include "theme.hh"
#include "surface.hh"
#include "opengl_text.hh"
#include "progressbar.hh"
#include "webcam.hh"
#include "screen_players.hh"
#include "configuration.hh"

class Players;
class Audio;
class Database;
class Video;
class Menu;

typedef boost::ptr_vector<InstrumentGraph> Instruments;
typedef boost::ptr_vector<InstrumentGraph> Dancers;

/// shows score at end of song
class ScoreWindow {
  public:
	/// constructor
	ScoreWindow(Instruments& instruments, Database& database, Dancers& dancers);
	/// draws ScoreWindow
	void draw();
	bool empty() { return m_database.scores.empty(); }
  private:
	Database& m_database;
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
	ScreenSing(std::string const& name, Audio& audio, Database& database, Backgrounds& bgs):
	  Screen(name), m_audio(audio), m_database(database), m_backgrounds(bgs), m_latencyAV(), m_only_singers_alive(true), m_selectedTrack(TrackName::LEAD_VOCAL)
	{}
	void enter();
	void exit();
	void reloadGL();
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
	bool instrumentLayout(double time);
	void danceLayout(double time);
	void drawMenu();
	Audio& m_audio;
	Database& m_database;
	Backgrounds& m_backgrounds;
	boost::shared_ptr<Song> m_song; /// Pointer to the current song
	boost::scoped_ptr<ScoreWindow> m_score_window;
	boost::scoped_ptr<ProgressBar> m_progress;
	boost::scoped_ptr<Surface> m_background;
	boost::scoped_ptr<Video> m_video;
	boost::scoped_ptr<Webcam> m_cam;
	boost::scoped_ptr<Surface> m_pause_icon;
	boost::scoped_ptr<Surface> m_help;
	boost::scoped_ptr<Engine> m_engine;
	boost::ptr_vector<LayoutSinger> m_layout_singer;
	boost::scoped_ptr<ThemeInstrumentMenu> m_menuTheme;
	Menu m_menu;
	Instruments m_instruments;
	Dancers m_dancers;
	double m_latencyAV;  // Latency between audio and video output (do not confuse with latencyAR)
	boost::shared_ptr<ThemeSing> theme;
	AnimValue m_quitTimer;
	bool m_only_singers_alive;
	std::string m_selectedTrack;
	std::string m_selectedTrackLocalized;
	ConfigItem m_vocalTrackOpts;
};


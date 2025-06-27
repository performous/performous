#pragma once

#include "animvalue.hh"
#include "audio.hh" // for AUDIO_MAX_ANALYZERS
#include "configuration.hh"
#include "menu.hh"
#include "opengl_text.hh"
#include "progressbar.hh"
#include "scorewindow.hh"
#include "screen.hh"
#include "texture.hh"
#include "theme.hh"
#include "instrumentgraph.hh"
#include "instruments.hh"

#include <deque>

class Audio;
class Backgrounds;
class Database;
class Engine;
class InstrumentGraph;
class LayoutSinger;
class Players;
class Song;
class ThemeInstrumentMenu;
class ThemeSing;
class Video;
class Webcam;

/// class for actual singing screen
class ScreenSing: public Screen {
  public:
	/// constructor
	ScreenSing(Game &game, std::string const& name, Audio& audio, Database& database, Backgrounds& bgs);
	void enter();
	void exit();
	void reloadGL();
	void manageEvent(SDL_Event event);
	void manageEvent(input::NavEvent const& event);
	Menu const& getMenu() const { return m_menu; }
	void prepare();
	void draw();

	unsigned selectedVocalTrack() const { return m_selectedVocal; }
	bool singingDuet() const { return m_singingDuet; }
	void setupVocals();

	void setSong (std::shared_ptr<Song> song_)
	{
		m_song = song_;
	}

	std::shared_ptr<Song> getSong()
	{
		return m_song;
	}

	/** Get the current position in seconds. If not known or nothing is playing, NaN is returned. **/
	double getSongPosition()
	{
		return m_audio.getPosition();
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
	void createPauseMenu();
	void drawMenu();
	void prepareVoicesMenu(unsigned moveSelectionTo = 0);
	bool devCanParticipate(input::DevType const& devType) const;
	size_t players() const; // Always have at least one player to display lyrics and prevent crashes.

	Audio& m_audio;
	Database& m_database;
	Backgrounds& m_backgrounds;
	std::shared_ptr<Song> m_song; /// Pointer to the current song
	std::unique_ptr<ScoreWindow> m_score_window;
	std::unique_ptr<ProgressBar> m_progress;
	std::unique_ptr<Texture> m_background;
	std::unique_ptr<Video> m_video;
	std::unique_ptr<Webcam> m_cam;
	std::unique_ptr<Texture> m_pause_icon;
	std::unique_ptr<Texture> m_player_icon;
	std::unique_ptr<Texture> m_help;
	std::unique_ptr<Engine> m_engine;
	std::vector<std::unique_ptr<LayoutSinger>> m_layout_singer;
	std::unique_ptr<ThemeInstrumentMenu> m_menuTheme;
	Menu m_menu;
	Instruments m_instruments;
	std::shared_ptr<ThemeSing> theme;
	AnimValue m_quitTimer;
	AnimValue m_statusTextSwitch;
	AnimValue m_DuetTimeout;
	std::string m_selectedTrack;
	std::string m_selectedTrackLocalized;
	ConfigItem m_vocalTracks[AUDIO_MAX_ANALYZERS];
	ConfigItem m_duet;
	bool m_singingDuet;
	unsigned m_selectedVocal;
	bool m_displayAutoPlay = false;
	bool keyPressed = false;
};


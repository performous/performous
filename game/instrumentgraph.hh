#pragma once

#include <vector>

#include "animvalue.hh"
#include "notes.hh"
#include "audio.hh"
#include "joystick.hh"
#include "surface.hh"
#include "opengl_text.hh"
#include "glutil.hh"
#include "menu.hh"
#include "screen.hh"
#include "fs.hh"
#include "theme.hh"


/// Represents popup messages
class Popup {
  public:
	/// Constructor
	Popup(std::string msg, Color c, double speed, SvgTxtThemeSimple* popupText, std::string info = "", SvgTxtTheme* infoText = NULL):
	  m_msg(msg), m_color(c), m_anim(AnimValue(0.0, speed)), m_popupText(popupText), m_info(info), m_infoText(infoText)
	  {
		m_anim.setTarget(1.0, false);
	}
	/// Draw the popup
	/// Returns false if it is expired
	bool draw(double offsetX) {
		double anim = m_anim.get();
		if (anim > 0.0 && m_popupText) {
			float s = 0.2 * (1.0 + anim);
			float a = 1.0 - anim;
			m_color.a = a;
			glutil::ColorRIIA color(m_color);
			m_popupText->render(m_msg);
			m_popupText->dimensions().center(0.1).middle(offsetX).stretch(s,s);
			m_popupText->draw();
			if (m_info != "" && m_infoText) {
				m_infoText->dimensions.screenBottom(-0.02).middle(-0.12 + offsetX);
				m_infoText->draw(m_info, a);
			}
			if (anim > 0.999) m_anim.setTarget(0.0, true);
		} else return false;
		return true;
	}
  private:
	std::string m_msg;  /// Popup text
	Color m_color;  /// Color
	AnimValue m_anim;  /// Animation timer
	SvgTxtThemeSimple* m_popupText;  /// Font for popup
	std::string m_info;  /// Text to show in the bottom
	SvgTxtTheme* m_infoText;  /// Font for the additional text
};


const unsigned max_panels = 10; // Maximum number of arrow lines / guitar frets

class Song;

class InstrumentGraph {
  public:
	/// Constructor
	InstrumentGraph(Audio& audio, Song const& song, input::DevType inp);
	/// Virtual destructor
	virtual ~InstrumentGraph() {}

	// Interface functions
	virtual void draw(double time) = 0;
	virtual void engine() = 0;
	virtual bool dead() const = 0;
	virtual std::string getTrack() const = 0;
	virtual std::string getDifficultyString() const = 0;
	virtual std::string getModeId() const = 0;
	virtual void changeTrack(int dir = 1) = 0;
	virtual void changeDifficulty(int dir = 1) = 0;

	// General shared functions
	void setupPauseMenu();
	void doUpdates();
	void drawMenu();
	void toggleMenu(int forcestate = -1); // 0 = close, 1 = open, -1 = auto/toggle
	void togglePause(int) { m_audio.togglePause(); }
	void quit(int) { ScreenManager::getSingletonPtr()->activateScreen("Songs"); }
	void unjoin();

	// General getters
	bool joining(double time) const { return time < m_jointime; }
	bool ready() const { return m_ready; };
	bool menuOpen() const { return m_menu.isOpen(); }
	void position(double cx, double width) { m_cx.setTarget(cx); m_width.setTarget(width); }
	unsigned stream() const { return m_stream; }
	double correctness() const { return m_correctness.get(); }
	int getScore() const { return (m_score > 0 ? m_score : 0) * m_scoreFactor; }
	input::DevType getGraphType() const { return m_input.getDevType(); }
	virtual double getWhammy() const { return 0; }

  protected:
	// Core stuff
	Audio& m_audio;
	Song const& m_song;
	input::InputDev m_input; /// input device (guitar/drums/keyboard)
	std::size_t m_stream; /// audio stream number
	AnimValue m_cx, m_width; /// controls horizontal position and width smoothly
	struct Event {
		double time;
		AnimValue glow;
		AnimValue whammy;
		int type; // 0 = miss (pick), 1 = tap, 2 = pick
		int fret;
		Duration const* dur;
		double holdTime;
		Event(double t, int ty, int f = -1, Duration const* d = NULL): time(t), glow(0.0, 5.0), whammy(0.0, 0.5), type(ty), fret(f), dur(d), holdTime(d ? d->begin : getNaN()) { if (type > 0) glow.setValue(1.0); }
	};
	typedef std::vector<Event> Events;
	Events m_events;
	typedef std::vector<Popup> Popups;
	Popups m_popups;
	Menu m_menu;

	// Shared functions for derived classes
	void drawPopups(double offsetX);
	void handleCountdown(double time, double beginTime);

	// Functions not really shared, but needed here
	Color const& color(int fret) const;

	// Media
	Surface m_button;
	SvgTxtTheme m_text;
	boost::scoped_ptr<SvgTxtThemeSimple> m_popupText;
	boost::scoped_ptr<ThemeInstrumentMenu> m_menuTheme;

	// Dynamic stuff for join menu
	ConfigItem m_selectedTrack; /// menu modifies this to select track
	ConfigItem m_selectedDifficulty; /// menu modifies this to select difficulty
	ConfigItem m_rejoin; /// menu sets this if we want to re-join
	std::string m_trackOpt;
	std::string m_difficultyOpt;
	std::string m_leftyOpt;

	// Misc counters etc.
	int m_pads; /// how many panels the current gaming mode uses
	bool m_pressed[max_panels]; /// is certain panel pressed currently
	AnimValue m_pressed_anim[max_panels]; /// animation for panel pressing
	AnimValue m_correctness;
	double m_score; /// unnormalized scores
	double m_scoreFactor; /// normalization factor
	double m_starmeter; /// when this is high enough, GodMode becomes available
	int m_streak; /// player's current streak/combo
	int m_longestStreak; /// player's longest streak/combo
	int m_bigStreak; /// next limit when a popup appears
	int m_countdown; /// countdown counter
	double m_jointime; /// when the player joined
	int m_dead; /// how many notes has been passed without hitting buttons
	bool m_ready;
};


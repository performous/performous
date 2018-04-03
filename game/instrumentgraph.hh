#pragma once

#include <vector>

#include "audio.hh"
#include "animvalue.hh"
#include "notes.hh"
#include "controllers.hh"
#include "surface.hh"
#include "opengl_text.hh"
#include "glutil.hh"
#include "menu.hh"
#include "screen.hh"
#include "fs.hh"

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
	bool draw() {
		double anim = m_anim.get();
		if (anim <= 0.0 || !m_popupText) return false;
		float a = 1.0 - anim;
		m_color.a = a;
		ColorTrans color(m_color);
		m_popupText->render(m_msg);
		{
			using namespace glmath;
			Transform trans(translate(vec3(0.0f, 0.0f, 0.5f * anim)));
			m_popupText->dimensions().center(0.1 - 0.03 * anim).middle().stretch(0.2f, 0.2f);
			m_popupText->draw();
		}
		if (m_info != "" && m_infoText) {
			m_infoText->dimensions.screenBottom(-0.02).middle(-0.12);
			m_infoText->draw(m_info);
		}
		if (anim > 0.999) m_anim.setTarget(0.0, true);
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

class Audio;
class Song;
class ThemeInstrumentMenu;

class InstrumentGraph {
public:
	/// Constructor
	InstrumentGraph(Audio& audio, Song const& song, input::DevicePtr dev);
	/// Virtual destructor
	virtual ~InstrumentGraph();

	// Interface functions
	virtual void draw(double time) = 0;
	virtual void engine() = 0;
	virtual void process(input::Event const&) {}
	virtual std::string getTrack() const = 0;
	virtual std::string getDifficultyString() const = 0;
	virtual std::string getModeId() const = 0;
	virtual void changeTrack(int dir = 1) = 0;
	virtual void changeDifficulty(int dir = 1) = 0;

	// General shared functions
	bool dead() const;
	void setupPauseMenu();
	void doUpdates();
	void drawMenu();
	void toggleMenu(int forcestate = -1); // 0 = close, 1 = open, -1 = auto/toggle
	void togglePause(int);
	void quit(int) { Game::getSingletonPtr()->activateScreen("Songs"); }
	void unjoin();

	// General getters
	bool joining(double time) const { return time < m_jointime; }
	bool ready() const { return m_ready; };
	bool menuOpen() const { return m_menu.isOpen(); }
	void position(double cx, double width) { m_cx.setTarget(cx); m_width.setTarget(width); }
	unsigned stream() const { return m_stream; }
	double correctness() const { return m_correctness.get(); }
	int getScore() const { return (m_score > 0 ? m_score : 0) * m_scoreFactor; }
	input::DevType getGraphType() const { return m_dev->type; }
	virtual double getWhammy() const { return 0; }
	bool isKeyboard() const { return m_dev->source.isKeyboard(); }

  protected:
	// Core stuff
	Audio& m_audio;
	Song const& m_song;
	std::size_t m_stream; /// audio stream number
	input::DevicePtr m_dev;
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
	void drawPopups();
	void handleCountdown(double time, double beginTime);

	// Functions not really shared, but needed here
	Color const& color(unsigned fret) const;

	// Media
	Surface m_button;
	Surface m_arrow_up;
	Surface m_arrow_down;
	Surface m_arrow_left;
	Surface m_arrow_right;
	SvgTxtTheme m_text;
	std::unique_ptr<SvgTxtThemeSimple> m_popupText;
	std::unique_ptr<ThemeInstrumentMenu> m_menuTheme;

	// Dynamic stuff for join menu
	ConfigItem m_selectedTrack; /// menu modifies this to select track
	ConfigItem m_selectedDifficulty; /// menu modifies this to select difficulty
	ConfigItem m_rejoin; /// menu sets this if we want to re-join
	ConfigItem m_leftymode; /// switch guitar notes to right-to-left direction
	std::string m_trackOpt;
	std::string m_difficultyOpt;
	std::string m_leftyOpt;

	// Misc counters etc.
	unsigned m_pads; /// how many panels the current gaming mode uses
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
	unsigned m_dead; /// how many notes has been passed without hitting buttons
	bool m_ready;
};


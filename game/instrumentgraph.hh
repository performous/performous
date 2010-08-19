#pragma once

#include <vector>

#include "animvalue.hh"
#include "notes.hh"
#include "audio.hh"
#include "joystick.hh"
#include "surface.hh"
#include "opengl_text.hh"
#include "glutil.hh"
#include "fs.hh"

/// Represents popup messages
class Popup {
  public:
	/// Constructor
	Popup(std::string msg, glutil::Color c, double speed, SvgTxtThemeSimple* popupText, std::string info = "", SvgTxtTheme* infoText = NULL):
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
			glColor4fv(m_color);
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
	glutil::Color m_color;  /// Color
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
	InstrumentGraph(Audio& audio, Song const& song, input::DevType inp):
	  m_audio(audio), m_song(song), m_input(input::DevType(inp)),
	  m_stream(),
	  m_cx(0.0, 0.2), m_width(0.5, 0.4),
	  m_text(getThemePath("sing_timetxt.svg"), config["graphic/text_lod"].f()),
	  m_pads(),
	  m_correctness(0.0, 5.0),
	  m_score(),
	  m_scoreFactor(),
	  m_starmeter(),
	  m_streak(),
	  m_longestStreak(),
	  m_bigStreak(),
	  m_countdown(3), // Display countdown 3 secs before note start
	  m_jointime(getNaN()),
	  m_dead()
	  {
		m_popupText.reset(new SvgTxtThemeSimple(getThemePath("sing_popup_text.svg"), config["graphic/text_lod"].f()));
	};
	virtual ~InstrumentGraph() {}
	
	// Interface functions
	virtual void draw(double time) = 0;
	virtual void engine() = 0;
	virtual bool dead() const = 0;
	virtual std::string getTrack() const = 0;
	virtual std::string getDifficultyString() const = 0;

	// General getters
	void position(double cx, double width) { m_cx.setTarget(cx); m_width.setTarget(width); }
	unsigned stream() const { return m_stream; }
	double correctness() const { return m_correctness.get(); }
	int getScore() const { return (m_score > 0 ? m_score : 0) * m_scoreFactor; }

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

	// Shared functions for derived classes
	void drawPopups(double offsetX);
	void handleCountdown(double time, double beginTime);
	bool joining(double time) const { return time < m_jointime; }

	// Media
	SvgTxtTheme m_text;
	boost::scoped_ptr<SvgTxtThemeSimple> m_popupText;

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
};

#pragma once

#include <vector>

#include "animvalue.hh"
#include "notes.hh"
#include "audio.hh"
#include "joystick.hh"
#include "surface.hh"
#include "opengl_text.hh"
#include "3dobject.hh"
#include "glutil.hh"
#include "fs.hh"

class Song;

class InstrumentGraph {
  public:
	/// Constructor
	InstrumentGraph(Audio& audio, Song const& song, input::DevType inp):
	  m_audio(audio), m_song(song), m_input(input::DevType(inp)),
	  m_stream(),
	  m_cx(0.0, 0.2), m_width(0.5, 0.4),
	  m_text(getThemePath("sing_timetxt.svg"), config["graphic/text_lod"].f()),
	  m_correctness(0.0, 5.0),
	  m_streakPopup(0.0, 1.0),
	  m_godmodePopup(0.0, 0.666),
	  m_score(),
	  m_scoreFactor(),
	  m_streak(),
	  m_longestStreak(),
	  m_bigStreak(),
	  m_jointime(getNaN()),
	  m_dead()
	  {
		m_popupText.reset(new SvgTxtThemeSimple(getThemePath("sing_score_text.svg"), config["graphic/text_lod"].f()));
	};

	virtual void draw(double time) = 0;
	virtual void engine() = 0;
	virtual bool dead() const = 0;
	virtual std::string getTrack() const = 0;
	virtual std::string getDifficultyString() const = 0;

	void position(double cx, double width) { m_cx.setTarget(cx); m_width.setTarget(width); }
	unsigned stream() const { return m_stream; }
	double correctness() const { return m_correctness.get(); }
	int getScore() const { return (m_score > 0 ? m_score : 0) * m_scoreFactor; }
  protected:
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

	void drawPopups(double time, double offsetX, Dimensions dimensions);

	SvgTxtTheme m_text;
	boost::scoped_ptr<SvgTxtThemeSimple> m_popupText;

	AnimValue m_correctness;
	AnimValue m_streakPopup; /// for animating the popup
	AnimValue m_godmodePopup; /// for animating the popup
	double m_score; /// unnormalized scores
	double m_scoreFactor; /// normalization factor
	double m_starmeter; /// when this is high enough, GodMode becomes available
	int m_streak; /// player's current streak/combo
	int m_longestStreak; /// player's longest streak/combo
	int m_bigStreak; /// next limit when a popup appears
	double m_jointime; /// when the player joined
	int m_dead; /// how many notes has been passed without hitting buttons
};

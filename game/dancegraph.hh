#pragma once

#include <vector>

#include <boost/ptr_container/ptr_map.hpp>
#include "animvalue.hh"
#include "song.hh"
#include "notes.hh"
#include "audio.hh"
#include "joystick.hh"
#include "surface.hh"
#include "opengl_text.hh"

class Song;

/// handles drawing of notes
class DanceGraph {
  public:
	/// constructor
	DanceGraph(Audio& audio, Song const& song);
	/** draws DanceGraph
	 * @param time at which time to draw
	 */
	void draw(double time);
	void engine();
	void position(double cx, double width) { m_cx.setTarget(cx); m_width.setTarget(width); }
	double dead(double time) const { return time > -0.5 && m_dead > 50; }
	unsigned stream() const { return m_stream; }
	double correctness() const { return m_correctness.get(); }
	int getScore() const { return m_score * m_scoreFactor; }
  private:
	void dance(double time, input::Event const& ev);
	void drawNote(int arrow_i, glutil::Color, float tBeg, float tEnd);
	glutil::Color const& color(int arrow_i) const ;
	void drawArrow(int arrow_i, float x, float y, float scale = 1.0);
	Audio& m_audio;
	input::InputDev m_input;
	Song const& m_song;
	DanceChords m_chords;
	Surface m_arrow;
	AnimValue m_cx, m_width;
	std::size_t m_stream;
	struct Event {
		double time;
		AnimValue glow;
		int type; // 0 = miss (pick), 1 = tap, 2 = pick
		int fret;
		Duration const* dur;
		double holdTime;
		Event(double t, int ty, int f = -1, Duration const* d = NULL): time(t), glow(0.0, 5.0), type(ty), fret(f), dur(d), holdTime(d ? d->begin : getNaN()) { if (type > 0) glow.setValue(1.0); }
	};
	typedef std::vector<Event> Events;
	Events m_events;
	bool m_holds[4];
	int m_dead;
	SvgTxtTheme m_text;
	AnimValue m_correctness;
	int m_flow_direction;
	double m_score;
	double m_scoreFactor;
	int m_streak;
	int m_longestStreak;
};


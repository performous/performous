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

struct DanceNote {
	DanceNote(Note note) :
		note(note), hitAnim(0.0, 5.0), releaseTime(0), score(0), isHit(false) {}
	Note note;
	AnimValue hitAnim; /// for animating hits
	double releaseTime; /// tells when a hold was ended
	int score;
	bool isHit;
};


typedef std::vector<DanceNote> DanceNotes;

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
	std::string getGameMode() const { return m_gamingMode; }
	std::string getDifficultyString() const;
  private:
	enum DanceStep {
		STEP_LEFT = 0,
		STEP_DOWN = 1,
		STEP_UP = 2,
		STEP_RIGHT = 3
	};
	void difficultyDelta(int delta);
	void difficulty(DanceDifficulty level);
	DanceDifficulty m_level;
	glutil::Color const& color(int arrow_i) const;
	void dance(double time, input::Event const& ev);
	void drawNote(DanceNote& note, double time);
	void drawArrow(int arrow_i, Texture& tex, float x, float y, float scale = 1.0, float ty1 = 0.0, float ty2 = 1.0);
	void drawMine(float x, float y, float rot = 0.0, float scale = 1.0);
	Audio& m_audio;
	Song const& m_song;
	input::InputDev m_input;
	DanceNotes m_notes;
	DanceNotes::iterator m_notesIt;
	Texture m_arrows;
	Texture m_arrows_cursor;
	Texture m_arrows_hold;
	Surface m_mine;
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
	bool m_pressed[4];
	AnimValue m_pressed_anim[4];
	int m_dead;
	SvgTxtTheme m_text;
	AnimValue m_correctness;
	int m_flow_direction;
	double m_score;
	double m_scoreFactor;
	int m_streak;
	int m_longestStreak;
	std::string m_gamingMode;
};


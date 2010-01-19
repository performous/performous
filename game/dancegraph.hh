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
		note(note), hitAnim(0.0, 5.0), releaseTime(0), error(getNaN()), score(0), isHit(false) {}
	Note note;
	AnimValue hitAnim; /// for animating hits
	double releaseTime; /// tells when a hold was ended
	double error; /// time difference between hit and correct time (negative = late)
	int score;
	bool isHit;
};


typedef std::vector<DanceNote> DanceNotes;
const size_t max_panels = 10; // Maximum number of arrow lines

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
	unsigned stream() const { return m_stream; }
	bool dead() const;
	double correctness() const { return m_correctness.get(); }
	int getScore() const { return (m_score > 0 ? m_score : 0) * m_scoreFactor; }
	std::string getGameMode() const { return m_gamingMode; }
	std::string getDifficultyString() const;
  private:
	enum DanceStep { STEP_LEFT, STEP_DOWN, STEP_UP, STEP_RIGHT };
	void gameMode(int direction);
	void difficultyDelta(int delta);
	void difficulty(DanceDifficulty level);
	DanceDifficulty m_level;
	void dance(double time, input::Event const& ev);
	void drawBeats(double time);
	void drawNote(DanceNote& note, double time);
	void drawInfo(double time, double offsetX, Dimensions dimensions);
	void drawArrow(int arrow_i, Texture& tex, float x, float y, float scale = 1.0, float ty1 = 0.0, float ty2 = 1.0);
	void drawMine(float x, float y, float rot = 0.0, float scale = 1.0);
	float panel2x(int i) { return -(m_pads * 0.5f) + m_arrow_map[i] + 0.5f; } /// Get x for an arrow line
	Audio& m_audio;
	Song const& m_song;
	input::InputDev m_input; /// input device (keyboard/dance pad)
	DanceNotes m_notes; /// contains the dancing notes for current game mode and difficulty
	DanceNotes::iterator m_notesIt; /// the first note that hasn't gone away yet
	DanceNotes::iterator m_activeNotes[max_panels]; /// hold notes that are currently pressed down
	Texture m_beat;
	Texture m_arrows;
	Texture m_arrows_cursor;
	Texture m_arrows_hold;
	Surface m_mine;
	AnimValue m_cx, m_width; /// controls horizontal position and width smoothly
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
	bool m_pressed[max_panels]; /// is certain panel pressed currently
	AnimValue m_pressed_anim[max_panels]; /// animation for panel pressing
	int m_arrow_map[max_panels]; /// game mode dependant mapping of arrows' ordering at cursor
	SvgTxtTheme m_text; /// generic text
	boost::scoped_ptr<SvgTxtThemeSimple> m_popupText; /// generic text for making popups
	AnimValue m_correctness;
	AnimValue m_streakPopup; /// for animating the popup
	int m_flow_direction;
	double m_score; /// unnormalized scores
	double m_scoreFactor; /// normalization factor
	int m_streak; /// player's current streak/combo
	int m_longestStreak; /// player's longest streak/combo
	int m_bigStreak; /// next limit when a popup appears
	int m_pads; /// how many panels the current gaming mode uses
	std::string m_gamingMode; /// current game mode
	DanceTracks::const_iterator m_curTrackIt; /// iterator to the currently selected game mode
	double m_jointime; /// when the player joined
	int m_dead; /// how many notes has been passed without hitting buttons
};


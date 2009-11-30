#pragma once

#include <vector>

#include <boost/ptr_container/ptr_map.hpp>
#include "animvalue.hh"
#include "notes.hh"
#include "audio.hh"
#include "joystick.hh"
#include "surface.hh"
#include "opengl_text.hh"
#include "3dobject.hh"
#include "glutil.hh"

class Song;

struct Chord {
	double begin, end;
	bool fret[5];
	Duration const* dur[5];
	int polyphony;
	bool tappable;
	int status; // Guitar: 0 = not played, 10 = tapped, 20 = picked, 30 = released, drums: number of pads hit
	int score;
	Chord(): begin(), end(), polyphony(), tappable(), status(), score() {
		std::fill(fret, fret + 5, false);
		std::fill(dur, dur + 5, static_cast<Duration const*>(NULL));
	}
	bool matches(bool const* fretPressed) {
		if (polyphony == 1) {
			bool shadowed = true;
			for (int i = 0; i < 5; ++i) {
				if (fret[i]) shadowed = false;
				if (!shadowed && fret[i] != fretPressed[i]) return false;
			}
			return true;
		}
		return std::equal(fret, fret + 5, fretPressed);
	}
};

static inline bool operator==(Chord const& a, Chord const& b) {
	return std::equal(a.fret, a.fret + 5, b.fret);
}

/// handles drawing of notes and waves
class GuitarGraph {
  public:
	/// constructor
	GuitarGraph(Audio& audio, Song const& song, std::string track);
	/** draws GuitarGraph
	 * @param time at which time to draw
	 */
	void updateNeck();
	void draw(double time);
	void engine();
	void position(double cx, double width) { m_cx.setTarget(cx); m_width.setTarget(width); }
	double dead(double time) const { return time > -0.5 && m_dead > 50; }
	unsigned stream() const { return m_stream; }
	double correctness() const { return m_correctness.get(); }
	std::string getTrackIndex() const { return m_track_index->first; }
	int getScore() const { return m_score * m_scoreFactor; }
	std::string getTrack() const { return m_track_index->first; }
	std::string getDifficultyString() const;
  private:
	void activateStarpower();
	void fail(double time, int fret);
	void endHold(int fret);
	void endStreak();
	Audio& m_audio;
	input::InputDev m_input;
	Song const& m_song;
	Surface m_button;
	Texture m_button_l;
	Surface m_tap;
	Surface m_neckglow;
	glutil::Color m_neckglowColor;
	Object3d m_fretObj;
	Object3d m_tappableObj;
	AnimValue m_hit[6];
	std::vector<Sample> m_samples;
	boost::scoped_ptr<Texture> m_neck;
	bool m_drums;
	bool m_use3d;
	AnimValue m_starpower;
	AnimValue m_cx, m_width;
	std::size_t m_stream;
	TrackMapConstPtr m_track_map;
	TrackMapConstPtr::const_iterator m_track_index;
	void drumHit(double time, int pad);
	void guitarPlay(double time, input::Event const& ev);
	enum Difficulty {
		DIFFICULTY_SUPAEASY,
		DIFFICULTY_EASY,
		DIFFICULTY_MEDIUM,
		DIFFICULTY_AMAZING,
		DIFFICULTYCOUNT
	} m_level;
	struct Event {
		double time;
		AnimValue glow;
		AnimValue whammy;
		int type; // 0 = miss (pick), 1 = tap, 2 = pick
		int fret;
		Duration const* dur;
		double holdTime;
		Event(double t, int ty, int f = -1, Duration const* d = NULL): time(t), glow(0.0, 5.0), whammy(0.0, 1.2), type(ty), fret(f), dur(d), holdTime(d ? d->begin : getNaN()) { if (type > 0) glow.setValue(1.0); }
	};
	typedef std::vector<Event> Events;
	Events m_events;
	unsigned m_holds[5];
	int m_dead;
	glutil::Color const& color(int fret) const;
	void drawBar(double time, float h);
	void drawNote(int fret, glutil::Color, float tBeg, float tEnd, float whammy = 0, bool tappable = false);
	void nextTrack();
	void difficultyAuto(bool tryKeepCurrent = false);
	bool difficulty(Difficulty level);
	SvgTxtTheme m_text;
	boost::scoped_ptr<SvgTxtThemeSimple> m_streakPopupText;
	void updateChords();
	typedef std::vector<Chord> Chords;
	Chords m_chords;
	Chords::iterator m_chordIt;
	typedef std::map<Duration const*, unsigned> NoteStatus; // Note in song to m_events[unsigned - 1] or 0 for not played
	NoteStatus m_notes;
	AnimValue m_correctness;
	AnimValue m_streakPopup;
	double m_score;
	double m_scoreFactor;
	double m_starmeter;
	int m_streak;
	int m_longestStreak;
	int m_bigStreak;
};


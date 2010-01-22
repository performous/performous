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
	int status; // Guitar: 0 = not played, 10 = tapped, 20 = picked, 30 = released, drums: number of pads hit, all: >100 = past
	int score;
	AnimValue hitAnim[5];
	double releaseTimes[5];
	Chord(): begin(), end(), polyphony(), tappable(), status(), score() {
		std::fill(fret, fret + 5, false);
		std::fill(dur, dur + 5, static_cast<Duration const*>(NULL));
		std::fill(hitAnim, hitAnim + 5, AnimValue(0.0, 1.5));
		std::fill(releaseTimes, releaseTimes + 5, 0.0);
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
	GuitarGraph(Audio& audio, Song const& song, bool drums, int number);
	/** draws GuitarGraph
	 * @param time at which time to draw
	 */
	void updateNeck();
	void draw(double time);
	void engine();
	void position(double cx, double width) { m_cx.setTarget(cx); m_width.setTarget(width); }
	unsigned stream() const { return m_stream; }
	bool dead() const;
	double correctness() const { return m_correctness.get(); }
	std::string getTrackIndex() const { return m_track_index->first; }
	int getScore() const { return m_score * m_scoreFactor; }
	std::string getTrack() const { return m_track_index->first; }
	std::string getDifficultyString() const;
  private:
	bool canActivateStarpower() { return (m_starmeter > 6000); }
	void activateStarpower();
	void fail(double time, int fret);
	void endHold(int fret, double time = 0.0);
	void endStreak() { m_streak = 0; m_bigStreak = 0; }
	Audio& m_audio;
	input::InputDev m_input; /// input device (guitar/drums/keyboard)
	Song const& m_song;
	Surface m_button;
	Texture m_button_l;
	Texture m_flame;
	Texture m_flame_godmode;
	Surface m_tap; /// image for 2d HOPO note cap
	Surface m_neckglow; /// image for the glow from the bottom of the neck
	glutil::Color m_neckglowColor;
	Object3d m_fretObj; /// 3d object for regular note
	Object3d m_tappableObj; /// 3d object for the HOPO note cap
	AnimValue m_hit[6];
	std::vector<Sample> m_samples; /// sound effects
	boost::scoped_ptr<Texture> m_neck; /// necks
	bool m_drums; /// are we using drums?
	bool m_use3d; /// are we using 3d?
	AnimValue m_starpower; /// how long the GodMode lasts (also used in fading the effect)
	AnimValue m_cx, m_width; /// controls horizontal position and width smoothly
	std::size_t m_stream;
	std::vector<AnimValue> m_flames[5]; /// flame effect queues for each fret
	TrackMapConstPtr m_track_map; /// tracks
	TrackMapConstPtr::const_iterator m_track_index;
	std::vector<Duration> m_solos; /// holds guitar solos
	std::vector<Duration> m_drumfills; /// holds drum fills (used for activating GodMode)
	Durations::const_iterator m_dfIt; /// current drum fill
	void updateDrumFill(double time);
	void drumHit(double time, int pad);
	void guitarPlay(double time, input::Event const& ev);
	enum Difficulty {
		DIFFICULTY_SUPAEASY, // Easy
		DIFFICULTY_EASY,     // Medium
		DIFFICULTY_MEDIUM,   // Hard
		DIFFICULTY_AMAZING,  // Expert
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
		Event(double t, int ty, int f = -1, Duration const* d = NULL): time(t), glow(0.0, 5.0), whammy(0.0, 0.5), type(ty), fret(f), dur(d), holdTime(d ? d->begin : getNaN()) { if (type > 0) glow.setValue(1.0); }
	};
	typedef std::vector<Event> Events;
	Events m_events;
	unsigned m_holds[5]; /// active hold notes
	glutil::Color const& color(int fret) const;
	glutil::Color const colorize(glutil::Color c, double time) const;
	void drawBar(double time, float h);
	void drawNote(int fret, glutil::Color, float tBeg, float tEnd, float whammy = 0, bool tappable = false, bool hit = false, double hitAnim = 0.0, double releaseTime = 0.0);
	void drawInfo(double time, double offsetX, Dimensions dimensions);
	void nextTrack(bool fast = false);
	void difficultyAuto(bool tryKeepCurrent = false);
	bool difficulty(Difficulty level);
	SvgTxtTheme m_text;
	boost::scoped_ptr<SvgTxtThemeSimple> m_popupText;
	void updateChords();
	typedef std::vector<Chord> Chords;
	Chords m_chords;
	Chords::iterator m_chordIt;
	typedef std::map<Duration const*, unsigned> NoteStatus; // Note in song to m_events[unsigned - 1] or 0 for not played
	NoteStatus m_notes;
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
	double m_drumfillScore; /// keeps track that enough hits are scored
};


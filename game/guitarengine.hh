#pragma once

#include "animvalue.hh"
#include "controllers.hh"
#include "notes.hh"
#include "playevent.hh"

#include <algorithm>
#include <map>
#include <vector>

struct GuitarChord {
	double begin, end;
	bool fret[5];
	bool fret_cymbal[5];
	Duration const* dur[5];
	unsigned polyphony;
	bool tappable;
	bool passed; // Set to true for notes that should not re-appear when rewinding
	unsigned status; // Guitar: 0 = not played, 1 = tapped, 2 = picked, drums: number of pads hit
	float score;
	mutable AnimValue hitAnim[5]; // Graphics only
	double releaseTimes[5];
	GuitarChord(): begin(), end(), polyphony(), tappable(), passed(), status(), score() {
		std::fill(fret, fret + 5, false);
		std::fill(fret_cymbal, fret_cymbal + 5, false);
		std::fill(dur, dur + 5, static_cast<Duration const*>(nullptr));
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

static inline bool operator==(GuitarChord const& a, GuitarChord const& b) {
	return std::equal(a.fret, a.fret + 5, b.fret);
}

/// Gameplay rules and scoring of guitar, bass and drums tracks, without graphics, audio or config.
/// Feed it the song time and input events; it reports what happened through a Listener.
class GuitarEngine {
  public:
	enum class Difficulty : int {
		KIDS,     // Kids
		SUPAEASY, // Easy
		EASY,     // Medium
		MEDIUM,   // Hard
		AMAZING,  // Expert
		COUNT
	};
	static constexpr double maxTolerance = 0.15; // Maximum error in seconds
	static constexpr float drumfill_min_rate = 6.0; // The rate of hits per second required to complete a drum fill
	// The visible part of the neck. Some rules depend on it (see updateLookahead).
	static constexpr float past = -0.2f; // Relative time from cursor that is considered past (out of screen)
	static constexpr float future = 1.5f; // Relative time from cursor that is considered future (out of screen)
	static unsigned basePitch(Difficulty level);
	static char const* difficultyName(Difficulty level); ///< Untranslated
	/// Does the track have notes for this difficulty level?
	static bool hasDifficulty(InstrumentTrack const& track, Difficulty level);

	/// Notifications of what happened, mostly for sound and visual effects
	struct Listener {
		virtual ~Listener() = default;
		virtual void onMiss(int /*fret*/) {} ///< Play a fail sample; fret < 0 means any
		virtual void onFlame(unsigned /*fret*/) {}
		virtual void onDrumKick() {}
		virtual void onErrorMeter(double /*error*/) {} ///< Signed timing error of a hit, in seconds
		virtual void onStarpowerActivated() {}
		virtual void onSoloEnded(unsigned /*percent*/) {}
		virtual void onBigStreak(int /*streak*/) {}
		virtual void onChordPassed(double /*time*/) {}
	};
	/// Options read from the configuration
	struct Settings {
		bool missPenalty = false; ///< game/instrument_miss_penalty
		unsigned missVolumePercent[static_cast<int>(Difficulty::COUNT)] = {}; ///< audio/miss_volume_*
	};

	GuitarEngine(Listener& listener, bool drums, bool proMode, bool songHasBRE);

	/// Select the chords of a track and difficulty. Returns false (and keeps the current level) if unusable.
	bool setDifficulty(InstrumentTrack const& track, Difficulty level);
	/// Reset what InstrumentGraph::unjoin() resets
	void resetScore();

	// Call these once per frame, in this order: beginFrame, setPressed/input for each event, update
	void beginFrame(double time);
	void setPressed(unsigned num, bool pressed) { m_pressed[num] = pressed; } ///< Guitar frets held: num = fret + 1
	void input(double time, input::Event ev);
	void update(double time);
	/// Rules that historically ran while drawing the notes visible from time + past to time + future:
	/// mark chords as passed and give the notes under a drum fill / Big Rock Ending for free.
	void updateLookahead(double time);

	// State
	Settings settings;
	Difficulty difficulty() const { return m_level; }
	typedef std::vector<GuitarChord> Chords;
	Chords const& chords() const { return m_chords; }
	std::vector<PlayEvent> const& events() const { return m_events; }
	unsigned noteEvent(Duration const* dur) const; ///< Index + 1 in events() of the note's hit, 0 if not played
	std::vector<Duration> const& solos() const { return m_solos; }
	Durations const& drumfills() const { return m_drumfills; }
	Durations::const_iterator drumfillIt() const { return m_dfIt; } ///< Current drum fill, drumfills().end() if none
	float drumfillHits() const { return m_drumfillHits; }
	double getNotesBeginTime() const { return m_chords.front().begin; }
	bool pressed(unsigned num) const { return m_pressed[num]; }
	bool solo() const { return m_solo; }
	bool canActivateStarpower() const { return (m_starmeter > 6000); }
	AnimValue const& starpower() const { return m_starpower; }
	AnimValue const& correctness() const { return m_correctness; }
	double score() const { return m_score; }
	double scoreFactor() const { return m_scoreFactor; }
	double starmeter() const { return m_starmeter; }
	int streak() const { return m_streak; }
	int longestStreak() const { return m_longestStreak; }
	double whammy() const { return m_whammy; }
	bool proMode() const { return m_proMode; }

  private:
	void updateChords();
	bool updateTom(unsigned int tomTrack, int fretId); // returns true if this tom track exists
	void activateStarpower();
	double missVolumeTarget() const;
	void fail(double time, int fret);
	void endHold(unsigned fret, double time = 0.0);
	void endBRE();
	void endStreak() { m_streak = 0; m_bigStreak = 0; }
	void updateDrumFill(double time);
	void drumHit(double time, unsigned layer, unsigned fret);
	void guitarPlay(double time, input::Event const& ev);

	Listener& m_listener;
	bool m_drums; /// are we using drums?
	bool m_proMode; /// true if pro drums. (it would be better to split guitar/trum tracks into sep classes)
	bool m_songHasBRE;
	unsigned m_pads = 5;
	Difficulty m_level;
	InstrumentTrack const* m_track = nullptr;
	Chords m_chords;
	Chords::iterator m_chordIt;
	typedef std::map<Duration const*, unsigned> NoteStatus; // Note in song to m_events[unsigned - 1] or 0 for not played
	NoteStatus m_notes;
	std::vector<PlayEvent> m_events;
	unsigned m_holds[max_panels]; /// active hold notes
	bool m_pressed[max_panels]; /// is certain panel pressed currently
	std::vector<Duration> m_solos; /// holds guitar solos
	std::vector<Duration> m_drumfills; /// holds drum fills (used for activating GodMode)
	Durations::const_iterator m_dfIt; /// current drum fill
	AnimValue m_correctness;
	AnimValue m_starpower; /// how long the GodMode lasts (also used in fading the effect)
	double m_score; /// unnormalized scores
	double m_scoreFactor; /// normalization factor
	double m_starmeter; /// when this is high enough, GodMode becomes available
	int m_streak; /// player's current streak/combo
	int m_longestStreak; /// player's longest streak/combo
	int m_bigStreak; /// next limit when a popup appears
	float m_drumfillHits; /// keeps track that enough hits are scored
	float m_drumfillScore; /// max score for the notes under drum fill
	float m_soloTotal; /// maximum solo score
	float m_soloScore; /// score during solo
	bool m_solo; /// are we currently playing a solo
	bool m_hasTomTrack; /// true if the track has at least one tom track
	double m_whammy; /// whammy value for pitch shift
};

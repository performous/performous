#pragma once

#include "instrumentgraph.hh"
#include "graphic/3dobject.hh"

#include <cstdint>

class Song;

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
	AnimValue hitAnim[5];
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

/// handles drawing of notes and waves
class GuitarGraph: public InstrumentGraph {
	enum class Difficulty : int {
		KIDS,     // Kids
		SUPAEASY, // Easy
		EASY,     // Medium
		MEDIUM,   // Hard
		AMAZING,  // Expert
		COUNT
	};

  public:
	/// constructor
	GuitarGraph(Game &game, Audio& audio, Song const& song, input::DevicePtr dev, int number);
	/** draws GuitarGraph
	 * @param time at which time to draw
	 */
	void draw(double time);
	void engine();
	std::string getTrack() const;
	std::string getDifficultyString() const;
	std::string getModeId() const;
	void changeTrack(int dir = 1);
	void changeDifficulty(int dir = 1);
	double getWhammy() const { return m_whammy; }

  private:
	// refactoring methods
	void initDrums();
	void initGuitar();
	void setupJoinMenuDifficulty();
	void setupJoinMenuDrums();
	void setupJoinMenuGuitar();
	// Engine / scoring utils
	void updateNeck();
	bool canActivateStarpower() { return (m_starmeter > 6000); }
	void activateStarpower();
	void errorMeter(double error);
	void fail(double time, int fret);
	void endHold(unsigned fret, double time = 0.0);
	void endBRE();
	void endStreak() { m_streak = 0; m_bigStreak = 0; }
	void updateDrumFill(double time);
	void drumHit(double time, unsigned layer, unsigned fret);
	void guitarPlay(double time, input::Event const& ev);

	void setupJoinMenu();
	void updateJoinMenu();
	void nextTrack(bool fast = false);
	void setTrack(const std::string& track);
	void difficultyAuto(bool tryKeepCurrent = false);
	bool difficulty(Difficulty level, bool check_only = false);

	// Graphics functions
	Color const colorize(Color c, double time) const;
	void drawNeckStuff(double time);  ///< Anything in neck coordinates
	void drawNotes(double time);  ///< Frets etc.
	void drawBar(double time, float h);
	void drawNote(unsigned fret, Color, double tBeg, double tEnd, float whammy = 0.0f, bool tappable = false, bool hit = false, double hitAnim = 0.0, double releaseTime = 0.0);
	void drawDrumfill(double tBeg, double tEnd);
	void drawInfo(double time);
	float getFretX(unsigned fret) { return (-2.0f + static_cast<float>(fret) - (m_drums ? 0.5f : 0.0f)) * (m_leftymode.b() ? -1 : 1); }
	float neckWidth() const; ///< Get the currently effective neck width (0.5 or less)
	// Chords & notes
	void updateChords();
	bool updateTom(unsigned int tomTrack, int fretId); // returns true if this tom track exists
	double getNotesBeginTime() const { return m_chords.front().begin; }

	// Media
	Texture m_tail;
	Texture m_tail_glow;
	Texture m_tail_drumfill;
	Texture m_flame;
	Texture m_flame_godmode;
	Texture m_tap; /// image for 2d HOPO note cap
	Texture m_neckglow; /// image for the glow from the bottom of the neck
	glmath::vec4 m_neckglowColor{};
	Object3d m_fretObj; /// 3d object for regular note
	Object3d m_tappableObj; /// 3d object for the HOPO note cap
	std::vector<std::string> m_samples; /// sound effects
	std::unique_ptr<Texture> m_neck; /// necks
	std::unique_ptr<SvgTxtThemeSimple> m_scoreText;
	std::unique_ptr<SvgTxtThemeSimple> m_streakText;

	// Flags
	bool m_drums = false; /// are we using drums?

	// Track stuff
	Difficulty m_level = Difficulty::MEDIUM;
	InstrumentTracksConstPtr m_instrumentTracks; /// tracks
	InstrumentTracksConstPtr::const_iterator m_track_index;
	unsigned m_holds[max_panels]; /// active hold notes

	typedef std::vector<GuitarChord> Chords;
	Chords m_chords;
	Chords::iterator m_chordIt;
	typedef std::map<Duration const*, unsigned> NoteStatus; // Note in song to m_events[unsigned - 1] or 0 for not played
	NoteStatus m_notes;
	std::vector<Duration> m_solos; /// holds guitar solos
	std::vector<Duration> m_drumfills; /// holds drum fills (used for activating GodMode)
	Durations::const_iterator m_dfIt; /// current drum fill

	// Animation & misc score keeping
	std::vector<AnimValue> m_flames[max_panels]; /// flame effect queues for each fret
	AnimValue m_errorMeter{0.0, 2.0 };
	AnimValue m_errorMeterFlash{ 0.0, 4.0 };
	AnimValue m_errorMeterFade{ 0.0, 0.333 };
	AnimValue m_drumJump{ 0.0, 12.0 };
	AnimValue m_starpower{ 0.0, 0.1 }; /// how long the GodMode lasts (also used in fading the effect)
	float m_starmeter = 0.f; /// when this is high enough, GodMode becomes available
	float m_drumfillHits = 0.f; /// keeps track that enough hits are scored
	float m_drumfillScore = 0.f; /// max score for the notes under drum fill
	float m_soloTotal = 0.f; /// maximum solo score
	float m_soloScore = 0.f; /// score during solo
	bool m_solo = false; /// are we currently playing a solo
	bool m_hasTomTrack = false; /// true if the track has at least one tom track
	bool m_proMode = false; /// true if pro drums. (it would be better to split guitar/trum tracks into sep classes)
	double m_whammy = 0.0; /// whammy value for pitch shift
};

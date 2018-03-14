#pragma once

#include <boost/ptr_container/ptr_map.hpp>

#include "instrumentgraph.hh"
#include "3dobject.hh"

class Song;

struct GuitarChord {
	double begin, end;
	bool fret[5];
	bool fret_cymbal[5];
	Duration const* dur[5];
	int polyphony;
	bool tappable;
	bool passed; // Set to true for notes that should not re-appear when rewinding
	int status; // Guitar: 0 = not played, 1 = tapped, 2 = picked, drums: number of pads hit
	int score;
	AnimValue hitAnim[5];
	double releaseTimes[5];
	GuitarChord(): begin(), end(), polyphony(), tappable(), passed(), status(), score() {
		std::fill(fret, fret + 5, false);
		std::fill(fret_cymbal, fret_cymbal + 5, false);
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

static inline bool operator==(GuitarChord const& a, GuitarChord const& b) {
	return std::equal(a.fret, a.fret + 5, b.fret);
}

/// handles drawing of notes and waves
class GuitarGraph: public InstrumentGraph {
  public:
	/// constructor
	GuitarGraph(Audio& audio, Song const& song, input::DevicePtr dev, int number);
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
	void errorMeter(float error);
	void fail(double time, int fret);
	void endHold(unsigned fret, double time = 0.0);
	void endBRE();
	void endStreak() { m_streak = 0; m_bigStreak = 0; }
	void updateDrumFill(double time);
	void drumHit(double time, unsigned layer, unsigned pad);
	void guitarPlay(double time, input::Event const& ev);

	// Media
	Texture m_tail;
	Texture m_tail_glow;
	Texture m_tail_drumfill;
	Texture m_flame;
	Texture m_flame_godmode;
	Surface m_tap; /// image for 2d HOPO note cap
	Surface m_neckglow; /// image for the glow from the bottom of the neck
	glmath::vec4 m_neckglowColor;
	Object3d m_fretObj; /// 3d object for regular note
	Object3d m_tappableObj; /// 3d object for the HOPO note cap
	std::vector<std::string> m_samples; /// sound effects
	boost::scoped_ptr<Texture> m_neck; /// necks
	boost::scoped_ptr<SvgTxtThemeSimple> m_scoreText;
	boost::scoped_ptr<SvgTxtThemeSimple> m_streakText;

	// Flags
	bool m_drums; /// are we using drums?

	// Track stuff
	enum Difficulty {
		DIFFICULTY_KIDS,     // Kids
		DIFFICULTY_SUPAEASY, // Easy
		DIFFICULTY_EASY,     // Medium
		DIFFICULTY_MEDIUM,   // Hard
		DIFFICULTY_AMAZING,  // Expert
		DIFFICULTYCOUNT
	} m_level;
	void setupJoinMenu();
	void updateJoinMenu();
	void nextTrack(bool fast = false);
	void setTrack(const std::string& track);
	void difficultyAuto(bool tryKeepCurrent = false);
	bool difficulty(Difficulty level, bool check_only = false);
	InstrumentTracksConstPtr m_instrumentTracks; /// tracks
	InstrumentTracksConstPtr::const_iterator m_track_index;
	unsigned m_holds[max_panels]; /// active hold notes

	// Graphics functions
	Color const colorize(Color c, double time) const;
	void drawNeckStuff(double time);  ///< Anything in neck coordinates
	void drawNotes(double time);  ///< Frets etc.
	void drawBar(double time, float h);
	void drawNote(int fret, Color, float tBeg, float tEnd, float whammy = 0, bool tappable = false, bool hit = false, double hitAnim = 0.0, double releaseTime = 0.0);
	void drawDrumfill(float tBeg, float tEnd);
	void drawInfo(double time);
	float getFretX(int fret) { return (-2.0f + fret- (m_drums ? 0.5 : 0)) * (m_leftymode.b() ? -1 : 1); }
	double neckWidth() const; ///< Get the currently effective neck width (0.5 or less)
	// Chords & notes
	void updateChords();
	bool updateTom(unsigned int tomTrack, unsigned int fretId); // returns true if this tom track exists
	double getNotesBeginTime() const { return m_chords.front().begin; }
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
	AnimValue m_errorMeter;
	AnimValue m_errorMeterFlash;
	AnimValue m_errorMeterFade;
	AnimValue m_drumJump;
	AnimValue m_starpower; /// how long the GodMode lasts (also used in fading the effect)
	double m_starmeter; /// when this is high enough, GodMode becomes available
	double m_drumfillHits; /// keeps track that enough hits are scored
	double m_drumfillScore; /// max score for the notes under drum fill
	double m_soloTotal; /// maximum solo score
	double m_soloScore; /// score during solo
	bool m_solo; /// are we currently playing a solo
	bool m_hasTomTrack; /// true if the track has at least one tom track
	bool m_proMode; /// true if pro drums. (it would be better to split guitar/trum tracks into sep classes)
	double m_whammy; /// whammy value for pitch shift
};

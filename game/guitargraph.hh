#pragma once

#include "instrumentgraph.hh"
#include "3dobject.hh"
#include "gamerecorder.hh"
#include "guitarengine.hh"

#include <cstdint>

class Song;

/// handles drawing of notes and waves
class GuitarGraph: public InstrumentGraph, private GuitarEngine::Listener {
  public:
	/// constructor
	GuitarGraph(Game &game, Audio& audio, Song const& song, input::DevicePtr dev, int number);
	/** draws GuitarGraph
	 * @param time at which time to draw
	 */
	void draw(double time) override;
	void engine() override;
	std::string getTrack() const override;
	std::string getDifficultyString() const override;
	std::string getModeId() const override;
	void changeTrack(int dir = 1) override;
	void changeDifficulty(int dir = 1) override;
	double getWhammy() const override { return m_engine.whammy(); }
	unsigned getScore() const override { return static_cast<unsigned>(clamp(m_engine.score() * m_engine.scoreFactor(), 0.0, 10000.0)); }
	double correctness() const override { return m_engine.correctness().get(); }
	bool canSaveReport() const override { return true; }
	void saveReport() override;

  private:
	// refactoring methods
	void initDrums();
	void initGuitar();
	void setupJoinMenuDifficulty();
	void setupJoinMenuDrums();
	void setupJoinMenuGuitar();
	void updateNeck();
	bool canActivateStarpower() const { return m_engine.canActivateStarpower(); }
	// GuitarEngine::Listener
	void onMiss(int fret) override;
	void onFlame(unsigned fret) override;
	void onDrumKick() override;
	void onErrorMeter(double error) override;
	void onStarpowerActivated() override;
	void onSoloEnded(unsigned percent) override;
	void onBigStreak(int streak) override;
	void onChordPassed(double time) override;

	// Media
	Texture m_tail;
	Texture m_tail_glow;
	Texture m_tail_drumfill;
	Texture m_flame;
	Texture m_flame_godmode;
	Texture m_tap; /// image for 2d HOPO note cap
	Texture m_neckglow; /// image for the glow from the bottom of the neck
	glmath::vec4 m_neckglowColor;
	Object3d m_fretObj; /// 3d object for regular note
	Object3d m_tappableObj; /// 3d object for the HOPO note cap
	std::vector<std::string> m_samples; /// sound effects
	std::unique_ptr<Texture> m_neck; /// necks
	std::unique_ptr<SvgTxtThemeSimple> m_scoreText;
	std::unique_ptr<SvgTxtThemeSimple> m_streakText;

	// Flags
	bool m_drums; /// are we using drums?

	// Track stuff
	using Difficulty = GuitarEngine::Difficulty;
	Difficulty level() const { return m_engine.difficulty(); }
	void setupJoinMenu();
	void updateJoinMenu();
	void nextTrack(bool fast = false);
	void setTrack(const std::string& track);
	void difficultyAuto(bool tryKeepCurrent = false);
	bool difficulty(Difficulty level, bool check_only = false);
	InstrumentTracksConstPtr m_instrumentTracks; /// tracks
	InstrumentTracksConstPtr::const_iterator m_track_index;

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
	// Gameplay rules and scoring
	GuitarEngine m_engine;

	// Animation
	std::vector<AnimValue> m_flames[max_panels]; /// flame effect queues for each fret
	AnimValue m_errorMeter;
	AnimValue m_errorMeterFlash;
	AnimValue m_errorMeterFade;
	AnimValue m_drumJump;
	GameRecorder m_recorder; /// what happened, for bug reports
};

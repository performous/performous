#pragma once

#include "instrumentgraph.hh"

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

/// handles drawing of notes
class DanceGraph: public InstrumentGraph {
  public:
	/// constructor
	DanceGraph(Audio& audio, Song const& song, input::DevicePtr dev);
	/** draws DanceGraph
	 * @param time at which time to draw
	 */
	void draw(double time);
	void engine();
	std::string getTrack() const;
	std::string getDifficultyString() const;
	std::string getModeId() const;
	void changeTrack(int dir = 1);
	void changeDifficulty(int dir = 1);

  private:
	// Difficulty & mode selection
	enum class DanceStep { STEP_LEFT, STEP_DOWN, STEP_UP, STEP_RIGHT };
	void setupJoinMenu();
	void updateJoinMenu();
	void setTrack(const std::string& track);
	void finalizeTrackChange();
	bool difficulty(DanceDifficulty level, bool check_only = false);
	DanceDifficulty m_level;
	std::string m_gamingMode; /// current game mode
	DanceTracks::const_iterator m_curTrackIt; /// iterator to the currently selected game mode

	// Scoring & drawing
	void dance(double time, input::Event const& ev);
	void drawBeats(double time);
	void drawNote(DanceNote& note, double time);
	void drawInfo(double time, Dimensions dimensions);
	void drawArrow(int arrow_i, Texture& tex, float ty1 = 0.0, float ty2 = 1.0);

	// Helpers
	float panel2x(int i) const { return getScale() * (-(m_pads * 0.5f) + m_arrow_map[i] + 0.5f); } /// Get x for an arrow line
	float getScale() const { return 1.0f / m_pads * 8.0f; }
	double getNotesBeginTime() const { return m_notes.front().note.begin; }
	glutil::danceNoteUniforms m_uniforms;

	// Note stuff
	DanceNotes m_notes; /// contains the dancing notes for current game mode and difficulty
	DanceNotes::iterator m_notesIt; /// the first note that hasn't gone away yet
	DanceNotes::iterator m_activeNotes[max_panels]; /// hold notes that are currently pressed down

	// Textures
	Texture m_beat;
	Texture m_arrows;
	Texture m_arrows_cursor;
	Texture m_arrows_hold;
	Texture m_mine;

	// Misc
	int m_arrow_map[max_panels]; /// game mode dependant mapping of arrows' ordering at cursor
	bool m_insideStop;
};


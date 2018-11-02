#pragma once

#include "animvalue.hh"
#include "texture.hh"
#include "notes.hh"

class Song;
class Database;

/// handles drawing of notes and waves
class NoteGraph {
  public:
	enum Position {FULLSCREEN, TOP, BOTTOM, LEFT, RIGHT};
	/// constructor
	NoteGraph(VocalTrack const& vocal);
	/// resets NoteGraph and Notes
	void reset();
	/** draws NoteGraph (notelines, notes, waves)
	 * @param time at which time to draw
	 * @param players reference to the list of singing Players
	 */
	void draw(double time, Database const& database, Position position = NoteGraph::FULLSCREEN);
  private:
	/// draw notebars
	void drawNotes();
	/// draw waves (what players are singing)
	void drawWaves(Database const& database);
	VocalTrack const& m_vocal;
	Texture m_notelines;
	Texture m_wave;
	Texture m_star;
	Texture m_star_hl;
	Texture m_notebar;
	Texture m_notebar_hl;
	Texture m_notebarfs;
	Texture m_notebarfs_hl;
	Texture m_notebargold;
	Texture m_notebargold_hl;
	float m_notealpha;
	AnimValue m_nlTop, m_nlBottom;
	Notes::const_iterator m_songit;
	double m_time;
	double m_max, m_min, m_noteUnit, m_baseY, m_baseX;

};


#pragma once

#include <boost/scoped_ptr.hpp>
#include "animvalue.hh"
#include "engine.hh"
#include "surface.hh"

class Song;

/// handles drawing of notes and waves
class NoteGraph {
  public:
	enum Position {FULLSCREEN, TOP};
	/// constructor
	NoteGraph(Song const& song);
	/// resets NoteGraph and Notes
	void reset();
	/** draws NoteGraph (notelines, notes, waves)
	 * @param time at which time to draw
	 * @param players reference to the list of singing Players
	 */
	void draw(double time, Players const& players, Position position = NoteGraph::FULLSCREEN);
  private:
	/// draw notebars
	void drawNotes();
	/// draw waves (what players are singing)
	void drawWaves(Players const& players);
	Song const& m_song;
	Texture m_notelines;
	Texture m_wave;
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


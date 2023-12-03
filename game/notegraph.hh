#pragma once

#include "animvalue.hh"
#include "graphic/texture.hh"
#include "notes.hh"
#include "dynamicnotegraphscaler.hh"

class Song;
class Database;
class TextureManager;
class Window;

/// handles drawing of notes and waves
class NoteGraph {
  public:
	enum class Position { FULLSCREEN, TOP, BOTTOM, LEFT, RIGHT };
	/// constructor
	NoteGraph(VocalTrack const& vocal, NoteGraphScalerPtr const&, TextureManager&);
	/// resets NoteGraph and Notes
	void reset();
	/** draws NoteGraph (notelines, notes, waves)
	 * @param time at which time to draw
	 * @param players reference to the list of singing Players
	 */
	void draw(Window&, double time, Database const& database, Position position = NoteGraph::Position::FULLSCREEN);

  private:
	/// draw notebars
	void drawNotes(Window&);
	/// draw waves (what players are singing)
	void drawWaves(Window&, Database const& database);
	float barHeight();
	float waveThickness();

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
	float m_notealpha = 0.f;
	AnimValue m_nlTop{ 0.0, 4.0 };
	AnimValue m_nlBottom{ 0.0, 4.0 };
	Notes::const_iterator m_songit;
	double m_time = 0.0;
	float m_max, m_min, m_noteUnit, m_baseY, m_baseX;
	const NoteGraphScalerPtr m_scaler;
};


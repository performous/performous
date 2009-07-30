#pragma once

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/scoped_ptr.hpp>
#include "animvalue.hh"
#include "engine.hh"
#include "surface.hh"
#include "opengl_text.hh"

class Song;

/// handles drawing of notes and waves
class GuitarGraph {
  public:
	/// constructor
	GuitarGraph(Song const& song);
	void inputProcess();
	/** draws GuitarGraph
	 * @param time at which time to draw
	 */
	void draw(double time);
	void engine(double time);
  private:
	Song const& m_song;
	Surface m_button;
	AnimValue m_pickValue;
	boost::ptr_vector<Texture> m_necks;
	bool m_drums;
	std::size_t m_instrument;
	enum Difficulty {
		DIFFICULTY_SUPAEASY,
		DIFFICULTY_EASY,
		DIFFICULTY_MEDIUM,
		DIFFICULTY_AMAZING,
		DIFFICULTYCOUNT
	} m_level;
	glutil::Color const& color(int fret) const;
	void drawBar(double time, float h);
	void difficultyAuto();
	bool difficulty(Difficulty level);
	SvgTxtTheme m_text;
	typedef std::map<Duration const*, int> NoteStatus;
	NoteStatus m_notes;
	int m_score;
};


#pragma once

#include <boost/scoped_ptr.hpp>
#include "animvalue.hh"
#include "engine.hh"
#include "surface.hh"

class Song;

/// handles drawing of notes and waves
class GuitarGraph {
  public:
	/// constructor
	GuitarGraph(Song const& song);

	/** draws GuitarGraph
	 * @param time at which time to draw
	 */
	void draw(double time);
  private:
	Song const& m_song;
	Texture m_neck;
};


#ifndef PERFORMOUS_NOTEGRAPH_HH
#define PERFORMOUS_NOTEGRAPH_HH

#include <boost/scoped_ptr.hpp>
#include "animvalue.hh"
#include "engine.hh"
#include "surface.hh"

class Song;

class NoteGraph {
  public:
	NoteGraph(Song const& song);
	void reset();
	void draw(double time, std::list<Player> const& players);
  private:
	void drawNotes();
	void drawWaves(std::list<Player> const& players);
	Song const& m_song;
	boost::scoped_ptr<Texture> m_notelines;
	boost::scoped_ptr<Texture> m_wave;
	boost::scoped_ptr<Texture> m_notebar;
	boost::scoped_ptr<Texture> m_notebar_hl;
	boost::scoped_ptr<Texture> m_notebarfs;
	boost::scoped_ptr<Texture> m_notebarfs_hl;
	boost::scoped_ptr<Texture> m_notebargold;
	boost::scoped_ptr<Texture> m_notebargold_hl;
	float m_notealpha;
	AnimValue m_nlTop, m_nlBottom;
	Notes::const_iterator m_songit;
	double m_time;
	double m_max, m_min, m_noteUnit, m_baseY, m_baseX;

};

#endif


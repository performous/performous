#ifndef __SCREENSING_H__
#define __SCREENSING_H__

#include "../config.h"

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/scoped_ptr.hpp>
#include "engine.hh"
#include "screen.hh"
#include "pitch_graph.hh"
#include "theme.hh"
#include "video.hh"
#include "lyrics.hh"
#include "sdl_helper.hh"
#include "surface.hh"

class CScreenSing: public CScreen {
  public:
	CScreenSing(std::string const& name, boost::ptr_vector<Analyzer>& analyzers):
	  CScreen(name), m_analyzers(analyzers)
	{}
	void enter();
	void exit();
	void manageEvent(SDL_Event event);
	void draw();
  private:
	boost::ptr_vector<Analyzer>& m_analyzers;
	boost::scoped_ptr<Surface> m_background;
	boost::scoped_ptr<Video> m_video;
	boost::scoped_ptr<Surface> m_notelines;
	boost::scoped_ptr<Surface> m_wave;
	boost::scoped_ptr<Surface> m_notebar;
	boost::scoped_ptr<Engine> m_engine;
	// Keeps the pitch tracking graphics
	// in separate surface
	PitchGraph pitchGraph;
	std::vector<Note> m_sentence;
	bool play;
	bool finished;
	double playOffset;
	float m_notealpha;
	double min, max;
	boost::scoped_ptr<CThemeSing> theme;
	boost::scoped_ptr<Lyrics> lyrics;
	Song::notes_t::const_iterator m_songit;
};

#endif

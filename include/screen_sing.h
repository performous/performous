#ifndef __SCREENSING_H__
#define __SCREENSING_H__

#include "../config.h"

#include <boost/scoped_ptr.hpp>
#include <screen.h>
#include <pitch_graph.h>
#include <theme.h>
#include <video.h>
#include <lyrics.h>
#include <sdl_helper.h>
#include <surface.h>

class CScreenSing: public CScreen {
  public:
	CScreenSing(std::string const& name, Analyzer const& analyzer):
	  CScreen(name), m_analyzer(analyzer)
	{}
	void enter();
	void exit();
	void manageEvent(SDL_Event event);
	void draw();
  private:
	Analyzer const& m_analyzer;
	boost::scoped_ptr<Surface> m_background;
	boost::scoped_ptr<Video> m_video;
	// Keeps the pitch tracking graphics
	// in separate surface
	PitchGraph pitchGraph;
	std::vector<Note> m_sentence;
	bool play;
	bool finished;
	double playOffset;
	boost::scoped_ptr<CThemeSing> theme;
	boost::scoped_ptr<Lyrics> lyrics;
	Song::notes_t::const_iterator m_songit;
};

#endif

#ifndef __SCREENSING_H__
#define __SCREENSING_H__

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/scoped_ptr.hpp>
#include "engine.hh"
#include "screen.hh"
#include "theme.hh"
#include "video.hh"
#include "lyrics.hh"
#include "sdl_helper.hh"
#include "surface.hh"
#include "opengl_text.hh"
#include "progressbar.hh"

class ScoreWindow {
  public:
	ScoreWindow(CScreenManager const* sm, Engine const& e);
	void draw();
  private:
	Surface m_bg;
	ProgressBar m_scoreBar;
	SvgTxtTheme m_score_text;
	SvgTxtTheme m_score_rank;
	std::list<Player> m_players;
};

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
	boost::scoped_ptr<ScoreWindow> m_score_window;
	boost::ptr_vector<Analyzer>& m_analyzers;
	boost::scoped_ptr<ProgressBar> m_progress;
	boost::scoped_ptr<Surface> m_background;
	boost::scoped_ptr<Video> m_video;
	boost::scoped_ptr<Surface> m_pause_icon;
	boost::scoped_ptr<Texture> m_notelines;
	boost::scoped_ptr<Texture> m_wave;
	boost::scoped_ptr<Texture> m_notebar;
	boost::scoped_ptr<Texture> m_notebar_hl;
	boost::scoped_ptr<Texture> m_notebarfs;
	boost::scoped_ptr<Texture> m_notebarfs_hl;
	boost::scoped_ptr<Texture> m_notebargold;
	boost::scoped_ptr<Texture> m_notebargold_hl;
	boost::scoped_ptr<Engine> m_engine;
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

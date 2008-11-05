#ifndef __SCREENSING_H__
#define __SCREENSING_H__

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/scoped_ptr.hpp>
#include <deque>
#include "animvalue.hh"
#include "engine.hh"
#include "screen.hh"
#include "theme.hh"
#include "video.hh"
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
	SvgTxtThemeSimple m_score_text;
	SvgTxtTheme m_score_rank;
	std::list<Player> m_players;
	std::string m_rank;
};

class LyricRow {
  public:
	AnimValue extraspacing, fade;
	typedef Song::notes_t::const_iterator Iterator;
	LyricRow(Iterator& it, Iterator const& eof): extraspacing(0.0, 2.0), fade(0.0, 0.6) {
		fade.setTarget(1.0);
		m_begin = it;
		while (it != eof && it->type != Note::SLEEP) ++it;
		m_end = it;
		if (it != eof) ++it;
		if (m_begin == m_end) throw std::logic_error("Empty sentence");
	}
	bool expired(double time) const {
		double lastTime = 0.0;
		for (Iterator it = m_begin; it != m_end; ++it) lastTime = it->end;
		return time > lastTime;
	}
	void draw(SvgTxtTheme& txt, double time, double pos) const {
		std::vector<TZoomText> sentence;
		for (Iterator it = m_begin; it != m_end; ++it) {
			sentence.push_back(TZoomText(it->syllable));
			bool current = (time >= it->begin && time < it->end);
			sentence.back().factor = current ? 1.2 - 0.2 * (time - it->begin) / (it->end - it->begin) : 1.0;
		}
		txt.dimensions.screenBottom(pos);
		txt.draw(sentence, fade.get());
	}
  private:
	Iterator m_begin, m_end;
};

class CScreenSing: public CScreen {
  public:
	CScreenSing(std::string const& name, boost::ptr_vector<Analyzer>& analyzers):
	  CScreen(name), m_analyzers(analyzers), m_latencyAV(), m_nlTop(0.0, 2.0), m_nlBottom(0.0, 2.0)
	{}
	void enter();
	void exit();
	void manageEvent(SDL_Event event);
	void draw();
	enum SongStatus { NORMAL, INSTRUMENTAL_BREAK, FINISHED };
	SongStatus songStatus() const;
  private:
	boost::scoped_ptr<ScoreWindow> m_score_window;
	boost::ptr_vector<Analyzer>& m_analyzers;
	boost::scoped_ptr<ProgressBar> m_progress;
	boost::scoped_ptr<Surface> m_background;
	boost::scoped_ptr<Video> m_video;
	boost::scoped_ptr<Surface> m_pause_icon;
	boost::scoped_ptr<Surface> m_player_icon;
	boost::scoped_ptr<SvgTxtThemeSimple> m_score_text[2];
	boost::scoped_ptr<Texture> m_notelines;
	boost::scoped_ptr<Texture> m_wave;
	boost::scoped_ptr<Texture> m_notebar;
	boost::scoped_ptr<Texture> m_notebar_hl;
	boost::scoped_ptr<Texture> m_notebarfs;
	boost::scoped_ptr<Texture> m_notebarfs_hl;
	boost::scoped_ptr<Texture> m_notebargold;
	boost::scoped_ptr<Texture> m_notebargold_hl;
	boost::scoped_ptr<Engine> m_engine;
	double m_latencyAV;  // Latency between audio and video output (do not confuse with latencyAR)
	float m_notealpha;
	boost::scoped_ptr<CThemeSing> theme;
	Song::notes_t::const_iterator m_songit;
	Song::notes_t::const_iterator m_lyricit;
	AnimValue m_nlTop, m_nlBottom;
	std::deque<LyricRow> m_lyrics;
};

#endif

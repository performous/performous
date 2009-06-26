#ifndef PERFORMOUS_SCREEN_SING_HH
#define PERFORMOUS_SCREEN_SING_HH

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/scoped_ptr.hpp>
#include <deque>
#include "animvalue.hh"
#include "engine.hh"
#include "notegraph.hh"
#include "screen.hh"
#include "theme.hh"
#include "video.hh"
#include "surface.hh"
#include "opengl_text.hh"
#include "progressbar.hh"

/// shows score at end of song
class ScoreWindow {
  public:
	/// constructor
	ScoreWindow(Engine & e);
	/// draws ScoreWindow
	void draw();

  private:
	AnimValue m_pos;
	Surface m_bg;
	ProgressBar m_scoreBar;
	SvgTxtThemeSimple m_score_text;
	SvgTxtTheme m_score_rank;
	std::list<Player> m_players;
	std::string m_rank;
};

/// handles songlyrics
class LyricRow {
  public:
	AnimValue extraspacing; ///< extraspacing for lyrics (used when the previous line is removed)
	AnimValue fade; ///< fade
	/// iterator
	typedef Notes::const_iterator Iterator;
	/// constructor
	LyricRow(Iterator& it, Iterator const& eof): extraspacing(0.0, 2.0), fade(0.0, 0.6) {
		fade.setTarget(1.0);
		m_begin = it;
		while (it != eof && it->type != Note::SLEEP) ++it;
		m_end = it;
		if (it != eof) ++it;
		if (m_begin == m_end) throw std::logic_error("Empty sentence");
	}
	/// lyric expired?
	bool expired(double time) const {
		double lastTime = 0.0;
		for (Iterator it = m_begin; it != m_end; ++it) lastTime = it->end;
		return time > lastTime;
	}
	/// draw/print lyrics
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

/// class for actual singing screen
class ScreenSing: public Screen {
  public:
	/// constructor
	ScreenSing(std::string const& name, Audio& audio, Songs& songs, Capture& capture):
	  Screen(name), m_audio(audio), m_songs(songs), m_capture(capture), m_latencyAV()
	{}
	void enter();
	void exit();
	void manageEvent(SDL_Event event);
	void draw();

  private:
	void drawNonKaraoke(double time);
	Audio& m_audio;
	Songs& m_songs;  // TODO: take song instead of all of them
	boost::scoped_ptr<ScoreWindow> m_score_window;
	Capture& m_capture;
	boost::scoped_ptr<ProgressBar> m_progress;
	boost::scoped_ptr<Surface> m_background;
	boost::scoped_ptr<Video> m_video;
	boost::scoped_ptr<Surface> m_pause_icon;
	boost::scoped_ptr<Surface> m_player_icon;
	boost::scoped_ptr<SvgTxtThemeSimple> m_score_text[2];
	boost::scoped_ptr<Engine> m_engine;
	boost::scoped_ptr<NoteGraph> m_noteGraph;
	double m_latencyAV;  // Latency between audio and video output (do not confuse with latencyAR)
	boost::scoped_ptr<ThemeSing> theme;
	Notes::const_iterator m_lyricit;
	std::deque<LyricRow> m_lyrics;
	AnimValue m_quitTimer;
};

#endif

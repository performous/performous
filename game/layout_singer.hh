#pragma once
#include "engine.hh"
#include "theme.hh"
#include "notegraph.hh"

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

class LayoutSinger {
  public:
	LayoutSinger(Songs &_songs, Engine &_engine, ThemeSing& _theme);
	~LayoutSinger();
	void reset();
	void draw(double time);
	double lyrics_begin();
  private:
	Engine& m_engine;
  	Songs& m_songs;
	ThemeSing& m_theme;
	NoteGraph m_noteGraph;
	Notes::const_iterator m_lyricit;
	std::deque<LyricRow> m_lyrics;
};

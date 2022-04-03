#pragma once

#include "theme.hh"
#include "opengl_text.hh"
#include "notegraph.hh"
#include "configuration.hh"

#include <deque>

class Database;

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
		while (it != eof && it->type != Note::Type::SLEEP) ++it;
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
	void draw(SvgTxtTheme& txt, double time, Dimensions &dim) const {
		std::vector<TZoomText> sentence;
		for (Iterator it = m_begin; it != m_end; ++it) {
			sentence.push_back(TZoomText(it->syllable));
			if(!config["game/Textstyle"].i()) {
			bool current = (time >= it->begin && time < it->end);
			sentence.back().factor = current ? 1.1 - 0.1 * (time - it->begin) / (it->end - it->begin) : 1.0; // Zoom-in and out while it's the current syllable.
			} else {
			bool current = time >=it->begin;
			sentence.back().factor = current ? std::min(1.0 + (0.15 * (time - it->begin) / (it->end - it->begin)), 1.1) : 1.0; // Zoom-in and out syllable proportionally to their length.
			}
		}
		ColorTrans c(Color::alpha(fade.get()));
		txt.dimensions = dim;
		txt.draw(sentence, true);
	}

  private:
	Iterator m_begin, m_end;
};

class LayoutSinger {
  public:
	enum class PositionMode { FULL, TOP, BOTTOM, LEFT, RIGHT };
	/// ThemeSing is optional if you want to use drawScore only
	LayoutSinger(VocalTrack& vocal, Database& database, NoteGraphScalerPtr const&, std::shared_ptr<ThemeSing> theme = std::make_shared<ThemeSing>());
	~LayoutSinger();
	void reset();
	void draw(double time, PositionMode position = LayoutSinger::PositionMode::FULL);
	void drawScore(PositionMode position);
	double lyrics_begin() const;
	void hideLyrics(bool hide = true) { m_hideLyrics = hide; }
  private:
	VocalTrack& m_vocal;
	NoteGraph m_noteGraph;
	Notes::const_iterator m_lyricit;
	std::deque<LyricRow> m_lyrics;
	std::unique_ptr<Texture> m_player_icon;
	std::unique_ptr<SvgTxtThemeSimple> m_score_text[4];
	std::unique_ptr<SvgTxtThemeSimple> m_line_rank_text[4];
	Database& m_database;
	std::shared_ptr<ThemeSing> m_theme;
	AnimValue m_feedbackFader;
	bool m_hideLyrics;
};

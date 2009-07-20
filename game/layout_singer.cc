#include "layout_singer.hh"


LayoutSinger::LayoutSinger(Songs &_songs, Engine &_engine, ThemeSing& _theme): m_engine(_engine), m_songs(_songs), m_theme(_theme), m_noteGraph(_songs.current()),m_lyricit(_songs.current().notes.begin()), m_lyrics() {};

LayoutSinger::~LayoutSinger() {};

void LayoutSinger::reset() {
	m_lyricit = m_songs.current().notes.begin();
	m_lyrics.clear();
}

void LayoutSinger::draw(double time) {
	Song &song = m_songs.current();

	// Draw notes and pitch waves (only when not in karaoke mode)
	if (!config["game/karaoke_mode"].b()) {
		std::list<Player> const& players = m_engine.getPlayers();
		m_noteGraph.draw(time, players);
	}

	// Draw the lyrics
	const double basepos = -0.1;
	const double linespacing = 0.06;
	bool dirty;
	do {
		dirty = false;
		if (!m_lyrics.empty() && m_lyrics[0].expired(time)) {
			// Add extra spacing to replace the removed row
			if (m_lyrics.size() > 1) m_lyrics[1].extraspacing.move(m_lyrics[0].extraspacing.get() + 1.0);
			m_lyrics.pop_front();
			dirty = true;
		}
		if (!dirty && m_lyricit != song.notes.end() && m_lyricit->begin < time + 4.0) {
			m_lyrics.push_back(LyricRow(m_lyricit, song.notes.end()));
			dirty = true;
		}
	} while (dirty);
	double pos = basepos;
	for (size_t i = 0; i < m_lyrics.size(); ++i, pos += linespacing) {
		pos += m_lyrics[i].extraspacing.get() * linespacing;
		if (i == 0) m_lyrics[0].draw(m_theme.lyrics_now, time, pos);
		else if (i == 1) m_lyrics[1].draw(m_theme.lyrics_next, time, pos);
	}
}

double LayoutSinger::lyrics_begin() {
	return m_lyricit->begin;
}

#include "layout_singer.hh"


LayoutSinger::LayoutSinger(Songs &_songs, Engine &_engine, ThemeSing& _theme): m_engine(_engine), m_songs(_songs), m_theme(_theme), m_noteGraph(_songs.current()),m_lyricit(_songs.current().notes.begin()), m_lyrics() {
	m_score_text[0].reset(new SvgTxtThemeSimple(getThemePath("sing_score_text.svg"), config["graphic/text_lod"].f()));
	m_score_text[1].reset(new SvgTxtThemeSimple(getThemePath("sing_score_text.svg"), config["graphic/text_lod"].f()));
	m_score_text[2].reset(new SvgTxtThemeSimple(getThemePath("sing_score_text.svg"), config["graphic/text_lod"].f()));
	m_score_text[3].reset(new SvgTxtThemeSimple(getThemePath("sing_score_text.svg"), config["graphic/text_lod"].f()));
	m_player_icon.reset(new Surface(getThemePath("sing_pbox.svg")));
}

LayoutSinger::~LayoutSinger() {};

void LayoutSinger::reset() {
	m_lyricit = m_songs.current().notes.begin();
	m_lyrics.clear();
}

void LayoutSinger::draw(double time, Position position) {
	Song &song = m_songs.current();

	// Draw notes and pitch waves (only when not in karaoke mode)
	if (!config["game/karaoke_mode"].b()) {
		std::list<Player> const& players = m_engine.getPlayers();
		switch(position) {
			case LayoutSinger::BOTTOM:
				m_noteGraph.draw(time, players, NoteGraph::FULLSCREEN);
				break;
			case LayoutSinger::MIDDLE:
				m_noteGraph.draw(time, players, NoteGraph::TOP);
				break;
		}
	}

	// Draw the lyrics
	double linespacing = 0.0;
	Dimensions pos;
	switch(position) {
		case LayoutSinger::BOTTOM:
			pos.screenBottom(-0.1);
			linespacing = 0.06;
			break;
		case LayoutSinger::MIDDLE:
			linespacing = 0.04;
			break;
	}
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
	for (size_t i = 0; i < m_lyrics.size(); ++i, pos.move(0.0, linespacing)) {
		pos.move(0.0, m_lyrics[i].extraspacing.get() * linespacing);
		if (i == 0) m_lyrics[0].draw(m_theme.lyrics_now, time, pos);
		else if (i == 1 && position == LayoutSinger::BOTTOM) m_lyrics[1].draw(m_theme.lyrics_next, time, pos);
	}

	// Score display
	if (!config["game/karaoke_mode"].b() ) {// draw score if not in karaoke mode
		std::list<Player> const& players = m_engine.getPlayers();
		unsigned int i = 0;
		for (std::list<Player>::const_iterator p = players.begin(); p != players.end(); ++p, ++i) {
			float act = p->activity();
			//if (act == 0.0f) continue;
			glColor4f(p->m_color.r, p->m_color.g, p->m_color.b,act);
			switch(position) {
				case LayoutSinger::BOTTOM:
					m_player_icon->dimensions.left(-0.5 + 0.01 + 0.25 * i).fixedWidth(0.075).screenTop(0.055);
					break;
				case LayoutSinger::MIDDLE:
					m_player_icon->dimensions.right(0.35).fixedHeight(0.050).screenTop(0.025 + 0.050 * i);
					break;
			}
			m_player_icon->draw();
			m_score_text[i%4]->render((boost::format("%04d") % p->getScore()).str());
			switch(position) {
				case LayoutSinger::BOTTOM:
					m_score_text[i%4]->dimensions().middle(-0.350 + 0.01 + 0.25 * i).fixedHeight(0.075).screenTop(0.055);
					break;
				case LayoutSinger::MIDDLE:
					m_score_text[i%4]->dimensions().right(0.45).fixedHeight(0.050).screenTop(0.025 + 0.050 * i);
					break;
			}
			m_score_text[i%4]->draw();
			glColor4f(1.0, 1.0, 1.0, 1.0);
		}
	}
}

double LayoutSinger::lyrics_begin() {
	return m_lyricit->begin;
}

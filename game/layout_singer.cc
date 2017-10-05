#include "layout_singer.hh"

#include "configuration.hh"
#include "fs.hh"
#include "song.hh"
#include "database.hh"

#include <list>
#include <boost/format.hpp>

LayoutSinger::LayoutSinger(VocalTrack& vocal, Database& database, boost::shared_ptr<ThemeSing> theme):
  m_vocal(vocal), m_noteGraph(vocal),m_lyricit(vocal.notes.begin()), m_lyrics(), m_database(database), m_theme(theme), m_hideLyrics() {
	m_score_text[0].reset(new SvgTxtThemeSimple(findFile("sing_score_text.svg"), config["graphic/text_lod"].f()));
	m_score_text[1].reset(new SvgTxtThemeSimple(findFile("sing_score_text.svg"), config["graphic/text_lod"].f()));
	m_score_text[2].reset(new SvgTxtThemeSimple(findFile("sing_score_text.svg"), config["graphic/text_lod"].f()));
	m_score_text[3].reset(new SvgTxtThemeSimple(findFile("sing_score_text.svg"), config["graphic/text_lod"].f()));
	m_line_rank_text[0].reset(new SvgTxtThemeSimple(findFile("sing_score_text.svg"), config["graphic/text_lod"].f()));
	m_line_rank_text[1].reset(new SvgTxtThemeSimple(findFile("sing_score_text.svg"), config["graphic/text_lod"].f()));
	m_line_rank_text[2].reset(new SvgTxtThemeSimple(findFile("sing_score_text.svg"), config["graphic/text_lod"].f()));
	m_line_rank_text[3].reset(new SvgTxtThemeSimple(findFile("sing_score_text.svg"), config["graphic/text_lod"].f()));
	m_player_icon.reset(new Surface(findFile("sing_pbox.svg")));
}

LayoutSinger::~LayoutSinger() {}

void LayoutSinger::reset() {
	m_lyricit = m_vocal.notes.begin();
	m_lyrics.clear();
}

void LayoutSinger::drawScore(PositionMode position) {
	unsigned int i = 0, j = 0;
	for (std::list<Player>::const_iterator p = m_database.cur.begin(); p != m_database.cur.end(); ++p, ++i) {
		if (p->m_vocal.name != m_vocal.name) continue;
		Color color(p->m_color.r, p->m_color.g, p->m_color.b, p->activity());
		if (color.a == 0.0) continue;
		m_score_text[i%4]->render((boost::format("%04d") % p->getScore()).str());
		switch(position) {
			case LayoutSinger::FULL:
				m_player_icon->dimensions.left(-0.5 + 0.01 + 0.125 * j).fixedWidth(0.035).screenTop(0.055);
				if (m_database.cur.size() < 9){
					m_player_icon->dimensions.left(-0.5 + 0.01 + 0.125 * j).fixedWidth(0.035).screenTop(0.05);
					m_score_text[i%4]->dimensions().middle(-0.425 + 0.01 + 0.125 * j).fixedHeight(0.035).screenTop(0.055);}			
				else{
					m_player_icon->dimensions.left(-0.506 + 0.01 + 0.0905 * j).fixedWidth(0.028).screenTop(0.050);				
					m_score_text[i%4]->dimensions().middle(-0.519 + 0.08 + 0.0905 * j).fixedHeight(0.029).screenTop(0.053);}
				break;
			case LayoutSinger::TOP:
				m_player_icon->dimensions.right(0.35).fixedHeight(0.050).screenTop(0.025 + 0.050 * j);
				m_score_text[i%4]->dimensions().right(0.45).fixedHeight(0.050).screenTop(0.025 + 0.050 * j);
				break;
			case LayoutSinger::BOTTOM:
				m_player_icon->dimensions.right(0.35).fixedHeight(0.050).center(0.025 + 0.050 * j);
				m_score_text[i%4]->dimensions().right(0.45).fixedHeight(0.050).center(0.025 + 0.050 * j);
				break;
			case LayoutSinger::LEFT:
			case LayoutSinger::RIGHT:
				m_player_icon->dimensions.left(-0.5 + 0.01 + 0.25 * j).fixedWidth(0.075).screenTop(0.055);
				m_score_text[i%4]->dimensions().middle(-0.350 + 0.01 + 0.25 * j).fixedHeight(0.075).screenTop(0.055);
				break;
		}
		{
			ColorTrans c(color);
			m_player_icon->draw();
			m_score_text[i%4]->draw();
		}
		// Give some feedback on how well the last lyrics row went
		double fact = p->m_feedbackFader.get();
		if (p->m_prevLineScore > 0.5 && fact > 0) {
			std::string prevLineRank;
			double fzoom = 3.0 / (2.0 + fact);
			if (p->m_prevLineScore > 0.95) prevLineRank = "Perfect";
			else if (p->m_prevLineScore > 0.9) prevLineRank = "Excellent";
			else if (p->m_prevLineScore > 0.8) prevLineRank = "Great";
			else if (p->m_prevLineScore > 0.6) prevLineRank = "Good";
			else if (p->m_prevLineScore > 0.4) prevLineRank = "OK";
			m_line_rank_text[i%4]->render(prevLineRank);
			switch(position) {
				case LayoutSinger::FULL:
					m_line_rank_text[i%4]->dimensions().middle(-0.350 + 0.01 + 0.25 * j).fixedHeight(0.055*fzoom).screenTop(0.11);
					break;
				case LayoutSinger::TOP:
					m_line_rank_text[i%4]->dimensions().right(0.30).fixedHeight(0.05*fzoom).screenTop(0.025 + 0.050 * j);
					break;
				case LayoutSinger::BOTTOM:
					m_line_rank_text[i%4]->dimensions().right(0.30).fixedHeight(0.05*fzoom).center(0.025 + 0.050 * j);
					break;
				case LayoutSinger::LEFT:
				case LayoutSinger::RIGHT:
					m_line_rank_text[i%4]->dimensions().middle(-0.350 + 0.01 + 0.25 * j).fixedHeight(0.055*fzoom).screenTop(0.11);
					break;
			}
			{
				color.a = clamp(fact*2.0);
				ColorTrans c(color);
				m_line_rank_text[i%4]->draw();
			}
		}
		++j;
	}
}

void LayoutSinger::draw(double time, PositionMode position) {
	// Draw notes and pitch waves (only when not in karaoke mode)
	if (!config["game/karaoke_mode"].i()) {
		switch(position) {
			case LayoutSinger::FULL:
				m_noteGraph.draw(time, m_database, NoteGraph::FULLSCREEN);
				break;
			case LayoutSinger::TOP:
				m_noteGraph.draw(time, m_database, NoteGraph::TOP);
				break;
			case LayoutSinger::BOTTOM:
				m_noteGraph.draw(time, m_database, NoteGraph::BOTTOM);
				break;
			case LayoutSinger::LEFT:
			case LayoutSinger::RIGHT:
				m_noteGraph.draw(time, m_database, NoteGraph::LEFT);
				break;
		}
	}

	// Draw the lyrics
	if (!m_hideLyrics) {
		double linespacing = 0.0;
		Dimensions pos;
		switch(position) {
			case LayoutSinger::FULL:
				if(config["game/karaoke_mode"].i() >= 2) {
					pos.center(0);
				} else {
					pos.screenBottom(-0.07);
				}
				linespacing = 0.04;
				break;
			case LayoutSinger::TOP:
				pos.center(-0.06);
				linespacing = 0.04;
				break;
			case LayoutSinger::BOTTOM:
				pos.screenBottom(-0.07);
				linespacing = 0.04;
				break;
			case LayoutSinger::LEFT:
			case LayoutSinger::RIGHT:
				pos.screenBottom(-0.1);
				linespacing = 0.06;
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
			if (!dirty && m_lyricit != m_vocal.notes.end() && m_lyricit->begin < time + 4.0) {
				m_lyrics.push_back(LyricRow(m_lyricit, m_vocal.notes.end()));
				dirty = true;
			}
		} while (dirty);
		if (m_theme.get()) // if there is a theme, draw the lyrics with it
		{
			for (size_t i = 0; i < m_lyrics.size(); ++i, pos.move(0.0, linespacing)) {
				pos.move(0.0, m_lyrics[i].extraspacing.get() * linespacing);
				if (i == 0) m_lyrics[0].draw(m_theme->lyrics_now, time, pos);
				else if (i == 1) m_lyrics[1].draw(m_theme->lyrics_next, time, pos);
			}
		}
	}

	if (!config["game/karaoke_mode"].i() ) drawScore(position); // draw score if not in karaoke mode
}

double LayoutSinger::lyrics_begin() const {
	return m_lyricit->begin;
}

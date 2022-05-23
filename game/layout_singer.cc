#include "layout_singer.hh"

#include "configuration.hh"
#include "fs.hh"
#include "song.hh"
#include "database.hh"

#include <list>
#include <fmt/format.h>

LayoutSinger::LayoutSinger(VocalTrack& vocal, Database& database, NoteGraphScalerPtr const& scaler, std::shared_ptr<ThemeSing> theme):
  m_vocal(vocal), m_noteGraph(vocal, scaler), m_lyricit(vocal.notes.begin()), m_lyrics(), m_database(database), m_theme(theme), m_hideLyrics() {
	m_score_text[0] = std::make_unique<SvgTxtThemeSimple>(findFile("sing_score_text.svg"), config["graphic/text_lod"].f());
	m_score_text[1] = std::make_unique<SvgTxtThemeSimple>(findFile("sing_score_text.svg"), config["graphic/text_lod"].f());
	m_score_text[2] = std::make_unique<SvgTxtThemeSimple>(findFile("sing_score_text.svg"), config["graphic/text_lod"].f());
	m_score_text[3] = std::make_unique<SvgTxtThemeSimple>(findFile("sing_score_text.svg"), config["graphic/text_lod"].f());
	m_line_rank_text[0] = std::make_unique<SvgTxtThemeSimple>(findFile("sing_score_text.svg"), config["graphic/text_lod"].f());
	m_line_rank_text[1] = std::make_unique<SvgTxtThemeSimple>(findFile("sing_score_text.svg"), config["graphic/text_lod"].f());
	m_line_rank_text[2] = std::make_unique<SvgTxtThemeSimple>(findFile("sing_score_text.svg"), config["graphic/text_lod"].f());
	m_line_rank_text[3] = std::make_unique<SvgTxtThemeSimple>(findFile("sing_score_text.svg"), config["graphic/text_lod"].f());
	m_player_icon = std::make_unique<Texture>(findFile("sing_pbox.svg"));
}

LayoutSinger::~LayoutSinger() {}

void LayoutSinger::reset() {
	m_lyricit = m_vocal.notes.begin();
	m_lyrics.clear();
}

void LayoutSinger::drawScore(PositionMode position) {
	unsigned int i = 0;
    float j = 0.0f;
	for (std::list<Player>::const_iterator p = m_database.cur.begin(); p != m_database.cur.end(); ++p, ++i) {
		if (p->m_vocal.name != m_vocal.name) continue;
		Color color(p->m_color.r, p->m_color.g, p->m_color.b, p->activity());
		if (color.a == 0.0f) continue;
		m_score_text[i % 4]->render(fmt::format("{:04d}", p->getScore()));
		switch(position) {
			case LayoutSinger::PositionMode::FULL:
				m_player_icon->dimensions.left(-0.5f + 0.01f + 0.125f * j).fixedWidth(0.035f).screenTop(0.055f);
				if (m_database.cur.size() < 9){
					m_player_icon->dimensions.left(-0.5f + 0.01f + 0.125f * j).fixedWidth(0.035f).screenTop(0.05f);
					m_score_text[i%4]->dimensions().middle(-0.425f + 0.01f + 0.125f * j).fixedHeight(0.035f).screenTop(0.055f);}
				else{
					m_player_icon->dimensions.left(-0.506f + 0.01f + 0.0905f * j).fixedWidth(0.028f).screenTop(0.050f);
					m_score_text[i%4]->dimensions().middle(-0.519f + 0.08f + 0.0905f * j).fixedHeight(0.029f).screenTop(0.053f);}
				break;
			case LayoutSinger::PositionMode::TOP:
				m_player_icon->dimensions.right(0.35f).fixedHeight(0.050f).screenTop(0.025f + 0.050f * j);
				m_score_text[i%4]->dimensions().right(0.45f).fixedHeight(0.050f).screenTop(0.025f + 0.050f * j);
				break;
			case LayoutSinger::PositionMode::BOTTOM:
				m_player_icon->dimensions.right(0.35f).fixedHeight(0.050f).center(0.025f + 0.050f * j);
				m_score_text[i%4]->dimensions().right(0.45f).fixedHeight(0.050f).center(0.025f + 0.050f * j);
				break;
			case LayoutSinger::PositionMode::LEFT:
			case LayoutSinger::PositionMode::RIGHT:
				m_player_icon->dimensions.left(-0.5f + 0.01f + 0.25f * j).fixedWidth(0.075f).screenTop(0.055f);
				m_score_text[i%4]->dimensions().middle(-0.350f + 0.01f + 0.25f * j).fixedHeight(0.075f).screenTop(0.055f);
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
			if (p->m_prevLineScore > 0.95) prevLineRank = _("Perfect");
			else if (p->m_prevLineScore > 0.9) prevLineRank = _("Excellent");
			else if (p->m_prevLineScore > 0.8) prevLineRank = _("Great");
			else if (p->m_prevLineScore > 0.6) prevLineRank = _("Good");
			else if (p->m_prevLineScore > 0.4) prevLineRank = _("OK");
			m_line_rank_text[i%4]->render(prevLineRank);
			switch(position) {
				case LayoutSinger::PositionMode::FULL:
					m_line_rank_text[i%4]->dimensions().middle(-0.350f + 0.01f + 0.25f * j).fixedHeight(static_cast<float>(0.055*fzoom)).screenTop(0.11f);
					break;
				case LayoutSinger::PositionMode::TOP:
					m_line_rank_text[i%4]->dimensions().right(0.30f).fixedHeight(static_cast<float>(0.05*fzoom)).screenTop(0.025f + 0.050f * j);
					break;
				case LayoutSinger::PositionMode::BOTTOM:
					m_line_rank_text[i%4]->dimensions().right(0.30f).fixedHeight(static_cast<float>(0.05*fzoom)).center(0.025f + 0.050f * j);
					break;
				case LayoutSinger::PositionMode::LEFT:
				case LayoutSinger::PositionMode::RIGHT:
					m_line_rank_text[i%4]->dimensions().middle(-0.350f + 0.01f + 0.25f * j).fixedHeight(static_cast<float>(0.055*fzoom)).screenTop(0.11f);
					break;
			}
			{
				color.a = static_cast<float>(clamp(fact*2.0));
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
			case LayoutSinger::PositionMode::FULL:
				m_noteGraph.draw(time, m_database, NoteGraph::Position::FULLSCREEN);
				break;
			case LayoutSinger::PositionMode::TOP:
				m_noteGraph.draw(time, m_database, NoteGraph::Position::TOP);
				break;
			case LayoutSinger::PositionMode::BOTTOM:
				m_noteGraph.draw(time, m_database, NoteGraph::Position::BOTTOM);
				break;
			case LayoutSinger::PositionMode::LEFT:
			case LayoutSinger::PositionMode::RIGHT:
				m_noteGraph.draw(time, m_database, NoteGraph::Position::LEFT);
				break;
		}
	}

	// Draw the lyrics
	if (!m_hideLyrics) {
		float linespacing = 0.0f;
		Dimensions pos;
		switch(position) {
			case LayoutSinger::PositionMode::FULL:
				if(config["game/karaoke_mode"].i() >= 2) {
					pos.center(0.0f);
				} else {
					pos.screenBottom(-0.07f);
				}
				linespacing = 0.04f;
				break;
			case LayoutSinger::PositionMode::TOP:
				pos.center(-0.06f);
				linespacing = 0.04f;
				break;
			case LayoutSinger::PositionMode::BOTTOM:
				pos.screenBottom(-0.07f);
				linespacing = 0.04f;
				break;
			case LayoutSinger::PositionMode::LEFT:
			case LayoutSinger::PositionMode::RIGHT:
				pos.screenBottom(-0.1f);
				linespacing = 0.06f;
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
			for (size_t i = 0; i < m_lyrics.size(); ++i, pos.move(0.0f, linespacing)) {
				pos.move(0.0f, static_cast<float>(m_lyrics[i].extraspacing.get() * linespacing));
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

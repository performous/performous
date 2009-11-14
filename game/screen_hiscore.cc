#include "screen_hiscore.hh"

#include "configuration.hh"
#include "audio.hh"
#include "fs.hh"
#include "util.hh"
#include "database.hh"

#include <iostream>
#include <sstream>
#include <boost/format.hpp>

ScreenHiscore::ScreenHiscore(std::string const& name, Audio& audio, Songs& songs, Database& database):
  ScreenSongs(name, audio, songs), m_database(database), m_players(database.m_players)
{
	m_players.setAnimMargins(5.0, 5.0);
}

void ScreenHiscore::enter() {
	m_score_text[0].reset(new SvgTxtThemeSimple(getThemePath("sing_score_text.svg"), config["graphic/text_lod"].f()));
	m_score_text[1].reset(new SvgTxtThemeSimple(getThemePath("sing_score_text.svg"), config["graphic/text_lod"].f()));
	m_score_text[2].reset(new SvgTxtThemeSimple(getThemePath("sing_score_text.svg"), config["graphic/text_lod"].f()));
	m_score_text[3].reset(new SvgTxtThemeSimple(getThemePath("sing_score_text.svg"), config["graphic/text_lod"].f()));
	m_player_icon.reset(new Surface(getThemePath("sing_pbox.svg")));

	// m_emptyCover.reset(new Surface(getThemePath("no_person.svg"))); // TODO use persons head

	m_search.text.clear();
	m_players.setFilter(m_search.text);

	ScreenSongs::enter();
}

void ScreenHiscore::exit() {
	m_player_icon.reset();
	m_score_text[0].reset();
	m_score_text[1].reset();
	m_score_text[2].reset();
	m_score_text[3].reset();

	ScreenSongs::exit();
}

void ScreenHiscore::activateNextScreen() {
	ScreenManager* sm = ScreenManager::getSingletonPtr();
	sm->activateScreen("Songs");
}

void ScreenHiscore::manageEvent(SDL_Event event) {
	if (event.type != SDL_KEYDOWN) return;
	SDL_keysym keysym = event.key.keysym;
	int key = keysym.sym;
	SDLMod mod = event.key.keysym.mod;

	// TODO: reload button? - needs database reload
	// if (key == SDLK_r && mod & KMOD_CTRL) { m_players.reload(); m_players.setFilter(m_search.text); }
	if (m_search.process(keysym)) m_players.setFilter(m_search.text);
	else if (key == SDLK_ESCAPE) {
		if (m_search.text.empty()) { activateNextScreen(); return; }
	else { m_search.text.clear(); m_players.setFilter(m_search.text); }
	}
	// The rest are only available when there are songs available
	else if (m_players.empty()) return;
	else if (key == SDLK_SPACE || (key == SDLK_PAUSE || (key == SDLK_p && mod & KMOD_CTRL))) m_audio.togglePause();
	else if (key == SDLK_RETURN) { activateNextScreen(); return; }
	else if (key == SDLK_LEFT) m_players.advance(-1);
	else if (key == SDLK_RIGHT) m_players.advance(1);
	else if (key == SDLK_PAGEUP) m_players.advance(-10);
	else if (key == SDLK_PAGEDOWN) m_players.advance(10);
}

/**Draw the scores in the bottom*/
void ScreenHiscore::drawScores() {
	// Score display
	{
		unsigned int i = 0;
		for (std::list<Player>::const_iterator p = m_database.cur.begin(); p != m_database.cur.end(); ++p, ++i) {
			float act = p->activity();
			if (act == 0.0f) continue;
			glColor4f(p->m_color.r, p->m_color.g, p->m_color.b,act);
			m_player_icon->dimensions.left(-0.5 + 0.01 + 0.25 * i).fixedWidth(0.075)
				.screenBottom(-0.025);
			m_player_icon->draw();
			m_score_text[i%4]->render((boost::format("%04d") % p->getScore()).str());
			m_score_text[i%4]->dimensions().middle(-0.350 + 0.01 + 0.25 * i).fixedHeight(0.075)
				.screenBottom(-0.025);
			m_score_text[i%4]->draw();
			glColor4f(1.0, 1.0, 1.0, 1.0);
		}
	}
}

void ScreenHiscore::draw() {
	ScreenSharedInfo info;
	info.videoGap = 0.0;

	ScreenSongs::drawMultimedia();
	ScreenSongs::updateMultimedia(*m_song, info);

	std::ostringstream oss_song, oss_order;

	// Format the player information text
	oss_song << "Hiscore for " << m_song->title << "\n";

	m_database.queryPerSongHiscore(oss_order, m_song);

	// Draw song and order texts
	theme->song.draw(oss_song.str());
	theme->order.draw(oss_order.str());

	ScreenSongs::stopMultimedia(info);
}


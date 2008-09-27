#include "screen_score.hh"
#include "xtime.hh"
#include <boost/lexical_cast.hpp>
#include <limits>

void CScreenScore::enter() {
  	theme.reset(new CThemeScore());
	m_time = seconds(now());
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	m_text.reset(new SvgTxtTheme(sm->getThemePathFile("score_txt.svg"),SvgTxtTheme::CENTER));
	m_rank.reset(new SvgTxtTheme(sm->getThemePathFile("score_rank.svg"),SvgTxtTheme::CENTER));
}

void CScreenScore::exit() {
	theme.reset();
	m_text.reset();
	m_rank.reset();
}

void CScreenScore::manageEvent(SDL_Event event) {
	if (event.type != SDL_KEYDOWN) return;
	if (event.key.keysym.sym == SDLK_PAUSE) m_time = std::numeric_limits<double>::infinity();
	else CScreenManager::getSingletonPtr()->activateScreen("Songs");
}

void CScreenScore::draw() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	if (seconds(now()) - m_time > 10.0) { sm->activateScreen("Songs"); return; }
	Song& song = sm->getSongs()->current();
	theme->bg->draw();
	// Draw some numbers
	int score = 0; //FIXME: song.getScore();
	char const* rank;
	if (score > 8000) rank = "Hit singer";
	else if (score > 6000) rank = "Lead singer";
	else if (score > 4000) rank = "Rising star";
	else if (score > 2000) rank = "Amateur";
	else rank = "Tone deaf";
	double scorePercent = score / 10000.0;
	m_rank->draw(rank);
	m_text->draw(boost::lexical_cast<std::string>(score));
}

#include <screen_score.h>
#include <xtime.h>
#include <boost/lexical_cast.hpp>
#include <limits>

CScreenScore::CScreenScore(std::string const& name, unsigned int width, unsigned int height):
  CScreen(name, width, height)
{}

CScreenScore::~CScreenScore() {}

void CScreenScore::enter() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
  	theme.reset(new CThemeScore(m_width, m_height));
	//bg_texture = sm->getVideoDriver()->initSurface(theme->bg->getSDLSurface());
	m_time = seconds(now());
}

void CScreenScore::exit() {
	theme.reset();
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
	theme->theme->clear();
	theme->bg->draw(0.5,0.5,0.5+m_width,0.5+m_height);
	// Draw some numbers
	int score = song.getScore();
	char const* rank;
	if (score > 8000) rank = "Hit singer";
	else if (score > 6000) rank = "Lead singer";
	else if (score > 4000) rank = "Rising star";
	else if (score > 2000) rank = "Amateur";
	else rank = "Tone deaf";
	double oldY = theme->level.y;
	double scorePercent = score / 10000.0;
	theme->level.y = theme->level.y + theme->level.final_height * (1.-scorePercent);
	theme->level.height = theme->level.final_height* scorePercent;
	theme->rank.text = rank;
	theme->theme->PrintText(&theme->normal_score);
	theme->theme->PrintText(&theme->rank);
	theme->theme->DrawRect(theme->level);
	theme->level.y = oldY;
	theme->normal_score.text = boost::lexical_cast<std::string>(score);
	Surface(theme->theme->getCurrent()).draw(0, 0, 0);
}

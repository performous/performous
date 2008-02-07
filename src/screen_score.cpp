#include <screen_score.h>
#include <xtime.h>
#include <limits>

CScreenScore::CScreenScore(std::string const& name, unsigned int width, unsigned int height):
  CScreen(name, width, height)
{}

CScreenScore::~CScreenScore() {}

void CScreenScore::enter() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
  	theme.reset(new CThemeScore(m_width, m_height));
	bg_texture = sm->getVideoDriver()->initSurface(theme->bg->getSDLSurface());
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
	// Draw some numbers
	int score = song.getScore();
	char scoreStr[32];
	char rankStr[32];
	float scorePercent;
	sprintf(scoreStr,"%4d",score);
	theme->normal_score.text = scoreStr;			
	if (score < 2000) sprintf(rankStr,"Tone deaf");
	else if (score < 4000) sprintf(rankStr,"Amateur");
	else if (score < 6000) sprintf(rankStr,"Rising star");
	else if (score < 8000) sprintf(rankStr,"Lead singer");
	else sprintf(rankStr,"Hit singer");
	double oldY = theme->level.y;
	scorePercent = score/10000.;
	theme->level.y = theme->level.y + theme->level.final_height * (1.-scorePercent);
	theme->level.height = theme->level.final_height* scorePercent;
	theme->rank.text = rankStr;
	theme->theme->PrintText(&theme->normal_score);
	theme->theme->PrintText(&theme->rank);
	theme->theme->DrawRect(theme->level);
	theme->level.y = oldY;
	sm->getVideoDriver()->drawSurface(bg_texture);
	sm->getVideoDriver()->drawSurface(theme->theme->getCurrent());
}

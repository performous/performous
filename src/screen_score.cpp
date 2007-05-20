#include <screen_score.h>

CScreenScore::CScreenScore(char * name)
{
	screenName = name;
}

CScreenScore::~CScreenScore()
{
}

void CScreenScore::enter( void )
{
	CScreenManager * sm = CScreenManager::getSingletonPtr();

	char * theme_path = new char[1024];
  	theme = new CThemeScore();
	bg_texture = sm->getVideoDriver()->initSurface(theme->bg->getSDLSurface());
}

void CScreenScore::exit( void )
{
	delete theme;
}

void CScreenScore::manageEvent( SDL_Event event )
{
	int keypressed;
	switch(event.type) {
		case SDL_KEYDOWN:
			CScreenManager::getSingletonPtr()->activateScreen("Songs");
			break;
	}
}

void CScreenScore::draw( void )
{
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	CSong * song = sm->getSong();

	theme->theme->clear();

	// Draw some numbers
	{
		if (song != NULL) {
			int score = song->score[0].score/10*10;
			char scoreStr[32];
			char rankStr[32];
			sprintf(scoreStr,"%4d",int(score));
			theme->normal_score.text = scoreStr;
			
			if (score < 2000)
				sprintf(rankStr,"Tone deaf");
			else if (score < 4000)
				sprintf(rankStr,"Amateur");
			else if (score < 6000)
				sprintf(rankStr,"Rising star");
			else if (score < 8000)
				sprintf(rankStr,"Lead singer");
			else
				sprintf(rankStr,"Hit singer");

			theme->rank.text = rankStr;
			
			theme->theme->PrintText(&theme->normal_score);
			theme->theme->PrintText(&theme->rank);
		}
	}

	sm->getVideoDriver()->drawSurface(bg_texture);
	sm->getVideoDriver()->drawSurface(theme->theme->getCurrent());
}

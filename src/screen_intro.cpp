#include "screen_intro.hh"

CScreenIntro::CScreenIntro(std::string const& name): CScreen(name) {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	background.reset(new Surface(sm->getThemePathFile("intro.svg"),Surface::SVG));
}

void CScreenIntro::enter() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	sm->getAudio()->playMusic(sm->getThemePathFile("menu.ogg"));
}

void CScreenIntro::exit() {}

void CScreenIntro::manageEvent(SDL_Event event) {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	if (event.type == SDL_KEYDOWN) {
		int key = event.key.keysym.sym;
		if (key == SDLK_ESCAPE || key == SDLK_q) sm->finished();
		else if (key == SDLK_s) sm->activateScreen("Songs");
		else if (key == SDLK_c) sm->activateScreen("Configuration");
		else if (key == SDLK_p) sm->activateScreen("Practice");
		else if (key == SDLK_SPACE || key == SDLK_PAUSE) sm->getAudio()->togglePause();
	}
}

void CScreenIntro::draw() {
	background->draw();
}


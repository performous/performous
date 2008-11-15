#include "screen_intro.hh"

CScreenIntro::CScreenIntro(std::string const& name, Audio& audio): CScreen(name), m_audio(audio) {}

void CScreenIntro::enter() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	m_audio.playMusic(sm->getThemePathFile("menu.ogg"));
	background.reset(new Surface(sm->getThemePathFile("intro.svg")));
}

void CScreenIntro::exit() {
	background.reset();
}

void CScreenIntro::manageEvent(SDL_Event event) {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	if (event.type == SDL_KEYDOWN) {
		int key = event.key.keysym.sym;
		if (key == SDLK_ESCAPE || key == SDLK_q) sm->finished();
		else if (key == SDLK_s) sm->activateScreen("Songs");
		else if (key == SDLK_c) sm->activateScreen("Configuration");
		else if (key == SDLK_p) sm->activateScreen("Practice");
		else if (key == SDLK_SPACE || key == SDLK_PAUSE) m_audio.togglePause();
	}
}

void CScreenIntro::draw() {
	background->draw();
}


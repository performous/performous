#include "screen_intro.hh"

CScreenIntro::CScreenIntro(std::string const& name, Audio& audio, Capture& capture): CScreen(name), m_audio(audio), m_capture(capture) {}

void CScreenIntro::enter() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	m_audio.playMusic(sm->getThemePathFile("menu.ogg"));
	background.reset(new Surface(sm->getThemePathFile("intro.svg")));
	std::string msg;
	if (!m_audio.isOpen()) msg = "No playback devices could be used.\n";
	if (m_capture.analyzers().empty()) msg += "No microphones found.\n";
	if (!msg.empty()) m_dialog.reset(new Dialog(msg + "\nPlease configure some before playing."));
}

void CScreenIntro::exit() {
	background.reset();
	m_dialog.reset();
}

void CScreenIntro::manageEvent(SDL_Event event) {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	if (event.type == SDL_KEYDOWN) {
		if (m_dialog) { m_dialog.reset(); return; }
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
	if (m_dialog) m_dialog->draw();
}


#include "screen_intro.hh"

#include "fs.hh"
#include "audio.hh"
#include "record.hh"

ScreenIntro::ScreenIntro(std::string const& name, Audio& audio, Capture& capture): Screen(name), m_audio(audio), m_capture(capture) {}

void ScreenIntro::enter() {
	m_audio.playMusic(getThemePath("menu.ogg"), true);
	//background.reset(new Surface(getThemePath("intro.svg")));
	theme.reset(new ThemeIntro());
	std::string msg;
	if (!m_audio.isOpen()) msg = "No playback devices could be used.\n";
	if (m_capture.analyzers().empty()) msg += "No microphones found.\n";
	if (!msg.empty()) m_dialog.reset(new Dialog(msg + "\nPlease configure some before playing."));
}

void ScreenIntro::exit() {
	theme.reset();
	m_dialog.reset();
}

void ScreenIntro::manageEvent(SDL_Event event) {
	ScreenManager* sm = ScreenManager::getSingletonPtr();
	if (event.type == SDL_KEYDOWN) {
		if (m_dialog) { m_dialog.reset(); return; }
		int key = event.key.keysym.sym;
		if (key == SDLK_ESCAPE || key == SDLK_q) sm->finished();
		
		/*
		 * Here im writing my code, loading the images in the theme
		 * "sing", "configure", etc. just using SDLK_DOWN and SDLK_UP
		 * my question is, how can i load those images? do i have to use
		 * something like "theme->sing.draw()"? because i think that is
		 * not working
		 * */
		
		else if (key == SDLK_DOWN) {
			theme->sing.draw();
		}
		else if (key == SDLK_DOWN) {
			theme->configure.draw();
		}
			
		
		else if (key == SDLK_s) sm->activateScreen("Songs");
		else if (key == SDLK_c) sm->activateScreen("Configuration");
		else if (key == SDLK_p) sm->activateScreen("Practice");
		else if (key == SDLK_SPACE || key == SDLK_PAUSE) m_audio.togglePause();
	} else if (event.type == SDL_JOYBUTTONDOWN) {
		int button = event.jbutton.button;
		if (button == 9) sm->activateScreen("Songs");
		else if (button == 8) sm->finished();
	}
}

void ScreenIntro::draw() {
	theme->bg.draw();
	//background->draw();
	if (m_dialog) m_dialog->draw();
}


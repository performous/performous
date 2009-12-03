#include "screen_intro.hh"

#include "fs.hh"
#include "audio.hh"
#include "record.hh"

ScreenIntro::ScreenIntro(std::string const& name, Audio& audio, Capture& capture): Screen(name), m_audio(audio), m_capture(capture), selected() {
	m_menuOptions.push_back(new MenuOption("Perform", "Songs", "intro_sing.svg", "Search songs database and start performing!"));
	m_menuOptions.push_back(new MenuOption("Practice", "Practice", "intro_practice.svg", "Check your skills or test the microphones"));
	m_menuOptions.push_back(new MenuOption("Configure", "Configuration", "intro_configure.svg", "Configure game options"));
	m_menuOptions.push_back(new MenuOption("Quit", "", "intro_quit.svg", "Leave the game"));
}

void ScreenIntro::enter() {
	m_audio.playMusic(getThemePath("menu.ogg"), true);
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
		else if (key == SDLK_DOWN) ++selected;
		else if (key == SDLK_UP) --selected;
		else if (key == SDLK_RETURN) {
			std::string screen = m_menuOptions[selected].screen;
			if (screen.empty()) sm->finished(); else sm->activateScreen(screen);
		} else if (key == SDLK_SPACE || key == SDLK_PAUSE) m_audio.togglePause();
		// Normalize selected to [0, size)
		selected = (m_menuOptions.size() + selected) % m_menuOptions.size();
	} else if (event.type == SDL_JOYBUTTONDOWN) {
		int button = event.jbutton.button;
		if (button == 9) sm->activateScreen("Songs");
		else if (button == 8) sm->finished();
	}
}

void ScreenIntro::draw_menu_options() {
	for (unsigned i = 0; i < m_menuOptions.size(); i++) {
		if (i == selected) {
			theme->back_h.dimensions.left(-0.4).center(-0.1 + i*0.08);
			theme->back_h.draw();
			theme->option.dimensions.left(-0.35).center(-0.1 + i*0.08);
			theme->option.draw(m_menuOptions[i].name);
		} else {
			theme->option.dimensions.left(-0.35).center(-0.1 + i*0.08);
			theme->option.draw(m_menuOptions[i].name);
		}
	}
}

void ScreenIntro::draw() {
	theme->bg.draw();
	m_menuOptions[selected].image.draw();
	theme->comment.dimensions.left(-0.45).center(0.336);
	theme->comment.draw(m_menuOptions[selected].comment);
	draw_menu_options();
	if (m_dialog) m_dialog->draw();
}

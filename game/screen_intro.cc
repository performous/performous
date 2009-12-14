#include "screen_intro.hh"

#include "fs.hh"
#include "audio.hh"
#include "record.hh"
#include "i18n.hh"
#include "joystick.hh"

ScreenIntro::ScreenIntro(std::string const& name, Audio& audio, Capture& capture): Screen(name), m_audio(audio), m_capture(capture), selected(), m_first(true) {
	m_menuOptions.push_back(new MenuOption(_("Perform"), "Songs", "intro_sing.svg", _("Start performing!")));
	m_menuOptions.push_back(new MenuOption(_("Practice"), "Practice", "intro_practice.svg", _("Check your skills or test the microphones")));
	m_menuOptions.push_back(new MenuOption(_("Configure"), "Configuration", "intro_configure.svg", _("Configure game options")));
	m_menuOptions.push_back(new MenuOption(_("Quit"), "", "intro_quit.svg", _("Leave the game")));
}

void ScreenIntro::enter() {
	m_audio.playMusic(getThemePath("menu.ogg"), true);
	theme.reset(new ThemeIntro());
	if( m_first ) {
		std::string msg;
		if (!m_audio.isOpen()) msg = _("No playback devices could be used.\n");
		if (m_capture.analyzers().empty()) msg += _("No microphones found.\n");
		if (!msg.empty()) m_dialog.reset(new Dialog(msg + _("\nPlease configure some before playing.")));
		m_first = false;
	} else {
		m_dialog.reset();
	}
}

void ScreenIntro::exit() {
	theme.reset();
	m_dialog.reset();
}

void ScreenIntro::manageEvent(SDL_Event event) {
	ScreenManager* sm = ScreenManager::getSingletonPtr();
	input::NavButton nav(input::getNav(event));
	if (nav != input::NONE) {
		if (m_dialog) { m_dialog.reset(); return; }
		if (nav == input::CANCEL) sm->finished();
		else if (nav == input::DOWN || nav == input::RIGHT) ++selected;
		else if (nav == input::UP || nav == input::LEFT) --selected;
		else if (nav == input::START) {
			std::string screen = m_menuOptions[selected].screen;
			if (screen.empty()) sm->finished(); else sm->activateScreen(screen);
		} else if (nav == input::PAUSE) m_audio.togglePause();
		// Normalize selected to [0, size)
		selected = (m_menuOptions.size() + selected) % m_menuOptions.size();
	}
}

void ScreenIntro::draw_menu_options() {
	for (unsigned i = 0; i < m_menuOptions.size(); i++) {
		if (i == selected) {
			theme->back_h.dimensions.left(-0.4).center(-0.097 + i*0.08);
			theme->back_h.draw();
			theme->option_selected.dimensions.left(-0.35).center(-0.1 + i*0.08);
			theme->option_selected.draw(m_menuOptions[i].name);
		} else {
			theme->option[i].dimensions.left(-0.35).center(-0.1 + i*0.08);
			theme->option[i].draw(m_menuOptions[i].name);
		}
	}
}

void ScreenIntro::draw() {
	theme->bg.draw();
	m_menuOptions[selected].image.draw();
	theme->comment_bg.dimensions.center().screenBottom(-0.01);
	theme->comment_bg.draw();
	theme->comment.dimensions.left(-0.45).screenBottom(-0.028);
	theme->comment.draw(m_menuOptions[selected].comment);
	draw_menu_options();
	if (m_dialog) m_dialog->draw();
}

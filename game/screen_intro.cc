#include "screen_intro.hh"

#include "fs.hh"
#include "audio.hh"
#include "i18n.hh"
#include "joystick.hh"
#include "theme.hh"
#include "menu.hh"

ScreenIntro::ScreenIntro(std::string const& name, Audio& audio): Screen(name), m_audio(audio), m_first(true) {
}

void ScreenIntro::enter() {
	m_audio.playMusic(getThemePath("menu.ogg"), true);
	theme.reset(new ThemeIntro());
	m_menu.clear();
	m_menu.add(MenuOption(_("Perform"), _("Start performing!"), "Songs", "intro_sing.svg"));
	m_menu.add(MenuOption(_("Practice"), _("Check your skills or test the microphones"), "Practice", "intro_practice.svg"));
	{	// Config submenu
		MenuOptions submenu;
		submenu.push_back(MenuOption(_("Audio Devices"), _("Setup microphones and playback"), "AudioDevices", "intro_configure.svg"));
		submenu.push_back(MenuOption(_("Configure"), _("Configure game options"), "Configuration", "intro_configure.svg"));
		submenu.push_back(MenuOption(_("Back"), _("Got back to main menu")));
		submenu.back().image.reset(new Surface(getThemePath("intro_quit.svg")));
		m_menu.add(MenuOption(_("Configure"), _("Configure audio and game options"), submenu));
		m_menu.back().image.reset(new Surface(getThemePath("intro_configure.svg")));
	} // Uncomment below (comment above) to hide audiodevices menu
	//m_menu.add(MenuOption(_("Configure"), _("Configure game options"), "Configuration", "intro_configure.svg"));
	m_menu.add(MenuOption(_("Quit"), _("Leave the game"), "", "intro_quit.svg"));
	if( m_first ) {
		std::string msg;
		if (!m_audio.isOpen()) msg = _("No playback devices could be used.\n");
		if (!msg.empty()) m_dialog.reset(new Dialog(msg + _("\nPlease configure some before playing.")));
		m_first = false;
	} else {
		m_dialog.reset();
	}
}

void ScreenIntro::exit() {
	m_menu.clear();
	theme.reset();
	m_dialog.reset();
}

void ScreenIntro::manageEvent(SDL_Event event) {
	input::NavButton nav(input::getNav(event));
	if (nav != input::NONE) {
		if (m_dialog) { m_dialog.reset(); return; }
		if (nav == input::CANCEL) m_menu.moveToLast();  // Move cursor to quit
		else if (nav == input::DOWN || nav == input::RIGHT || nav == input::MOREDOWN) m_menu.move(1);
		else if (nav == input::UP || nav == input::LEFT || nav == input::MOREUP) m_menu.move(-1);
		else if (nav == input::START) m_menu.action();
		else if (nav == input::PAUSE) m_audio.togglePause();
	}
}

void ScreenIntro::draw_menu_options() {
	int i = 0;
	double wcounter = 0;
	const float x = -0.35;
	const float sel_margin = 0.05;
	theme->back_h.dimensions.stretch(m_menu.dimensions.w(), theme->back_h.dimensions.h());
	for (MenuOptions::const_iterator it = m_menu.begin(); it != m_menu.end(); ++it, ++i) {
		if (static_cast<MenuOptions::const_iterator>(&m_menu.current()) == it) {
			theme->back_h.dimensions.left(x - sel_margin).center(-0.097 + i*0.08);
			theme->back_h.draw();
			theme->option_selected.dimensions.left(x).center(-0.1 + i*0.08);
			theme->option_selected.draw(it->getName());
			wcounter = std::max(wcounter, theme->option_selected.w() + 2 * sel_margin); // Calculate the widest entry
		} else {
			theme->option.dimensions.left(x).center(-0.1 + i*0.08);
			theme->option.draw(it->getName());
			wcounter = std::max(wcounter, theme->option.w() + 2 * sel_margin); // Calculate the widest entry
		}
	}
	m_menu.dimensions.stretch(wcounter, 1);
}

void ScreenIntro::draw() {
	theme->bg.draw();
	m_menu.current().image->draw();
	theme->comment_bg.dimensions.center().screenBottom(-0.01);
	theme->comment_bg.draw();
	theme->comment.dimensions.left(-0.48).screenBottom(-0.028);
	theme->comment.draw(m_menu.current().getComment());
	draw_menu_options();
	if (m_dialog) m_dialog->draw();
}

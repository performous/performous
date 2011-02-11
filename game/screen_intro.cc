#include "screen_intro.hh"

#include "fs.hh"
#include "glmath.hh"
#include "audio.hh"
#include "i18n.hh"
#include "joystick.hh"
#include "theme.hh"
#include "menu.hh"
#include "xtime.hh"


ScreenIntro::ScreenIntro(std::string const& name, Audio& audio): Screen(name), m_audio(audio), m_first(true) {
}

void ScreenIntro::enter() {
	m_audio.playMusic(getThemePath("menu.ogg"), true);
	m_selAnim = AnimValue(0.0, 10.0);
	m_submenuAnim = AnimValue(0.0, 3.0);
	theme.reset(new ThemeIntro());
	populateMenu();
	if( m_first ) {
		std::string msg;
		if (!m_audio.hasPlayback()) msg = _("No playback devices could be used.\n");
		if (!msg.empty()) ScreenManager::getSingletonPtr()->dialog(msg + _("\nPlease configure some before playing."));
		m_first = false;
	}
}

void ScreenIntro::exit() {
	m_menu.clear();
	theme.reset();
}

void ScreenIntro::manageEvent(SDL_Event event) {
	input::NavButton nav(input::getNav(event));
	if (nav != input::NONE) {
		if (nav == input::CANCEL) {
			if (m_menu.getSubmenuLevel() == 0) m_menu.moveToLast();  // Move cursor to quit in main menu
			else m_menu.closeSubmenu(); // One menu level up
		}
		else if (nav == input::DOWN || nav == input::MOREDOWN) m_menu.move(1);
		else if (nav == input::UP || nav == input::MOREUP) m_menu.move(-1);
		else if (nav == input::RIGHT && m_menu.getSubmenuLevel() >= 2) m_menu.action(1); // Config menu
		else if (nav == input::LEFT && m_menu.getSubmenuLevel() >= 2) m_menu.action(-1); // Config menu
		else if (nav == input::RIGHT && m_menu.getSubmenuLevel() < 2) m_menu.move(1); // Instrument nav hack
		else if (nav == input::LEFT && m_menu.getSubmenuLevel() < 2) m_menu.move(-1); // Instrument nav hack
		else if (nav == input::START) m_menu.action();
		else if (nav == input::PAUSE) m_audio.togglePause();
	} else if (event.type == SDL_KEYDOWN && m_menu.getSubmenuLevel() > 0) {
		// These are only available in config menu
		int key = event.key.keysym.sym;
		SDLMod modifier = event.key.keysym.mod;
		if (key == SDLK_r && modifier & KMOD_CTRL && m_menu.current().value) {
			m_menu.current().value->reset(modifier & KMOD_ALT);
		} else if (key == SDLK_s && modifier & KMOD_CTRL) {
			writeConfig(modifier & KMOD_ALT);
			ScreenManager::getSingletonPtr()->flashMessage((modifier & KMOD_ALT)
				? _("Settings saved as system defaults.") : _("Settings saved."));
		}
	}
	// Animation targets
	m_selAnim.setTarget(m_menu.curIndex());
	m_submenuAnim.setTarget(m_menu.getSubmenuLevel());
}

void ScreenIntro::draw_menu_options() {
	// Variables used for positioning and other stuff
	double wcounter = 0;
	const size_t showopts = 4; // Show at most 4 options simultaneously
	const float x = -0.35;
	const float start_y = -0.1;
	const float sel_margin = 0.05;
	const MenuOptions opts = m_menu.getOptions();
	double submenuanim = 1.0 - std::min(1.0, std::abs(m_submenuAnim.get()-m_menu.getSubmenuLevel()));
	theme->back_h.dimensions.stretch(m_menu.dimensions.w(), theme->back_h.dimensions.h());
	// Determine from which item to start
	int start_i = std::min((int)m_menu.curIndex() - 1, (int)opts.size() - (int)showopts
		+ (m_menu.getSubmenuLevel() == 2 ? 1 : 0)); // Hack to counter side-effects from displaying the value inside the menu
	if (start_i < 0 || opts.size() == showopts) start_i = 0;

	// Loop the currently visible options
	for (size_t i = start_i, ii = 0; ii < showopts && i < opts.size(); ++i, ++ii) {
		MenuOption const& opt = opts[i];

		// Selection
		if (i == m_menu.curIndex()) {
			// Animate selection higlight moving
			double selanim = m_selAnim.get() - start_i;
			if (selanim < 0) selanim = 0;
			theme->back_h.dimensions.left(x - sel_margin).center(start_y+0.003 + selanim*0.08);
			theme->back_h.draw();
			// Draw the text, dim if option not available
			theme->option_selected.dimensions.left(x).center(start_y + ii*0.08);
			theme->option_selected.draw(opt.getName(), submenuanim * (opt.isActive() ? 1.0f : 0.5f));
			wcounter = std::max(wcounter, theme->option_selected.w() + 2 * sel_margin); // Calculate the widest entry
			// If this is a config item, show the value below
			if (opt.type == MenuOption::CHANGE_VALUE) {
				++ii; // Use a slot for the value
				theme->option_selected.dimensions.left(x + sel_margin).center(-0.1 + (selanim+1)*0.08);
				theme->option_selected.draw("<  " + opt.value->getValue() + "  >", submenuanim);
			}

		// Regular option (not selected)
		} else {
			std::string title = opt.getName();
			SvgTxtTheme& txt = getTextObject(title);
			txt.dimensions.left(x).center(start_y + ii*0.08);
			txt.draw(title, submenuanim * (opt.isActive() ? 1.0f : 0.5f));
			wcounter = std::max(wcounter, txt.w() + 2 * sel_margin); // Calculate the widest entry
		}
	}
	m_menu.dimensions.stretch(wcounter, 1);
}

void ScreenIntro::draw() {
	{
		float anim = SDL_GetTicks() % 20000 / 20000.0;
		glutil::Color c(glmath::rotate(2.0 * M_PI * anim, glmath::Vec3(1.0, 1.0, 1.0)));
		theme->bg.draw();
	}
	if (m_menu.current().image) m_menu.current().image->draw();
	// Comment
	theme->comment_bg.dimensions.center().screenBottom(-0.01);
	theme->comment_bg.draw();
	theme->comment.dimensions.left(-0.48).screenBottom(-0.028);
	theme->comment.draw(m_menu.current().getComment());
	// Key help for config
	if (m_menu.getSubmenuLevel() > 0) {
		theme->short_comment_bg.dimensions.stretch(theme->short_comment.w() + 0.08, 0.025);
		theme->short_comment_bg.dimensions.left(-0.54).screenBottom(-0.054);
		theme->short_comment_bg.draw();
		theme->short_comment.dimensions.left(-0.48).screenBottom(-0.067);
		theme->short_comment.draw(_("Ctrl + S to save, Ctrl + R to reset defaults"));
	}
	// Menu
	draw_menu_options();
}

SvgTxtTheme& ScreenIntro::getTextObject(std::string const& txt) {
	if (theme->options.contains(txt)) return theme->options[txt];
	return *theme->options.insert(txt, new SvgTxtTheme(getThemePath("mainmenu_option.svg"), config["graphic/text_lod"].f()))->second;
}

void ScreenIntro::populateMenu() {
	m_menu.clear();
	boost::shared_ptr<Surface> config_bg(new Surface(getThemePath("intro_configure.svg")));
	m_menu.add(MenuOption(_("Perform"), _("Start performing!"), "Songs", "intro_sing.svg"));
	m_menu.add(MenuOption(_("Practice"), _("Check your skills or test the microphones"), "Practice", "intro_practice.svg"));

	{	// Config submenu
		MenuOptions audiomenu;
		MenuOptions gfxmenu;
		MenuOptions gamemenu;
		// Populate the submenus
		for (Config::iterator it = config.begin(); it != config.end(); ++it) {
			// Skip items that are configured elsewhere
			if (it->first == "audio/devices" || it->first.find("paths/") != std::string::npos) continue;
			MenuOptions* opts = &gamemenu; // Default to game menu
			if (it->first.find("audio/") != std::string::npos) opts = &audiomenu;
			else if (it->first.find("graphic/") != std::string::npos) opts = &gfxmenu;
			// Push the ConfigItem to the submenu
			opts->push_back(MenuOption(_(it->second.getShortDesc().c_str()), _(it->second.getLongDesc().c_str()), &it->second));
			opts->back().image = config_bg;
		}
		// Main config menu
		MenuOptions configmain;
		configmain.push_back(MenuOption(_("Audio Devices"), _("Setup microphones and playback"), "AudioDevices", "intro_configure.svg"));
		configmain.push_back(MenuOption(_("Audio"), _("Configure general audio settings"), audiomenu, "intro_configure.svg"));
		configmain.push_back(MenuOption(_("Graphics"), _("Configure rendering and video settings"), gfxmenu, "intro_configure.svg"));
		configmain.push_back(MenuOption(_("Game"), _("Gameplay related options"), gamemenu, "intro_configure.svg"));
		configmain.push_back(MenuOption(_("Paths"), _("Setup song and data paths"), "Paths", "intro_configure.svg"));
		// Add to root menu
		m_menu.add(MenuOption(_("Configure"), _("Configure audio and game options"), configmain, "intro_configure.svg"));
	}

	m_menu.add(MenuOption(_("Quit"), _("Leave the game"), "", "intro_quit.svg"));
}

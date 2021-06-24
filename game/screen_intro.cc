#include "screen_intro.hh"

#include "fs.hh"
#include "glmath.hh"
#include "audio.hh"
#include "i18n.hh"
#include "controllers.hh"
#include "platform.hh"
#include "theme.hh"
#include "menu.hh"

#include <SDL2/SDL_timer.h>

ScreenIntro::ScreenIntro(std::string const& name, Audio& audio): Screen(name), m_audio(audio), m_first(true) {
}

void ScreenIntro::enter() {
	Game::getSingletonPtr()->showLogo();

	m_audio.playMusic(findFile("menu.ogg"), true);
	m_selAnim = AnimValue(0.0, 10.0);
	m_submenuAnim = AnimValue(0.0, 3.0);
	populateMenu();
	if( m_first ) {
		std::string msg;
		if (!m_audio.hasPlayback()) msg = _("No playback devices could be used.\n");
		if (!msg.empty()) Game::getSingletonPtr()->dialog(msg + _("\nPlease configure some before playing."));
		m_first = false;
	}
	reloadGL();
	webserversetting = config["game/webserver_access"].i();
	m_audio.playSample("notice.ogg");
}

void ScreenIntro::reloadGL() {
	theme = std::make_unique<ThemeIntro>(showOpts);
}

void ScreenIntro::exit() {
	m_menu.clear();
	theme.reset();
}

void ScreenIntro::manageEvent(input::NavEvent const& event) {
	input::NavButton nav = event.button;
	if (nav == input::NAV_CANCEL) {
		if (m_menu.getSubmenuLevel() == 0) m_menu.moveToLast();  // Move cursor to quit in main menu
		else m_menu.closeSubmenu(); // One menu level up
	}
	else if (nav == input::NAV_DOWN || nav == input::NAV_MOREDOWN) m_menu.move(1);
	else if (nav == input::NAV_UP || nav == input::NAV_MOREUP) m_menu.move(-1);
	else if (nav == input::NAV_RIGHT && m_menu.getSubmenuLevel() >= 2) m_menu.action(1); // Config menu
	else if (nav == input::NAV_LEFT && m_menu.getSubmenuLevel() >= 2) m_menu.action(-1); // Config menu
	else if (nav == input::NAV_RIGHT && m_menu.getSubmenuLevel() < 2) m_menu.move(1); // Instrument nav hack
	else if (nav == input::NAV_LEFT && m_menu.getSubmenuLevel() < 2) m_menu.move(-1); // Instrument nav hack
	else if (nav == input::NAV_START) m_menu.action();
	else if (nav == input::NAV_PAUSE) m_audio.togglePause();
	// Animation targets
	m_selAnim.setTarget(m_menu.curIndex());
	m_submenuAnim.setTarget(m_menu.getSubmenuLevel());
}

void ScreenIntro::manageEvent(SDL_Event event) {
	if (event.type == SDL_KEYDOWN && m_menu.getSubmenuLevel() > 0) {
		// These are only available in config menu
		int key = event.key.keysym.scancode;
		uint16_t modifier = event.key.keysym.mod;
		if (key == SDL_SCANCODE_R && modifier & Platform::shortcutModifier() && m_menu.current().value) {
			m_menu.current().value->reset(modifier & KMOD_ALT);
		}
		else if (key == SDL_SCANCODE_S && modifier & Platform::shortcutModifier()) {
			writeConfig(modifier & KMOD_ALT);
			Game::getSingletonPtr()->flashMessage((modifier & KMOD_ALT)
				? _("Settings saved as system defaults.") : _("Settings saved."));
		}
	}
}

void ScreenIntro::draw_menu_options() {
	// Variables used for positioning and other stuff
	double wcounter = 0;
	const float x = -0.35;
	const float start_y = -0.1;
	const float sel_margin = 0.03;
	const MenuOptions opts = m_menu.getOptions();
	double submenuanim = 1.0 - std::min(1.0, std::abs(m_submenuAnim.get()-m_menu.getSubmenuLevel()));
	theme->back_h.dimensions.fixedHeight(0.038f);
	theme->back_h.dimensions.stretch(m_menu.dimensions.w(), theme->back_h.dimensions.h());
	// Determine from which item to start
	int start_i = std::min((int)m_menu.curIndex() - 1, (int)opts.size() - (int)showOpts
		+ (m_menu.getSubmenuLevel() == 2 ? 1 : 0)); // Hack to counter side-effects from displaying the value inside the menu
	if (start_i < 0 || opts.size() == showOpts) start_i = 0;

	// Loop the currently visible options
	for (size_t i = start_i, ii = 0; ii < showOpts && i < opts.size(); ++i, ++ii) {
		MenuOption const& opt = opts[i];
		ColorTrans c(Color::alpha(submenuanim));

		// Selection
		if (i == m_menu.curIndex()) {
			// Animate selection higlight moving
			double selanim = m_selAnim.get() - start_i;
			if (selanim < 0) selanim = 0;
			theme->back_h.dimensions.left(x - sel_margin).center(start_y + selanim*0.065);
			theme->back_h.draw();
			// Draw the text, dim if option not available
			{
				ColorTrans c(Color::alpha(opt.isActive() ? 1.0 : 0.5));
				theme->option_selected.dimensions.left(x).center(start_y + ii*0.065);
				theme->option_selected.draw(opt.getName());
			}
			wcounter = std::max(wcounter, theme->option_selected.w() + 2 * sel_margin); // Calculate the widest entry
			// If this is a config item, show the value below
			if (opt.type == MenuOption::CHANGE_VALUE) {
				++ii; // Use a slot for the value
				theme->option_selected.dimensions.left(x + sel_margin).center(-0.1 + (selanim+1)*0.065);
				theme->option_selected.draw("<  " + opt.value->getValue() + "  >");
			}

		// Regular option (not selected)
		} else {
			std::string title = opt.getName();
			SvgTxtTheme& txt = getTextObject(title);
			ColorTrans c(Color::alpha(opt.isActive() ? 1.0 : 0.5));
			txt.dimensions.left(x).center(start_y + ii*0.065);
			txt.draw(title);
			wcounter = std::max(wcounter, txt.w() + 2 * sel_margin); // Calculate the widest entry
		}
	}
	m_menu.dimensions.stretch(wcounter, 1);
}

void ScreenIntro::draw() {
	glutil::GLErrorChecker glerror("ScreenIntro::draw()");
	{
		float anim = SDL_GetTicks() % 20000 / 20000.0;
		ColorTrans c(glmath::rotate(TAU * anim, glmath::vec3(1.0, 1.0, 1.0)));
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
		theme->short_comment_bg.dimensions.stretch(theme->short_comment.w() + 0.065, 0.025);
		theme->short_comment_bg.dimensions.left(-0.54).screenBottom(-0.054);
		theme->short_comment_bg.draw();
		theme->short_comment.dimensions.left(-0.48).screenBottom(-0.067);
		theme->short_comment.draw(_("Ctrl + S to save, Ctrl + R to reset defaults"));
	}
	// Menu
	draw_menu_options();
	draw_webserverNotice();
}

SvgTxtTheme& ScreenIntro::getTextObject(std::string const& txt) {
	if (theme->options.find(txt) != theme->options.end()) return (*theme->options.at(txt).get());
	std::pair<std::string, std::unique_ptr<SvgTxtTheme>> kv = std::make_pair(txt, std::make_unique<SvgTxtTheme>(findFile("mainmenu_option.svg"), config["graphic/text_lod"].f(), WrappingStyle().menuScreenText(showOpts)));
	theme->options.insert(std::move(kv));
	return (*theme->options.at(txt).get());
}

void ScreenIntro::populateMenu() {
	MenuImage imgSing(new Texture(findFile("intro_sing.svg")));
	MenuImage imgPractice(new Texture(findFile("intro_practice.svg")));
	MenuImage imgConfig(new Texture(findFile("intro_configure.svg")));
	MenuImage imgQuit(new Texture(findFile("intro_quit.svg")));
	m_menu.clear();
	m_menu.add(MenuOption(_("Perform"), _("Start performing!"), imgSing).screen("Songs"));
	m_menu.add(MenuOption(_("Practice"), _("Check your skills or test the microphones"), imgPractice).screen("Practice"));
	// Configure menu + submenu options
	MenuOptions configmain;
	for (MenuEntry const& submenu: configMenu) {
		if (!submenu.items.empty()) {
			MenuOptions opts;
			// Process items that belong to that submenu
			for (std::string const& item: submenu.items) {
				ConfigItem& c = config[item];
				opts.push_back(MenuOption(_(c.getShortDesc().c_str()), _(c.getLongDesc().c_str())).changer(c));
			}
			configmain.push_back(MenuOption(_(submenu.shortDesc.c_str()), _(submenu.longDesc.c_str()), imgConfig).submenu(opts));
		} else {
			configmain.push_back(MenuOption(_(submenu.shortDesc.c_str()), _(submenu.longDesc.c_str()), imgConfig).screen(submenu.name));
		}
	}
	m_menu.add(MenuOption(_("Configure"), _("Configure audio and game options"), imgConfig).submenu(configmain));
	m_menu.add(MenuOption(_("Quit"), _("Leave the game"), imgQuit).screen(""));
}

#ifdef USE_WEBSERVER

void ScreenIntro::draw_webserverNotice() {
	if(m_webserverNoticeTimeout.get() == 0) {
		m_drawNotice = !m_drawNotice;
		m_webserverNoticeTimeout.setValue(5);
	}
	std::stringstream m_webserverStatusString;
	if((webserversetting == 1 || webserversetting == 2) && m_drawNotice) {
		std::string message = Game::getSingletonPtr()->subscribeWebserverMessages();		
		m_webserverStatusString << _("Webserver active!\n connect to this computer\nusing: ") << message;
		theme->WebserverNotice.draw(m_webserverStatusString.str());
	}
}

#else
void ScreenIntro::draw_webserverNotice() {}
#endif

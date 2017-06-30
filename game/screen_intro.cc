#include "screen_intro.hh"

#include "fs.hh"
#include "glmath.hh"
#include "audio.hh"
#include "i18n.hh"
#include "controllers.hh"
#include "theme.hh"
#include "menu.hh"
#include "xtime.hh"


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
	if(webserversetting) {
	m_audio.playSample("notice.ogg");
	}
}

void ScreenIntro::reloadGL() {
	theme.reset(new ThemeIntro());
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
		if (key == SDL_SCANCODE_R && modifier & KMOD_CTRL && m_menu.current().value) {
			m_menu.current().value->reset(modifier & KMOD_ALT);
		} else if (key == SDL_SCANCODE_S && modifier & KMOD_CTRL) {
			writeConfig(modifier & KMOD_ALT);
			Game::getSingletonPtr()->flashMessage((modifier & KMOD_ALT)
				? _("Settings saved as system defaults.") : _("Settings saved."));
		}
	}
}

void ScreenIntro::draw_menu_options() {
	// Variables used for positioning and other stuff
	double wcounter = 0;
	const size_t showopts = 4; // Show at most 4 options simultaneously
	const float x = -0.35;
	const float start_y = -0.1;
	const float sel_margin = 0.03;
	const MenuOptions opts = m_menu.getOptions();
	double submenuanim = 1.0 - std::min(1.0, std::abs(m_submenuAnim.get()-m_menu.getSubmenuLevel()));
	theme->back_h.dimensions.fixedHeight(0.038f);
	theme->back_h.dimensions.stretch(m_menu.dimensions.w(), theme->back_h.dimensions.h());
	// Determine from which item to start
	int start_i = std::min((int)m_menu.curIndex() - 1, (int)opts.size() - (int)showopts
		+ (m_menu.getSubmenuLevel() == 2 ? 1 : 0)); // Hack to counter side-effects from displaying the value inside the menu
	if (start_i < 0 || opts.size() == showopts) start_i = 0;

	// Loop the currently visible options
	for (size_t i = start_i, ii = 0; ii < showopts && i < opts.size(); ++i, ++ii) {
		MenuOption const& opt = opts[i];
		ColorTrans c(Color::alpha(submenuanim));

		// Selection
		if (i == m_menu.curIndex()) {
			// Animate selection higlight moving
			double selanim = m_selAnim.get() - start_i;
			if (selanim < 0) selanim = 0;
			theme->back_h.dimensions.left(x - sel_margin).center(start_y + selanim*0.08);
			theme->back_h.draw();
			// Draw the text, dim if option not available
			{
				ColorTrans c(Color::alpha(opt.isActive() ? 1.0 : 0.5));
				theme->option_selected.dimensions.left(x).center(start_y + ii*0.08);
				theme->option_selected.draw(opt.getName());
			}
			wcounter = std::max(wcounter, theme->option_selected.w() + 2 * sel_margin); // Calculate the widest entry
			// If this is a config item, show the value below
			if (opt.type == MenuOption::CHANGE_VALUE) {
				++ii; // Use a slot for the value
				theme->option_selected.dimensions.left(x + sel_margin).center(-0.1 + (selanim+1)*0.08);
				theme->option_selected.draw("<  " + opt.value->getValue() + "  >");
			}

		// Regular option (not selected)
		} else {
			std::string title = opt.getName();
			SvgTxtTheme& txt = getTextObject(title);
			ColorTrans c(Color::alpha(opt.isActive() ? 1.0 : 0.5));
			txt.dimensions.left(x).center(start_y + ii*0.08);
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
		ColorTrans c(glmath::rotate(2.0 * M_PI * anim, glmath::vec3(1.0, 1.0, 1.0)));
		theme->bg.draw();
	}
	glerror.check("bg");
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
	draw_webserverNotice();
}

SvgTxtTheme& ScreenIntro::getTextObject(std::string const& txt) {
	if (theme->options.contains(txt)) return theme->options[txt];
	return *theme->options.insert(txt, new SvgTxtTheme(findFile("mainmenu_option.svg"), config["graphic/text_lod"].f()))->second;
}

void ScreenIntro::populateMenu() {
	MenuImage imgSing(new Surface(findFile("intro_sing.svg")));
	MenuImage imgPractice(new Surface(findFile("intro_practice.svg")));
	MenuImage imgConfig(new Surface(findFile("intro_configure.svg")));
	MenuImage imgQuit(new Surface(findFile("intro_quit.svg")));
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

void ScreenIntro::draw_webserverNotice() {
	if(m_webserverNoticeTimeout.get() == 0) {
		m_drawNotice = !m_drawNotice;
		m_webserverNoticeTimeout.setValue(2);
	}
	std::stringstream m_webserverStatusString;
	if(webserversetting == 1 && m_drawNotice) { //TODO fetch port from config and add it to the string!
		m_webserverStatusString << _("Webserver active!\n use a webbrowser\nand navigate to localhost:") << config["game/webserver_port"].i();
		theme->WebserverNotice.draw(m_webserverStatusString.str());
	}
	else if(webserversetting == 2 && m_drawNotice) {
		if(m_ipaddr.empty()) {
			m_ipaddr = getIPaddr();
		}
		m_webserverStatusString << _("Webserver active!\n connect to this computer\nusing ") << m_ipaddr << ":" << config["game/webserver_port"].i();
		theme->WebserverNotice.draw(m_webserverStatusString.str());
	}
}


std::string ScreenIntro::getIPaddr() {
	try {
		boost::asio::io_service netService;
		boost::asio::ip::udp::resolver resolver(netService);
		boost::asio::ip::udp::resolver::query query(boost::asio::ip::udp::v4(), "google.com", ""); //it's a bit of a dirty hack, but it works!
		boost::asio::ip::udp::resolver::iterator endpoints = resolver.resolve(query);
		boost::asio::ip::udp::endpoint ep = *endpoints;
		boost::asio::ip::udp::socket socket(netService);
		socket.connect(ep);
		boost::asio::ip::address addr = socket.local_endpoint().address();
		return addr.to_string();
	} catch(std::exception& e) {
			return "cannot obtain IP";
	}

	return "IP address";
}

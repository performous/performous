#include "screen_intro.hh"

#include "fs.hh"
#include "glmath.hh"
#include "audio.hh"
#include "i18n.hh"
#include "controllers.hh"
#include "platform.hh"
#include "theme.hh"
#include "menu.hh"
#include "game.hh"
#include "graphic/color_trans.hh"

#include <SDL_timer.h>

ScreenIntro::ScreenIntro(Game &game, std::string const& name, Audio& audio): Screen(game, name), m_audio(audio), m_first(true) {
}

void ScreenIntro::enter() {
	getGame().showLogo();

	m_audio.playMusic(getGame(), findFile("menu.ogg"), true);
	m_selAnim = AnimValue(0.0, 10.0);
	m_submenuAnim = AnimValue(0.0, 3.0);
	populateMenu();
	if( m_first ) {
		std::string msg;
		if (!m_audio.hasPlayback()) msg = _("No playback devices could be used.\n");
		if (!msg.empty()) getGame().dialog(msg + _("\nPlease configure some before playing."));
		m_first = false;
	}
	reloadGL();
	webserversetting = config["webserver/access"].ui();
	m_audio.playSample("notice.ogg");
}

void ScreenIntro::reloadGL() {
	theme = std::make_unique<ThemeIntro>();
}

void ScreenIntro::exit() {
	m_menu.clear();
	theme.reset();
}

void ScreenIntro::manageEvent(input::NavEvent const& event) {
	input::NavButton nav = event.button;
	if (nav == input::NavButton::CANCEL) {
		if (m_menu.getSubmenuLevel() == 0) m_menu.moveToLast();  // Move cursor to quit in main menu
		else m_menu.closeSubmenu(); // One menu level up
	}
	else if (nav == input::NavButton::DOWN || nav == input::NavButton::MOREDOWN) m_menu.move(1);
	else if (nav == input::NavButton::UP || nav == input::NavButton::MOREUP) m_menu.move(-1);
	else if (nav == input::NavButton::RIGHT && m_menu.getSubmenuLevel() >= 2) m_menu.action(getGame(), 1); // Config menu
	else if (nav == input::NavButton::LEFT && m_menu.getSubmenuLevel() >= 2) m_menu.action(getGame(), -1); // Config menu
	else if (nav == input::NavButton::RIGHT && m_menu.getSubmenuLevel() < 2) m_menu.move(1); // Instrument nav hack
	else if (nav == input::NavButton::LEFT && m_menu.getSubmenuLevel() < 2) m_menu.move(-1); // Instrument nav hack
	else if (nav == input::NavButton::START) m_menu.action(getGame());
	else if (nav == input::NavButton::PAUSE) m_audio.togglePause();
	// Animation targets
	m_selAnim.setTarget(m_menu.curIndex());
	m_submenuAnim.setTarget(m_menu.getSubmenuLevel());
}

void ScreenIntro::manageEvent(SDL_Event event) {
	if (event.type == SDL_KEYDOWN && m_menu.getSubmenuLevel() > 0) {
		// These are only available in config menu
		int key = event.key.keysym.scancode;
		std::uint16_t modifier = event.key.keysym.mod;
		if (key == SDL_SCANCODE_R && modifier & Platform::shortcutModifier() && m_menu.current().value) {
			m_menu.current().value->reset(modifier & KMOD_ALT);
		}
		else if (key == SDL_SCANCODE_S && modifier & Platform::shortcutModifier()) {
			writeConfig(getGame(), modifier & KMOD_ALT);
			getGame().flashMessage((modifier & KMOD_ALT)
				? _("Settings saved as system defaults.") : _("Settings saved."));
		}
	}
}

void ScreenIntro::draw_menu_options() {
	auto& window = getGame().getWindow();
	// Variables used for positioning and other stuff
	float wcounter = 0.0f;
	const unsigned showopts = 5; // Show at most 5 options simultaneously
	const float x = -0.35f;
	const float start_y = -0.1f;
	const float sel_margin = 0.03f;
	const MenuOptions &opts = m_menu.getOptions();
	double submenuanim = 1.0 - std::min(1.0, std::abs(m_submenuAnim.get()-m_menu.getSubmenuLevel()));
	theme->back_h.dimensions.fixedHeight(0.038f);
	theme->back_h.dimensions.stretch(m_menu.dimensions.w(), theme->back_h.dimensions.h());
	// Determine from which item to start
	int start_i = std::min(static_cast<int>(m_menu.curIndex() - 1), static_cast<int>(opts.size() - showopts + (m_menu.getSubmenuLevel() == 2 ? 1 : 0))); // Hack to counter side-effects from displaying the value inside the menu
	if (start_i < 0 || opts.size() == showopts) start_i = 0;

	// Loop the currently visible options
	for (unsigned i = static_cast<unsigned>(start_i), ii = 0; ii < showopts && i < static_cast<unsigned>(opts.size()); ++i, ++ii) {
		MenuOption const& opt = opts[i];
		ColorTrans c(window, Color::alpha(static_cast<float>(submenuanim)));

		// Selection
		if (i == m_menu.curIndex()) {
			// Animate selection higlight moving
			double selanim = m_selAnim.get() - start_i;
			if (selanim < 0.0) selanim = 0.0;
			theme->back_h.dimensions.left(x - sel_margin).center(static_cast<float>(start_y + selanim*0.065));
			theme->back_h.draw(window);
			// Draw the text, dim if option not available
			{
				ColorTrans c(window, Color::alpha(opt.isActive() ? 1.0f : 0.5f));
				theme->option_selected.dimensions.left(x).center(start_y + static_cast<float>(ii)*0.065f);
				theme->option_selected.draw(window, _(opt.getName()));
			}
			wcounter = std::max(wcounter, theme->option_selected.w() + 2.0f * sel_margin); // Calculate the widest entry
			// If this is a config item, show the value below
			if (opt.type == MenuOption::Type::CHANGE_VALUE) {
				++ii; // Use a slot for the value
				theme->option_selected.dimensions.left(x + sel_margin).center(static_cast<float>(-0.1 + (selanim+1.0)*0.065));
				theme->option_selected.draw(window, "<  " + _(opt.value->getValue()) + "  >");
			}

		// Regular option (not selected)
		} else {
			std::string title = _(opt.getName());
			SvgTxtTheme& txt = getTextObject(title);
			ColorTrans c(window, Color::alpha(opt.isActive() ? 1.0f : 0.5f));
			txt.dimensions.left(x).center(start_y + static_cast<float>(ii)*0.065f);
			txt.draw(window, title);
			wcounter = std::max(wcounter, txt.w() + 2.0f * sel_margin); // Calculate the widest entry
		}
	}
	m_menu.dimensions.stretch(wcounter, 1.0f);
}

void ScreenIntro::draw() {
	auto& window = getGame().getWindow();
	glutil::GLErrorChecker glerror("ScreenIntro::draw()");
	{
		float anim = static_cast<float>(SDL_GetTicks() % 20000 / 20000.0);
		ColorTrans c(window, glmath::rotate(static_cast<float>(TAU * anim), glmath::vec3(1.0f, 1.0f, 1.0f)));
		theme->bg.draw(window);
	}
	if (m_menu.current().image) m_menu.current().image->draw(window);
	// Comment
	theme->comment_bg.dimensions.center().screenBottom(-0.01f);
	theme->comment_bg.draw(window);
	theme->comment.dimensions.left(-0.48f).screenBottom(-0.028f);
	theme->comment.draw(window, _(m_menu.current().getComment()));
	// Key help for config
	if (m_menu.getSubmenuLevel() > 0) {
		theme->short_comment_bg.dimensions.stretch(theme->short_comment.w() + 0.065f, 0.025f);
		theme->short_comment_bg.dimensions.left(-0.54f).screenBottom(-0.054f);
		theme->short_comment_bg.draw(window);
		theme->short_comment.dimensions.left(-0.48f).screenBottom(-0.067f);
		theme->short_comment.draw(window, _("Ctrl + S to save, Ctrl + R to reset defaults"));
	}
	// Menu
	draw_menu_options();
	draw_webserverNotice();
}

SvgTxtTheme& ScreenIntro::getTextObject(std::string const& txt) {
	if (theme->options.find(txt) != theme->options.end()) return (*theme->options.at(txt).get());
	std::pair<std::string, std::unique_ptr<SvgTxtTheme>> kv = std::make_pair(txt, std::make_unique<SvgTxtTheme>(findFile("mainmenu_option.svg"), config["graphic/text_lod"].f()));
	theme->options.insert(std::move(kv));
	return (*theme->options.at(txt).get());
}

void ScreenIntro::populateMenu() {
	MenuImage imgSing(new Texture(findFile("intro_sing.svg")));
	MenuImage imgPractice(new Texture(findFile("intro_practice.svg")));
	MenuImage imgConfig(new Texture(findFile("intro_configure.svg")));
	MenuImage imgQuit(new Texture(findFile("intro_quit.svg")));
	m_menu.clear();
	m_menu.add(MenuOption(translate_noop("Perform"), translate_noop("Start performing!"), imgSing)).screen("Songs");
	m_menu.add(MenuOption(translate_noop("Practice"), translate_noop("Check your skills or test the microphones."), imgPractice)).screen("Practice");
	// Configure menu + submenu options
	MenuOptions configmain;
	for (auto const& submenu: configMenu) {
		if (!submenu.items.empty()) {
			MenuOptions opts;
			// Process items that belong to that submenu
			for (auto const& item: submenu.items) {
				ConfigItem& c = config[item];
				opts.emplace_back(MenuOption(c.getShortDesc(), c.getLongDesc()));
				opts.back().changer(c);
			}
			configmain.emplace_back(MenuOption(submenu.shortDesc, submenu.longDesc, imgConfig));
			configmain.back().submenu(std::move(opts));
		} else {
			configmain.emplace_back(MenuOption(submenu.shortDesc, submenu.longDesc, imgConfig));
			configmain.back().screen(submenu.name);
		}
	}
	m_menu.add(MenuOption(translate_noop("Configure"), translate_noop("Configure audio and game options."), imgConfig)).submenu(std::move(configmain));
	m_menu.add(MenuOption(translate_noop("Quit"), translate_noop("Leave the game."), imgQuit)).screen("");
}

#ifdef USE_WEBSERVER

void ScreenIntro::draw_webserverNotice() {
	auto& window = getGame().getWindow();
	if(m_webserverNoticeTimeout.get() == 0) {
		m_drawNotice = !m_drawNotice;
		m_webserverNoticeTimeout.setValue(5);
	}
	std::stringstream m_webserverStatusString;
	if((webserversetting >= 1) && m_drawNotice) {
		std::string message(m_game.subscribeWebserverMessages());
		m_webserverStatusString << _("Webserver active!\n connect to this computer\nusing ") << message;
		theme->WebserverNotice.draw(window, m_webserverStatusString.str());
	}
}

#else
void ScreenIntro::draw_webserverNotice() {}
#endif

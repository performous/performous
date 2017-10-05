#include "screen_paths.hh"

#include "configuration.hh"
#include "controllers.hh"
#include "platform.hh"
#include "theme.hh"
#include "audio.hh"
#include "i18n.hh"
#include <boost/thread.hpp>
#include <boost/bind.hpp>

ScreenPaths::ScreenPaths(std::string const& name, Audio& audio, Songs& songs): Screen(name), m_audio(audio), m_songs(songs) {}

void ScreenPaths::enter() {
	m_theme.reset(new ThemeAudioDevices());
	fs::path homedir(getenv("HOME"));
	generateMenuFromPath(homedir);
}

void ScreenPaths::exit() {
	m_theme.reset();
	m_songs.reload();
}

void ScreenPaths::manageEvent(SDL_Event event) {
	if (event.type == SDL_KEYDOWN) {
		SDL_Keycode key = event.key.keysym.scancode;
		uint16_t modifier = event.key.keysym.mod;
		// Reset to defaults
		if (key == SDL_SCANCODE_R && modifier & Platform::shortcutModifier()) {
			config["paths/songs"].reset(modifier & KMOD_ALT);
			config["paths/system"].reset(modifier & KMOD_ALT);
			// TODO: Save
		}
		else if (key == SDL_SCANCODE_S && modifier & Platform::shortcutModifier()) {
			writeConfig(m_audio, modifier & KMOD_ALT);
			Game::getSingletonPtr()->flashMessage((modifier & KMOD_ALT)
				? _("Settings saved as system defaults.") : _("Settings saved."));
		}
	}
}

void ScreenPaths::manageEvent(input::NavEvent const& ev) {
	Game* gm = Game::getSingletonPtr();
	if (ev.button == input::NAV_CANCEL) {
		gm->activateScreen("Intro");
	}
	else if (ev.button == input::NAV_PAUSE) m_audio.togglePause();
	else if (ev.button == input::NAV_DOWN) m_menu.move(1); //one down
	else if (ev.button == input::NAV_MOREDOWN) m_menu.move(5); //five down (page-dwon-key)
	else if (ev.button == input::NAV_UP) m_menu.move(-1); //one up
	else if (ev.button == input::NAV_MOREUP) m_menu.move(-5); //five up (page up key)
	else if (ev.button == input::NAV_START) m_menu.action(); //enter: execute currently selected option.
}

void ScreenPaths::generateMenuFromPath(fs::path path) {
	m_menu.clear();
	bool folderInConfig = false;
	ConfigItem::StringList& sl = config["paths/songs"].sl();
	ConfigItem::StringList::iterator position = sl.begin();
	for(unsigned int i=0; i<sl.size(); i++) {
		std::string pathstring = path.string();
		if(sl.at(i) == pathstring) {
			folderInConfig = true;
			position = sl.begin() + i;
			break;
		}
	}
	bool showHiddenfolders = config["paths/showhiddenfolders"].b();
	if(showHiddenfolders) {
		m_menu.add(MenuOption(_("Hide hidden folders"),_("Hide hidden folders")).call([this, sl, path, position]() {
			config["paths/showhiddenfolders"].b() = false;
			generateMenuFromPath(path);
		}));
	} else {
		m_menu.add(MenuOption(_("Show hidden folders"),_("Show hidden folders")).call([this, sl, path, position]() {
			config["paths/showhiddenfolders"].b() = true;
			generateMenuFromPath(path);
		}));
	}

	if(folderInConfig) {
		m_menu.add(MenuOption(_("Remove this folder"),_("Remove current folder from song folders")).call([this, sl, path, position]() {
			config["paths/songs"].sl().erase(position); //WHY the fuck is this const??
			generateMenuFromPath(path);
			//Reload internal, but that crashes!! rely on the user to press ctrl+r in song selection screen
		}));
	} else {
		m_menu.add(MenuOption(_("Add this folder"),_("Add current folder to song folders")).call([this, sl, path]() {
			config["paths/songs"].sl().push_back(path.string()); //WHY the fuck is this const??
			generateMenuFromPath(path);
			//Reload internal, but that crashes!! rely on the user to press ctrl+r in song selection screen
		}));
	}
	m_menu.add(MenuOption(_(".."),_("Go up one folder")).call([this, sl, path, position]() {
		generateMenuFromPath(path.parent_path());
	}));
//todo sort folders
	for (fs::directory_iterator dirIt(path), dirEnd; dirIt != dirEnd; ++dirIt) { //loop through files and directories
		fs::path p = dirIt->path();
		if (fs::is_directory(p)) {
			if(p.filename().c_str()[0] == '.' && !showHiddenfolders) {
				std::clog << "screen_paths/notice: Ignoring hidden folder: ." << p.c_str() << std::endl;
				continue;
			} else {
				m_menu.add(MenuOption(p.string(),_("Open folder")).call([this, p](){ generateMenuFromPath(p);
				}));
			}
			
		}
	}
}


void ScreenPaths::draw() {

	m_theme->bg.draw();

	//draw menu:
	{
		m_theme->back_h.dimensions.fixedHeight(0.065f);
		m_theme->back_h.dimensions.stretch(m_menu.dimensions.w(), m_theme->back_h.dimensions.h());
		const size_t showopts = 13; // Show at most 8 options simultaneously
		const float sel_margin = 0.04;
		const float x = -0.45;
		const float start_y = -0.15;
		double wcounter = 0;
		const MenuOptions opts = m_menu.getOptions();
		int start_i = std::min((int)m_menu.curIndex() - 1, (int)opts.size() - (int)showopts
			+ (m_menu.getSubmenuLevel() == 2 ? 1 : 0)); // Hack to counter side-effects from displaying the value inside the menu
		if (start_i < 0 || opts.size() == showopts) { start_i = 0; }
		for (size_t i = start_i, ii = 0; ii < showopts && i < opts.size(); ++i, ++ii) {
			MenuOption const& opt = opts[i];
			if (i == m_menu.curIndex()) {
				double selanim = m_selAnim.get() - start_i;
				if (selanim < 0) { selanim = 0; }
				m_theme->back_h.dimensions.left(x - sel_margin).center(start_y + selanim*0.08);
				m_theme->back_h.draw();
				// Draw the text, dim if option not available
				{
					ColorTrans c(Color::alpha(opt.isActive() ? 1.0 : 0.5));
					m_theme->device.dimensions.left(x).center(start_y + ii*0.03);
					m_theme->device.draw(opt.getName());
				} // to make the colortrans object go out of scope
				wcounter = std::max(wcounter, m_theme->device.w() + 2 * sel_margin); // Calculate the widest entry
				// If this is a config item, show the value below
			} else {
				ColorTrans c(Color::alpha(opt.isActive() ? 0.8 : 0.5));
				m_theme->device.dimensions.left(x).center(start_y + ii*0.03);
				m_theme->device.draw(opt.getName());
			}
		}
	} //draw menu
	m_theme->comment_bg.dimensions.center().screenBottom(-0.01);
	m_theme->comment_bg.draw();
	m_theme->comment.dimensions.left(-0.48).screenBottom(-0.028);
	m_theme->comment.draw(m_menu.current().getComment());

}

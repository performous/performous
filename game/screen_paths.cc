#include "screen_paths.hh"

#include "configuration.hh"
#include "controllers.hh"
#include "theme.hh"
#include "audio.hh"
#include "i18n.hh"
#include <boost/thread.hpp>
#include <boost/bind.hpp>

ScreenPaths::ScreenPaths(std::string const& name, Audio& audio): Screen(name), m_audio(audio) {}

void ScreenPaths::enter() {
	m_theme.reset(new ThemeAudioDevices());
	m_txtinp.text.clear();
	fs::path homedir(getenv("HOME"));
	generateMenuFromPath(homedir);
}

void ScreenPaths::exit() { m_theme.reset(); }

void ScreenPaths::manageEvent(SDL_Event event) {
	if (event.type == SDL_TEXTINPUT) {
		m_txtinp += event.text.text;
	} else if (event.type == SDL_KEYDOWN) {
		SDL_Keycode key = event.key.keysym.scancode;
		uint16_t modifier = event.key.keysym.mod;
		// Reset to defaults
		if (key == SDL_SCANCODE_R && modifier & KMOD_CTRL) {
			config["paths/songs"].reset(modifier & KMOD_ALT);
			config["paths/system"].reset(modifier & KMOD_ALT);
			// TODO: Save
		}
	}
}

void ScreenPaths::manageEvent(input::NavEvent const& ev) {
	Game* gm = Game::getSingletonPtr();
	if (ev.button == input::NAV_CANCEL) {
		if (m_txtinp.text.empty()) gm->activateScreen("Intro");
		else m_txtinp.text.clear();
	}
	else if (ev.button == input::NAV_PAUSE) m_audio.togglePause();
	else if (ev.button == input::NAV_START) { 
		// TODO: Save config
		gm->activateScreen("Intro");
	}
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
	if(folderInConfig) {
		m_menu.add(MenuOption(_("Remove this folder"),_("Remove current folder from song folders")).call([this, sl, path, position]() {
			//sl.erase(position); //WHY the fuck is this const??
			config["paths/songs"].sl() = sl;
		}));
	} else {
		m_menu.add(MenuOption(_("Add this folder"),_("Add current folder to song folders")).call([this, sl, path]() {
		   // sl.push_back(path.string()); //WHY the fuck is this const??
		   config["paths/songs"].sl() = sl;
		}));
	}
	for (fs::directory_iterator dirIt(path), dirEnd; dirIt != dirEnd; ++dirIt) { //loop through files and directories
		fs::path p = dirIt->path();
		if (fs::is_directory(p)) {
			m_menu.add(MenuOption(p.c_str(),_("Open folder")).call([this, p](){
				generateMenuFromPath(p);
			}));
		}
	}
}


void ScreenPaths::draw() {

	m_theme->bg.draw();

	//draw menu:
	{
		const size_t showopts = 9; // Show at most 8 options simultaneously
		const float sel_margin = 0.04;
		const float x = -0.35;
		const float start_y = -0.15;
		double wcounter = 0;
		const MenuOptions opts = m_menu.getOptions();
		int start_i = std::min((int)m_menu.curIndex() - 1, (int)opts.size() - (int)showopts
			+ (m_menu.getSubmenuLevel() == 2 ? 1 : 0)); // Hack to counter side-effects from displaying the value inside the menu
		if (start_i < 0 || opts.size() == showopts) start_i = 0;
		for (size_t i = start_i, ii = 0; ii < showopts && i < opts.size(); ++i, ++ii) {
			MenuOption const& opt = opts[i];
			if (i == m_menu.curIndex()) {
				// Animate selection higlight moving
				wcounter = std::max(wcounter, m_theme->device.w() + 2 * sel_margin); // Calculate the widest entry
			} else {
				std::string title = opt.getName();
				ColorTrans c(Color::alpha(opt.isActive() ? 0.8 : 0.5));
				m_theme->device.dimensions.left(x).center(start_y + ii*0.05);
				m_theme->device.draw(title);
			}
		}
	}



	// Key help
	m_theme->comment_bg.dimensions.stretch(1.0, 0.025).middle().screenBottom(-0.054);
	m_theme->comment_bg.draw();
	m_theme->comment.dimensions.left(-0.48).screenBottom(-0.067);
	m_theme->comment.draw(_("Press any key to exit."));
	// Additional info
	#ifdef _WIN32
	m_theme->comment_bg.dimensions.middle().screenBottom(-0.01);
	m_theme->comment_bg.draw();
	m_theme->comment.dimensions.left(-0.48).screenBottom(-0.023);
	m_theme->comment.draw(_("Windows users can also use ConfigureSongDirectory.bat script."));
	#endif
}

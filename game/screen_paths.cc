#include "screen_paths.hh"

#include "configuration.hh"
#include "controllers.hh"
#include "platform.hh"
#include "theme.hh"
#include "audio.hh"
#include "i18n.hh"
#include "game.hh"

ScreenPaths::ScreenPaths(Game &game, std::string const& name, Audio& audio, Songs& songs): Screen(game, name), m_audio(audio), m_songs(songs) {}

void ScreenPaths::enter() {
	m_theme = std::make_unique<ThemeAudioDevices>();
	generateMenuFromPath(getHomeDir());
}

void ScreenPaths::exit() {
	m_theme.reset();
	m_songs.reload();
}

void ScreenPaths::manageEvent(SDL_Event event) {
	if (event.type == SDL_KEYDOWN) {
		SDL_Keycode key = event.key.keysym.scancode;
		std::uint16_t modifier = event.key.keysym.mod;
		// Reset to defaults
		if (key == SDL_SCANCODE_R && modifier & Platform::shortcutModifier()) {
			config["paths/songs"].reset(modifier & KMOD_ALT);
			config["paths/system"].reset(modifier & KMOD_ALT);
			// TODO: Save
		}
		else if (key == SDL_SCANCODE_S && modifier & Platform::shortcutModifier()) {
			writeConfig(getGame(), m_audio, modifier & KMOD_ALT);
			getGame().flashMessage((modifier & KMOD_ALT)
				? _("Settings saved as system defaults.") : _("Settings saved."));
		}
	}
}

void ScreenPaths::manageEvent(input::NavEvent const& ev) {
	if (ev.button == input::NavButton::CANCEL) {
		getGame().activateScreen("Intro");
	}
	else if (ev.button == input::NavButton::PAUSE) m_audio.togglePause();
	else if (ev.button == input::NavButton::DOWN) m_menu.move(1); //one down
	else if (ev.button == input::NavButton::MOREDOWN) m_menu.move(5); //five down (page-dwon-key)
	else if (ev.button == input::NavButton::UP) m_menu.move(-1); //one up
	else if (ev.button == input::NavButton::MOREUP) m_menu.move(-5); //five up (page up key)
	else if (ev.button == input::NavButton::START) m_menu.action(); //enter: execute currently selected option.
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
		m_menu.add(MenuOption(_("Hide hidden folders"),_("Hide hidden folders"))).call([this, sl, path]() {
			config["paths/showhiddenfolders"].b() = false;
			generateMenuFromPath(path);
		});
	} else {
		m_menu.add(MenuOption(_("Show hidden folders"),_("Show hidden folders"))).call([this, sl, path]() {
			config["paths/showhiddenfolders"].b() = true;
			generateMenuFromPath(path);
		});
	}

	if(folderInConfig) {
		m_menu.add(MenuOption(_("Remove this folder"),_("Remove current folder from song folders"))).call([this, sl, path, position]() {
			config["paths/songs"].sl().erase(position); //WHY the fuck is this const??
			generateMenuFromPath(path);
			//Reload internal, but that crashes!! rely on the user to press ctrl+r in song selection screen
		});
	} else {
		m_menu.add(MenuOption(_("Add this folder"),_("Add current folder to song folders"))).call([this, sl, path]() {
			config["paths/songs"].sl().push_back(path.string()); //WHY the fuck is this const??
			generateMenuFromPath(path);
			//Reload internal, but that crashes!! rely on the user to press ctrl+r in song selection screen
		});
	}
	auto parent = path.parent_path();
	if (!parent.empty() && parent != path)
		m_menu.add(MenuOption(_(".."),_("Go to parent folder"))).call([this, sl, path]() {
					generateMenuFromPath(path.parent_path());
	});

	// Extract list of all directories
	std::list<fs::path> directories;
	for (const auto &di : fs::directory_iterator(path)) {
		auto &p = di.path();
		if (fs::is_directory(p) && (showHiddenfolders || p.filename().c_str()[0] != '.')) {
			directories.emplace_back(p);
		}
	}

	// sort the directory list
	directories.sort(
			[] (const auto &a, const auto &b) {
				const auto &an = a.filename();
				const auto &bn = b.filename();
				/* If two entries have the same name, take the address of object as
				 * sort criterion as they can only be equal IIF they are the same */
				if (an == bn)
					return &a < &b;
				return an < bn;
			});

	// Add entries to menu
	for (const auto &p : directories) {
		m_menu.add(MenuOption(p.string(), _("Open folder"))).call([this, p] { generateMenuFromPath(p); });
	}
}


void ScreenPaths::draw() {

	m_theme->bg.draw();

	//draw menu:
	{
		m_theme->back_h.dimensions.fixedHeight(0.065f);
		m_theme->back_h.dimensions.stretch(m_menu.dimensions.w(), m_theme->back_h.dimensions.h());
		const unsigned showopts = 13; // Show at most 8 options simultaneously
		const float sel_margin = 0.04f;
		const float x = -0.45f;
		const float start_y = -0.15f;
		float wcounter = 0.0f;
		const MenuOptions &opts = m_menu.getOptions();
		int start_i = std::min(static_cast<int>(m_menu.curIndex() - 1), static_cast<int>(opts.size() - showopts + (m_menu.getSubmenuLevel() == 2 ? 1 : 0))); // Hack to counter side-effects from displaying the value inside the menu
		if (start_i < 0 || opts.size() == showopts) { start_i = 0; }
		for (unsigned i = static_cast<unsigned>(start_i), ii = 0; ii < showopts && i < static_cast<unsigned>(opts.size()); ++i, ++ii) {
			MenuOption const& opt = opts[i];
			if (i == m_menu.curIndex()) {
				double selanim = m_selAnim.get() - start_i;
				if (selanim < 0.0) { selanim = 0.0; }
				m_theme->back_h.dimensions.left(x - sel_margin).center(static_cast<float>(start_y + selanim*0.08f));
				m_theme->back_h.draw();
				// Draw the text, dim if option not available
				{
					ColorTrans c(Color::alpha(opt.isActive() ? 1.0f : 0.5f));
					m_theme->device.dimensions.left(x).center(start_y + static_cast<float>(ii)*0.03f);
					m_theme->device.draw(opt.getName());
				} // to make the colortrans object go out of scope
				wcounter = std::max(wcounter, m_theme->device.w() + 2.0f * sel_margin); // Calculate the widest entry
				// If this is a config item, show the value below
			} else {
				ColorTrans c(Color::alpha(opt.isActive() ? 0.8f : 0.5f));
				m_theme->device.dimensions.left(x).center(start_y + static_cast<float>(ii)*0.03f);
				m_theme->device.draw(opt.getName());
			}
		}
	} //draw menu
	m_theme->comment_bg.dimensions.center().screenBottom(-0.01f);
	m_theme->comment_bg.draw();
	m_theme->comment.dimensions.left(-0.48f).screenBottom(-0.028f);
	m_theme->comment.draw(m_menu.current().getComment());

}

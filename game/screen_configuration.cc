#include "screen_configuration.hh"

ScreenConfiguration::ScreenConfiguration(std::string const& name, Audio& audio): Screen(name), m_audio(audio), selected() {
	for (ConfigMenu::const_iterator it = configMenu.begin(); it != configMenu.end(); ++it) {
		for (size_t i = 0; i < it->items.size(); ++i) {
			//ConfigItem const& item = config[it->items[i]];
			//if (item.get_type() == "string" || item.get_type() == "string_list") continue; // TODO: remove this line and implement menu for strings and string_lists
			configuration.push_back(it->items[i]);
		}
	}
}

void ScreenConfiguration::enter() {
	theme.reset(new ThemeConfiguration());
}

void ScreenConfiguration::exit() { theme.reset(); }

void ScreenConfiguration::manageEvent(SDL_Event event) {
	ScreenManager* sm = ScreenManager::getSingletonPtr();
	ConfigItem* ci = NULL;
	if (!configuration.empty()) ci = &config[configuration[selected]];
	if (event.type == SDL_KEYDOWN) {
		int key = event.key.keysym.sym;
		SDLMod modifier = event.key.keysym.mod;
		if (key == SDLK_ESCAPE || key == SDLK_q) sm->activateScreen("Intro");
		else if (key == SDLK_SPACE || key == SDLK_PAUSE) m_audio.togglePause();
		if (configuration.empty()) return; // The rest work if there are any config options
		else if (key == SDLK_UP && selected > 0) --selected;
		else if (key == SDLK_DOWN && selected + 1 < configuration.size()) ++selected;
		else if (key == SDLK_LEFT) --*ci;
		else if (key == SDLK_RIGHT) ++*ci;
		else if (key == SDLK_r && modifier & KMOD_CTRL) ci->reset();
		else if (key == SDLK_s && modifier & KMOD_CTRL) writeConfig();
	}
}

void ScreenConfiguration::draw() {
	theme->bg.draw();
	if (!configuration.empty()) {
		ConfigItem const& ci = config[configuration[selected]];
		theme->item.draw(ci.getShortDesc());
		theme->value.draw(ci.getValue());
	}
}


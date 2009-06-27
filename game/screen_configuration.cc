#include "screen_configuration.hh"

ScreenConfiguration::ScreenConfiguration(std::string const& name, Audio& audio): Screen(name), m_audio(audio), selected() {
	for (ConfigMenu::const_iterator it = configMenu.begin(); it != configMenu.end(); ++it) {
		for (size_t i = 0; i < it->items.size(); ++i) {
			ConfigItem const& item = config[it->items[i]];
			if (item.get_type() == "string" || item.get_type() == "string_list") continue; // TODO: remove this line and implement menu for strings and string_lists
			configuration.push_back(new ConfigurationItem(it->items[i]));
		}
	}
}

void ScreenConfiguration::enter() {
	theme.reset(new ThemeConfiguration());
}

void ScreenConfiguration::exit() { theme.reset(); }

void ScreenConfiguration::manageEvent(SDL_Event event) {
	ScreenManager* sm = ScreenManager::getSingletonPtr();
	if (event.type == SDL_KEYDOWN) {
		int key = event.key.keysym.sym;
		SDLMod modifier = event.key.keysym.mod;
		if (key == SDLK_ESCAPE || key == SDLK_q) sm->activateScreen("Intro");
		else if (key == SDLK_SPACE || key == SDLK_PAUSE) m_audio.togglePause();
		else if (key == SDLK_UP && selected > 0) --selected;
		else if (key == SDLK_DOWN && selected < configuration.size() - 1) ++selected;
		else if (key == SDLK_LEFT && configuration.size() > 0) configuration[selected].setPrevious();
		else if (key == SDLK_RIGHT && configuration.size() > 0) configuration[selected].setNext();
		else if (key == SDLK_s && modifier & KMOD_CTRL) writeConfig();
	}
}

void ScreenConfiguration::draw() {
	theme->bg.draw();
	if(  configuration.size() > 0 ) {
		theme->item.draw(configuration[selected].getDescription());
		theme->value.draw(configuration[selected].getValue());
	}
}


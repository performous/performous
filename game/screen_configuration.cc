#include "screen_configuration.hh"

ScreenConfiguration::ScreenConfiguration(std::string const& name, Audio& audio): Screen(name), m_audio(audio)
{
	for(std::map<std::string, ConfigItem>::const_iterator itr = config.begin(); itr != config.end(); ++itr) {
		ConfigItem item = (*itr).second;
		std::string name = (*itr).first;
		if( item.get_type() != std::string("string") && item.get_type() != std::string("string_list") ) {
			configuration.push_back(new ConfigurationItem(item));
		}
	}
	selected=0;
}

void ScreenConfiguration::enter() {
	theme.reset(new ThemeConfiguration());
}

void ScreenConfiguration::exit() { theme.reset(); }

void ScreenConfiguration::manageEvent(SDL_Event event) {
	ScreenManager* sm = ScreenManager::getSingletonPtr();
	if (event.type == SDL_KEYDOWN) {
		int key = event.key.keysym.sym;
		if (key == SDLK_ESCAPE || key == SDLK_q) sm->activateScreen("Intro");
		else if (key == SDLK_SPACE || key == SDLK_PAUSE) m_audio.togglePause();
		else if (key == SDLK_UP && selected > 0) --selected;
		else if (key == SDLK_DOWN && selected + 1 < configuration.size()) ++selected;
		else if (key == SDLK_LEFT && configuration.size() > 0) configuration[selected].setPrevious();
		else if (key == SDLK_RIGHT && configuration.size() > 0) configuration[selected].setNext();
	}
}

void ScreenConfiguration::draw() {
	theme->bg->draw();
	if(  configuration.size() > 0 ) {
		theme->item->draw(configuration[selected].getDescription());
		theme->value->draw(configuration[selected].getValue());
	}
}


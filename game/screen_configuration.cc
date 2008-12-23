#include "screen_configuration.hh"

ScreenConfiguration::ScreenConfiguration(std::string const& name, Audio& audio): Screen(name), m_audio(audio)
{
	configuration.push_back(new ConfigurationAudioVolume("Music Volume", audio, &Audio::getVolumeMusic, &Audio::setVolumeMusic));
	configuration.push_back(new ConfigurationAudioVolume("Song Preview Volume", audio, &Audio::getVolumePreview, &Audio::setVolumePreview));
	selected=0;
}

void ScreenConfiguration::enter() {
	theme.reset(new ThemeConfiguration());
}

void ScreenConfiguration::exit() { theme.reset(); }

void ScreenConfiguration::manageEvent(SDL_Event event) {
	ScreenManager* sm = ScreenManager::getSingletonPtr();
	if (event.type == SDL_KEYDOWN) {
		m_audio.setVolumeMusic(m_audio.getVolumeMusic()); // Hack to reset the volume after preview volume adjustment
		int key = event.key.keysym.sym;
		if (key == SDLK_ESCAPE || key == SDLK_q) sm->activateScreen("Intro");
		else if (key == SDLK_SPACE || key == SDLK_PAUSE) m_audio.togglePause();
		else if (key == SDLK_UP && selected > 0) --selected;
		else if (key == SDLK_DOWN && selected + 1 < configuration.size()) ++selected;
		else if (key == SDLK_LEFT) configuration[selected].setPrevious();
		else if (key == SDLK_RIGHT) configuration[selected].setNext();
	}
}

void ScreenConfiguration::draw() {
	theme->bg->draw();
	theme->item->draw(configuration[selected].getDescription());
	theme->value->draw(configuration[selected].getValue());
}


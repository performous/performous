#include "screen_configuration.hh"

CScreenConfiguration::CScreenConfiguration(std::string const& name): CScreen(name)
{
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	CAudio& a = *sm->getAudio();
	configuration.push_back(new CConfigurationAudioVolume("Music Volume", a, &CAudio::getVolumeMusic, &CAudio::setVolumeMusic));
	configuration.push_back(new CConfigurationAudioVolume("Song Preview Volume", a, &CAudio::getVolumePreview, &CAudio::setVolumePreview));
	selected=0;
}

void CScreenConfiguration::enter() {
	theme.reset(new CThemeConfiguration());
}

void CScreenConfiguration::exit() { theme.reset(); }

void CScreenConfiguration::manageEvent(SDL_Event event) {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	if (event.type == SDL_KEYDOWN) {
		sm->getAudio()->setVolumeMusic(sm->getAudio()->getVolumeMusic()); // Hack to reset the volume after preview volume adjustment
		int key = event.key.keysym.sym;
		if (key == SDLK_ESCAPE || key == SDLK_q) sm->activateScreen("Intro");
		else if (key == SDLK_SPACE || key == SDLK_PAUSE) sm->getAudio()->togglePause();
		else if (key == SDLK_UP && selected > 0) --selected;
		else if (key == SDLK_DOWN && selected + 1 < configuration.size()) ++selected;
		else if (key == SDLK_LEFT) configuration[selected].setPrevious();
		else if (key == SDLK_RIGHT) configuration[selected].setNext();
	}
}

void CScreenConfiguration::draw() {
	theme->bg->draw();
	theme->item->draw(configuration[selected].getDescription());
	theme->value->draw(configuration[selected].getValue());
}


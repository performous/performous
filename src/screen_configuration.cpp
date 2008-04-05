#include <screen_configuration.h>

CScreenConfiguration::CScreenConfiguration(std::string const& name): CScreen(name)
{
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	configuration.push_back(new CConfigurationFullscreen());
	configuration.push_back(new CConfigurationAudioVolume("Ingame Audio Volume", sm->m_ingameVolume));
	configuration.push_back(new CConfigurationAudioVolume("Menu Audio Volume", sm->m_menuVolume));
	selected=0;
}

void CScreenConfiguration::enter() {
	theme.reset(new CThemeConfiguration());
}

void CScreenConfiguration::exit() { theme.reset(); }

void CScreenConfiguration::manageEvent(SDL_Event event) {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	if (event.type == SDL_KEYDOWN) {
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
	theme->theme->clear();
	cairo_text_extents_t extents;
	{
		theme->item.text = configuration[selected].getDescription();
		extents = theme->theme->GetTextExtents(theme->item);
		theme->item.x = (theme->item.svg_width - extents.width)/2;
		theme->theme->PrintText(&theme->item);

		theme->value.text = configuration[selected].getValue();
		extents = theme->theme->GetTextExtents(theme->value);
		theme->value.x = (theme->value.svg_width - extents.width)/2;
		theme->theme->PrintText(&theme->value);
	}
	theme->bg->draw();
	Surface(theme->theme->getCurrent()).draw();
}


#include <screen_configuration.h>

CScreenConfiguration::CScreenConfiguration(std::string const& name, unsigned int width, unsigned int height):
  CScreen(name, width, height)
{
	configuration.push_back(new CConfigurationFullscreen());
	configuration.push_back(new CConfigurationAudioVolume());
	selected=0;
}

CScreenConfiguration::~CScreenConfiguration()
{
	for(unsigned int i = 0; i < configuration.size(); i++)
	  delete configuration[i];
}

void CScreenConfiguration::enter()
{
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	theme = new CThemeConfiguration(m_width, m_height);
	bg_texture = sm->getVideoDriver()->initSurface(theme->bg->getSDLSurface());
}

void CScreenConfiguration::exit()
{
	delete theme;
}

void CScreenConfiguration::manageEvent(SDL_Event event)
{
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	SDLMod modifier;
	if (event.type == SDL_KEYDOWN) {
		int key = event.key.keysym.sym;
		if (key == SDLK_ESCAPE || key == SDLK_q) CScreenManager::getSingletonPtr()->activateScreen("Intro");
		else if (key == SDLK_SPACE) sm->getAudio()->togglePause();
		else if (key == SDLK_UP && selected > 0) --selected;
		else if (key == SDLK_DOWN && selected + 1 < configuration.size()) ++selected;
		else if (key == SDLK_LEFT && !configuration[selected]->isFirst()) configuration[selected]->setPrevious();
		else if (key == SDLK_RIGHT && !configuration[selected]->isLast()) configuration[selected]->setNext();
	}
}

void CScreenConfiguration::draw()
{
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	theme->theme->clear();
	cairo_text_extents_t extents;

	// Draw the "Order by" text
	{
		theme->item.text = (char*)configuration[selected]->getDescription();
		extents = theme->theme->GetTextExtents(theme->item);
		theme->item.x = (theme->item.svg_width - extents.width)/2;
		theme->theme->PrintText(&theme->item);

		theme->value.text = configuration[selected]->getValue();
		extents = theme->theme->GetTextExtents(theme->value);
		theme->value.x = (theme->value.svg_width - extents.width)/2;
		theme->theme->PrintText(&theme->value);
	}

	sm->getVideoDriver()->drawSurface(bg_texture);
	sm->getVideoDriver()->drawSurface(theme->theme->getCurrent());
}

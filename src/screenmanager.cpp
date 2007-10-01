#include <screen.h>
#include <stdexcept>

#ifndef THEMES_DIR
#define THEMES_DIR "/usr/local/share/ultrastar-ng/themes/"
#endif


template<> CScreenManager* CSingleton<CScreenManager>::ms_CSingleton = NULL;

CScreenManager::CScreenManager(unsigned int width, unsigned int height, std::string const& theme):
  m_finished(false),
  currentScreen(),
  screen(),
  audio(),
  songs(),
  songId(),
  videoDriver(),
  m_fullscreen(),
  m_width(width),
  m_height(height),
  m_difficulty(),
  m_theme(theme)
{}

CScreenManager::~CScreenManager()
{
	delete audio;
	delete songs;
	for(unsigned int i = 0 ; i < screens.size() ; i++)
		delete screens[i];
}

void CScreenManager::activateScreen(const char * name) {
	for(unsigned int i = 0 ; i < screens.size() ; i++)
		if(!strcmp(screens[i]->getName(),name)) {
			if(currentScreen != NULL)
				currentScreen->exit();
			currentScreen=screens[i];
			currentScreen->enter();
		}
}

CScreen * CScreenManager::getScreen(const char * name) {
	for(unsigned int i = 0 ; i < screens.size() ; i++)
		if(!strcmp(screens[i]->getName(),name))
			return screens[i];
	return NULL;
}

std::string CScreenManager::getThemePathFile(std::string const& file)
{
	if (m_theme.empty()) throw std::logic_error("CScreenManager::getThemePathFile(): m_theme is empty");
	return (*m_theme.rbegin() == '/' ? m_theme : THEMES_DIR + m_theme + "/") + file;
}

void CScreenManager::setPreviousSongId()
{
	songId = (songId > 0 ? songId : songs->nbSongs()) - 1;
}

void CScreenManager::setNextSongId()
{
	songId = (songId + 1) % songs->nbSongs();
}

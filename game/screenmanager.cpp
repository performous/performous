#include "screen.hh"
#include <stdexcept>

#ifndef THEMES_DIR
#define THEMES_DIR "/usr/local/share/ultrastar-ng/themes/"
#endif


template<> CScreenManager* CSingleton<CScreenManager>::ms_CSingleton = NULL;

CScreenManager::CScreenManager(std::string const& theme):
  m_finished(false),
  currentScreen(),
  audio(),
  songs(),
  m_fullscreen(),
  m_theme(theme),
  m_ingameVolume(100),
  m_menuVolume(30)
{}

void CScreenManager::activateScreen(std::string const& name) {
	CScreen* s = getScreen(name);
	if (currentScreen) currentScreen->exit();
	currentScreen = s;
	currentScreen->enter();
}

CScreen* CScreenManager::getScreen(std::string const& name) {
	try {
		return &screens.at(name);
	} catch (boost::bad_ptr_container_operation&) {
		throw std::invalid_argument("Screen " + name + " does not exist");
	}
}

std::string CScreenManager::getThemePathFile(std::string const& file)
{
	if (m_theme.empty()) throw std::logic_error("CScreenManager::getThemePathFile(): m_theme is empty");
	return (*m_theme.rbegin() == '/' ? m_theme : THEMES_DIR + m_theme + "/") + file;
}


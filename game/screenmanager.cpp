#include "screen.hh"
#include <stdexcept>

template<> CScreenManager* CSingleton<CScreenManager>::ms_CSingleton = NULL;

CScreenManager::CScreenManager(std::string const& theme):
  m_finished(false),
  currentScreen(),
  audio(),
  songs(),
  m_fullscreen(),
  m_theme(theme)
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
	return m_theme + "/" + file;
}


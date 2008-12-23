#include "screen.hh"
#include <stdexcept>

template<> ScreenManager* Singleton<ScreenManager>::ms_Singleton = NULL;

ScreenManager::ScreenManager(std::string const& theme):
  m_finished(false),
  currentScreen(),
  m_theme(theme)
{}

void ScreenManager::activateScreen(std::string const& name) {
	Screen* s = getScreen(name);
	if (currentScreen) currentScreen->exit();
	currentScreen = s;
	currentScreen->enter();
}

Screen* ScreenManager::getScreen(std::string const& name) {
	try {
		return &screens.at(name);
	} catch (boost::bad_ptr_container_operation&) {
		throw std::invalid_argument("Screen " + name + " does not exist");
	}
}

std::string ScreenManager::getThemePathFile(std::string const& file) const
{
	if (m_theme.empty()) throw std::logic_error("ScreenManager::getThemePathFile(): m_theme is empty");
	return m_theme + "/" + file;
}


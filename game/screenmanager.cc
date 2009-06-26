#include "screen.hh"
#include <stdexcept>

template<> ScreenManager* Singleton<ScreenManager>::ms_Singleton = NULL;

ScreenManager::ScreenManager(): m_finished(false), currentScreen() {}

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


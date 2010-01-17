#include "screen.hh"

#include <stdexcept>

template<> ScreenManager* Singleton<ScreenManager>::ms_Singleton = NULL;

ScreenManager::ScreenManager(): m_finished(false), currentScreen() {}

void ScreenManager::activateScreen(std::string const& name) {
	newScreen = getScreen(name);
}

void ScreenManager::updateScreen() {
	if (!newScreen) return;
	if (currentScreen) currentScreen->exit();
	currentScreen = NULL;  // Exception safety, do not remove
	newScreen->enter();
	currentScreen = newScreen;
	newScreen = NULL;
}

Screen* ScreenManager::getScreen(std::string const& name) {
	try {
		return &screens.at(name);
	} catch (boost::bad_ptr_container_operation&) {
		throw std::invalid_argument("Screen " + name + " does not exist");
	}
}


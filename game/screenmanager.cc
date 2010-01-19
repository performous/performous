#include "screen.hh"
#include "fs.hh"

#include <stdexcept>

template<> ScreenManager* Singleton<ScreenManager>::ms_Singleton = NULL;

ScreenManager::ScreenManager(): m_finished(false), currentScreen(), m_messagePopup(0.0, 1.0), m_textMessage(getThemePath("message_text.svg")) {
	m_textMessage.dimensions.middle().screenTop(0.05);
}

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

void ScreenManager::drawFlashMessage() {
	double time = m_messagePopup.get();
	if (time == 0.0) return;
	bool haveToFadeIn = time <= (m_timeToFadeIn); // Is this fade in?
	bool haveToFadeOut = time >= (m_messagePopup.getTarget() - m_timeToFadeOut); // Is this fade out?
	float fadeValue = 1.0f;
	
	if (haveToFadeIn) { // Fade in
		fadeValue = float(time / m_timeToFadeIn); // Calculate animation value
	} else if (haveToFadeOut) { // Fade out
		fadeValue = float((m_messagePopup.getTarget() - time) / m_timeToFadeOut); // Calculate animation value
		if (time >= m_messagePopup.getTarget()) m_messagePopup.setTarget(0.0, true); // Reset if fade out finished
	}

	m_textMessage.draw(m_message, fadeValue); // Draw the message
	if (haveToFadeIn || haveToFadeOut) glColor3f(1.0f, 1.0f, 1.0f); // Reset alpha
}

void ScreenManager::flashMessage(std::string const& message, float fadeIn, float hold, float fadeOut) {
	m_message = message;
	m_timeToFadeIn = fadeIn;
	m_timeToShow = hold;
	m_timeToFadeOut = fadeOut;
	m_messagePopup.setTarget(fadeIn + hold + fadeOut);
	m_messagePopup.setValue(0.0);
}

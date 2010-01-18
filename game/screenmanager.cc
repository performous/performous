#include "screen.hh"
#include "fs.hh"

#include <stdexcept>

template<> ScreenManager* Singleton<ScreenManager>::ms_Singleton = NULL;

ScreenManager::ScreenManager(): m_finished(false), currentScreen(), m_messagePopup(0.0, 1.0), m_textMessage(getThemePath("message_text.svg")) {
	m_timeToFade = 1.0f;
	m_timeToShow = 3.0f;

	m_messagePopup.setTarget(100.0);
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

float ScreenManager::getFadeTime(){ return m_timeToFade; }
float ScreenManager::getShowTime(){ return m_timeToFade; }

bool ScreenManager::setFadeTime(float fadeTime){
	if (fadeTime * 2 > m_timeToShow){ //fade in + fade out
		m_timeToFade = m_timeToShow / 2;
		return false;
	} else {
		m_timeToFade = fadeTime;
		return true;
	}
}

bool ScreenManager::setShowTime(float showTime){
	if (showTime < m_timeToFade * 2){
		m_timeToShow = m_timeToFade / 2;
		return false;
	} else {
		m_timeToShow = showTime;
		return true;
	}
}

void ScreenManager::FlashMessages() {
	double time = m_messagePopup.get();
	if (time > 0.0){
		bool haveToFadeIn = time < (m_timeToFade);
		bool haveToFadeOut = time > (m_messagePopup.getTarget() - m_timeToFade);
		bool haveToStop = time > (m_messagePopup.getTarget() - 0.001);
		float fadeValue = 1.0f;
		if (haveToFadeOut){
			fadeValue = float((m_messagePopup.getTarget() - time) / m_timeToFade);
			glColor4f(1.0f, 1.0f, 1.0f, (fadeValue));
			
			if(haveToStop){
				m_messagePopup.setTarget(0.0, true);
			}
		}else if (haveToFadeIn){
			fadeValue = float(time / m_timeToFade);

			glColor4f(1.0f, 1.0f, 1.0f, fadeValue);
		}

		m_textMessage.draw(m_message, fadeValue);

		if (haveToFadeIn || haveToFadeOut) glColor3f(1.0f, 1.0f, 1.0f);
	}
}

void ScreenManager::FlashMessage(std::string const& message) {
	m_messagePopup.setTarget(m_timeToShow);
	m_messagePopup.setValue(0);
	m_message = message;
}

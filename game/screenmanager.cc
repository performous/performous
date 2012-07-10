#include "screen.hh"
#include "fs.hh"
#include "configuration.hh"
#include "glutil.hh"
#include "glmath.hh"
#include "util.hh"

#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <stdexcept>
#include <cstdlib>

template<> ScreenManager* Singleton<ScreenManager>::ms_Singleton = NULL;

ScreenManager::ScreenManager(Window& _window):
  m_window(_window), m_finished(false), newScreen(), currentScreen(),
  m_timeToFadeIn(), m_timeToFadeOut(), m_timeToShow(), m_message(),
  m_messagePopup(0.0, 1.0), m_textMessage(getThemePath("message_text.svg"), config["graphic/text_lod"].f()),
  m_loadingProgress(0.0f), m_logo(getThemePath("logo.svg")), m_logoAnim(0.0, 0.5)
{
	m_textMessage.dimensions.middle().center(-0.05);
}

void ScreenManager::activateScreen(std::string const& name) {
	newScreen = getScreen(name);
}

void ScreenManager::updateScreen() {
	if (!newScreen) return;
	Screen* s = newScreen;  // A local copy in case exit() or enter() want to change screens again
	newScreen = NULL;
	if (currentScreen) currentScreen->exit();
	currentScreen = NULL;  // Exception safety, do not remove
	s->enter();
	currentScreen = s;
}

Screen* ScreenManager::getScreen(std::string const& name) {
	try {
		return &screens.at(name);
	} catch (boost::bad_ptr_container_operation&) {
		throw std::invalid_argument("Screen " + name + " does not exist");
	}
}

void ScreenManager::prepareScreen() {
	getCurrentScreen()->prepare();
}

void ScreenManager::drawScreen() {
	getCurrentScreen()->draw();
	drawLogo();
	drawNotifications();
}

void ScreenManager::loading(std::string const& message, float progress) {
	// TODO: Create a better one, this is quite ugly
	flashMessage(message + " " + boost::lexical_cast<std::string>(int(round(progress*100))) + "%", 0.0f, 1.0f, 1.0f);
	m_loadingProgress = progress;
	m_window.blank();
	m_window.render(boost::bind(&ScreenManager::drawLoading, this));
	m_window.swap();
}

void ScreenManager::drawLoading() {
	drawLogo();
	drawNotifications();
	const int maxi = 20;
	const float x = 0.3;
	const float spacing = 0.01;
	const float sq_size = (2*x - (maxi-1)*spacing) / maxi;
	for (int i = 0; i <= m_loadingProgress * maxi; ++i) {
		ColorTrans c(Color(0.2f, 0.7f, 0.7f, (m_loadingProgress + 1)*0.5f));
		UseShader shader(getShader("color"));
		glutil::Square(-x + i * (sq_size + spacing), 0, sq_size/2, true);
	}
}

void ScreenManager::fatalError(std::string const& message) {
	dialog("FATAL ERROR\n\n" + message);
	m_window.blank();
	m_window.render(boost::bind(&ScreenManager::drawNotifications, this));
	m_window.swap();
	boost::thread::sleep(now() + 4.0);
}

void ScreenManager::flashMessage(std::string const& message, float fadeIn, float hold, float fadeOut) {
	m_message = message;
	m_timeToFadeIn = fadeIn;
	m_timeToShow = hold;
	m_timeToFadeOut = fadeOut;
	m_messagePopup.setTarget(fadeIn + hold + fadeOut);
	m_messagePopup.setValue(0.0);
}

void ScreenManager::dialog(std::string const& text) {
	m_dialog.reset(new Dialog(text));
}

bool ScreenManager::closeDialog() {
	bool ret = m_dialog;
	m_dialog.reset();
	return ret;
}

void ScreenManager::drawLogo() {
	double v = 0.5 - 0.5 * std::cos(M_PI * m_logoAnim.get());
	m_logo.dimensions.fixedHeight(0.1).left(-0.45).screenTop(-0.1 + 0.11 * v);
	m_logo.draw();
}

void ScreenManager::drawNotifications() {
	double time = m_messagePopup.get();
	if (time != 0.0) {
		bool haveToFadeIn = time <= (m_timeToFadeIn); // Is this fade in?
		bool haveToFadeOut = time >= (m_messagePopup.getTarget() - m_timeToFadeOut); // Is this fade out?
		float fadeValue = 1.0f;

		if (haveToFadeIn) { // Fade in
			fadeValue = float(time / m_timeToFadeIn); // Calculate animation value
		} else if (haveToFadeOut) { // Fade out
			fadeValue = float((m_messagePopup.getTarget() - time) / m_timeToFadeOut); // Calculate animation value
			if (time >= m_messagePopup.getTarget()) m_messagePopup.setTarget(0.0, true); // Reset if fade out finished
		}

		ColorTrans c(Color(1.0, 1.0, 1.0, fadeValue));
		m_textMessage.draw(m_message); // Draw the message
	}
	// Dialog
	if (m_dialog) m_dialog->draw();
}

#include "game.hh"
#include "screen.hh"
#include "audio.hh"
#include "fs.hh"
#include "configuration.hh"
#include "glutil.hh"
#include "songs.hh"
#include "util.hh"
#include "graphic/color_trans.hh"

#include <thread>
#include <stdexcept>
#include <cstdlib>

Game::Game(Window& _window, Songs& _songs):
  m_window(_window),
  m_messagePopup(0.0, 1.0), m_textMessage(findFile("message_text.svg"), config["graphic/text_lod"].f()),
  m_loadingProgress(0.0f), m_logo(findFile("logo.svg")), m_logoAnim(0.0, 0.5)
{
	m_webserver = std::make_unique<WebServer>(*this, _songs);
	m_textMessage.dimensions.middle().center(-0.05f);
}

void Game::activateScreen(std::string const& name) {
	newScreen = getScreen(name);
}

void Game::updateScreen() {
	if (!newScreen) return;
	Screen* s = newScreen;  // A local copy in case exit() or enter() want to change screens again
	newScreen = nullptr;
	if (currentScreen) currentScreen->exit();
	currentScreen = nullptr;  // Exception safety, do not remove
	s->enter();
	currentScreen = s;
}

Screen* Game::getScreen(std::string const& name) {
	auto it = screens.find(name);
	if (it != screens.end()){
		return it->second.get();
	}
	throw std::invalid_argument("Screen " + name + " does not exist");
}

void Game::prepareScreen() {
	getCurrentScreen()->prepare();
}

void Game::drawScreen() {
	getCurrentScreen()->draw();
	drawLogo();
	drawNotifications();
}

void Game::restartWebServer() {
#ifdef USE_WEBSERVER
	m_webserver->restartServer();
#endif
}

void Game::loading(std::string const& message, float progress) {
	// TODO: Create a better one, this is quite ugly
	flashMessage(message + " " + std::to_string(int(round(progress*100))) + "%", 0.0f, 0.5f, 0.2f);
	m_loadingProgress = progress;
	m_window.blank();
	m_window.render(*this, [this] { drawLoading(); });
	m_window.swap();
}

void Game::drawLoading() {
	drawLogo();
	drawNotifications();
	const int maxi = 20;
	const float x = 0.3f;
	const float spacing = 0.01f;
	const float sq_size = (2*x - (maxi-1)*spacing) / maxi;
	for (float f = 0.0f; f <= m_loadingProgress * maxi; ++f) {
		ColorTrans c(m_window, Color(0.2f, 0.7f, 0.7f, (m_loadingProgress + 1.0f)*0.5f));
		UseShader shader(getShader(m_window, "color"));
		float cx = -x + f * (sq_size + spacing);
		float cy = 0;
		float r = sq_size/2;
		glutil::VertexArray va;
		va.vertex(cx - r, cy + r);
		va.vertex(cx + r, cy + r);
		va.vertex(cx - r, cy - r);
		va.vertex(cx + r, cy - r);
		va.draw(GL_TRIANGLE_STRIP);
	}
}

void Game::fatalError(std::string const& message) {
	dialog("FATAL ERROR\n\n" + message);
	m_window.blank();
	m_window.render(*this, [this] { drawNotifications(); });
	m_window.swap();
	std::this_thread::sleep_for(4s);
}

void Game::flashMessage(std::string const& message, float fadeIn, float hold, float fadeOut) {
	m_message = message;
	m_timeToFadeIn = fadeIn;
	m_timeToShow = hold;
	m_timeToFadeOut = fadeOut;
	m_messagePopup.setTarget(fadeIn + hold + fadeOut);
	m_messagePopup.setValue(0.0);
}

void Game::dialog(std::string const& text) {
	m_dialogTimeOut.setValue(10);
	m_dialog = std::make_unique<Dialog>(text);
}

bool Game::closeDialog() {
	bool ret = !!m_dialog;
	m_dialog.reset();
	return ret;
}

void Game::drawLogo() {
	m_logo.dimensions.fixedHeight(0.1f).left(-0.45f).screenTop(-0.1f + 0.11f * static_cast<float>(smoothstep(m_logoAnim.get())));
	m_logo.draw(m_window);
}

void Game::drawNotifications() {
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

		ColorTrans c(m_window, Color::alpha(fadeValue));
		m_textMessage.draw(m_window, m_message); // Draw the message
	}
	// Dialog
	if (m_dialog) {
		m_dialog->draw(m_window);
		if(m_dialogTimeOut.get() == 0) closeDialog();
	}
}

void Game::finished() {
#ifdef USE_WEBSERVER
	m_webserver->forceQuitServer();
#endif
	m_finished = true;
}

Game::~Game() {
	if (currentScreen) currentScreen->exit();
}

bool Game::isFinished() {
	return m_finished;
}

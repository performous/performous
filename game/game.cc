#include "game.hh"
#include "screen.hh"
#include "audio.hh"
#include "fs.hh"
#include "configuration.hh"
#include "glutil.hh"
#include "util.hh"

#include <thread>
#include <stdexcept>
#include <cstdlib>

template<> Game* Singleton<Game>::ms_Singleton = nullptr;

Game::Game(Window& _window, Audio& _audio):
  m_audio(_audio), m_window(_window), m_finished(false), newScreen(), currentScreen(), currentPlaylist(),
  m_timeToFadeIn(), m_timeToFadeOut(), m_timeToShow(), m_message(),
  m_messagePopup(0.0, 1.0), m_textMessage(findFile("message_text.svg"), config["graphic/text_lod"].f()),
  m_loadingProgress(0.0f), m_logo(findFile("logo.svg")), m_logoAnim(0.0, 0.5)
{
	m_textMessage.dimensions.middle().center(-0.05);
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
	const double x = 0.3;
	const double spacing = 0.01;
	const double sq_size = (2*x - (maxi-1)*spacing) / maxi;
	for (int i = 0; i <= m_loadingProgress * maxi; ++i) {
		ColorTrans c(Color(0.2, 0.7, 0.7, (m_loadingProgress + 1.0)*0.5));
		UseShader shader(getShader("color"));
		float cx = -x + i * (sq_size + spacing);
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
	m_logo.dimensions.fixedHeight(0.1).left(-0.45).screenTop(-0.1 + 0.11 * smoothstep(m_logoAnim.get()));
	m_logo.draw();
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

		ColorTrans c(Color::alpha(fadeValue));
		m_textMessage.draw(m_message); // Draw the message
	}
	// Dialog
	if (m_dialog) {
		m_dialog->draw();
		if(m_dialogTimeOut.get() == 0) closeDialog();
	}
}

void Game::finished() {
	m_finished = true;
}

Game::~Game() {
	if (currentScreen) currentScreen->exit();
}

bool Game::isFinished() {
	return m_finished;
}

void Game::restartAudio() {
	m_audio.restart();
	m_audio.playMusic(findFile("menu.ogg"), true); // Start music again
}

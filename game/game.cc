#include "game.hh"
#include "screen.hh"
#include "audio.hh"
#include "fs.hh"
#include "configuration.hh"
#include "graphic/glutil.hh"
#include "util.hh"
#include "graphic/color_trans.hh"
#include "theme/theme_loader.hh"

#include <thread>
#include <stdexcept>
#include <cstdlib>

Game::Game(Window& window) :
	m_window(window),
	m_textMessage(findFile("message_text.svg"), config["graphic/text_lod"].f()),
	m_logo(m_textureManager.get(findFile("logo.svg")))
{
	m_textMessage.dimensions.middle().center(-0.05f);

	m_constantValueProvider->setValue("screen.left", -0.5f);
	m_constantValueProvider->setValue("screen.center", 0.f);
	m_constantValueProvider->setValue("screen.right", 0.5f);
	m_constantValueProvider->setValue("screen.top", -0.5f * virtH());
	m_constantValueProvider->setValue("screen.middle", 0.f);
	m_constantValueProvider->setValue("screen.bottom", 0.5f *virtH());
}

void Game::loadTheme() {
	auto loader = ThemeLoader(m_constantValueProvider);
	auto theme = loader.load<Theme>("Global");

	if (theme && theme->getName() != m_currentThemeName) {
		m_drawLogo = theme->drawlogo;

		setImages(std::move(theme->images));

		m_currentThemeName = theme->getName();
	}
}

void Game::activateScreen(std::string const& name) {
	newScreen = getScreen(name);
}

void Game::updateScreen() {
	if (!newScreen)
		return;
	if (currentScreen && currentScreen->getName() == newScreen->getName())
		return;

	auto const from = (currentScreen ? currentScreen->getName() : std::string{});
	auto const to = newScreen->getName();

	auto* screen = newScreen;  // A local copy in case exit() or enter() want to change screens again
	newScreen = nullptr;
	if (currentScreen)
		currentScreen->exit();
	currentScreen = nullptr;  // Exception safety, do not remove
	screen->enter();
	currentScreen = screen;

	loadTheme();

	getEventManager().sendEvent("onleave", EventParameter({ {"screen", {from}}, {"next", {to}} }));
	getEventManager().sendEvent("onenter", { { {"screen", {to}}, {"previous", {from}} } });
}

Screen* Game::getScreen(std::string const& name) {
	auto it = screens.find(name);
	if (it != screens.end()) {
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
	drawImages();
	drawNotifications();
}

/// Reload OpenGL resources (after fullscreen toggle etc)
void Game::reloadGL() {
	if (currentScreen)
		currentScreen->reloadGL();
}

void Game::loading(std::string const& message, float progress) {
	// TODO: Create a better one, this is quite ugly
	flashMessage(message + " " + std::to_string(int(round(progress * 100))) + "%", 0.0f, 0.5f, 0.2f);
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
	const float sq_size = (2 * x - (maxi - 1) * spacing) / maxi;
	for (float f = 0.0f; f <= m_loadingProgress * maxi; ++f) {
		ColorTrans c(m_window, Color(0.2f, 0.7f, 0.7f, (m_loadingProgress + 1.0f) * 0.5f));
		UseShader shader(m_window.getShader("color"));
		float cx = -x + f * (sq_size + spacing);
		float cy = 0;
		float r = sq_size / 2;
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
	if (m_drawLogo) {
		m_logo.dimensions.fixedHeight(0.1f).left(-0.45f).screenTop(-0.1f + 0.11f * static_cast<float>(smoothstep(m_logoAnim.get())));
		m_logo.draw(m_window);
	}
}

void Game::drawImages() {
	getCurrentScreen()->drawImages(m_images);
}

void Game::setImages(std::vector<Theme::Image>&& images) {
	m_images = std::move(images);
}

Theme::Image* Game::findImage(std::string const& id) {
	for (auto& image : m_images)
		if (image.id == id)
			return &image;

	return nullptr;
}

EventManager& Game::getEventManager() {
	return m_eventManager;
}

TextureManager& Game::getTextureManager()
{
	return m_textureManager;
}

ConstantValueProviderPtr Game::getConstantValueProvider() const {
	return m_constantValueProvider;
}


void Game::drawNotifications() {
	double time = m_messagePopup.get();
	if (time != 0.0) {
		bool haveToFadeIn = time <= (m_timeToFadeIn); // Is this fade in?
		bool haveToFadeOut = time >= (m_messagePopup.getTarget() - m_timeToFadeOut); // Is this fade out?
		float fadeValue = 1.0f;

		if (haveToFadeIn) { // Fade in
			fadeValue = float(time / m_timeToFadeIn); // Calculate animation value
		}
		else if (haveToFadeOut) { // Fade out
			fadeValue = float((m_messagePopup.getTarget() - time) / m_timeToFadeOut); // Calculate animation value
			if (time >= m_messagePopup.getTarget()) m_messagePopup.setTarget(0.0, true); // Reset if fade out finished
		}

		ColorTrans c(m_window, Color::alpha(fadeValue));
		m_textMessage.draw(m_window, m_message); // Draw the message
	}
	// Dialog
	if (m_dialog) {
		m_dialog->draw(m_window);
		if (m_dialogTimeOut.get() == 0) closeDialog();
	}
}

void Game::finished() {
	m_finished = true;
}

Game::~Game() {
	if (currentScreen)
		currentScreen->exit();
}

/// Adds a screen to the manager
void Game::addScreen(std::unique_ptr<Screen> s) {
	screens.insert(std::make_pair(s->getName(), std::move(s)));
}

bool Game::isFinished() {
	return m_finished;
}

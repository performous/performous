#pragma once

#include <memory>
#include <string>

#include "animvalue.hh"
#include "graphic/opengl_text.hh"
#include "graphic/texture_manager.hh"
#include "graphic/window.hh"
#include "dialog.hh"
#include "playlist.hh"
#include "graphic/fbo.hh"
#include "audio.hh"
#include "screen.hh"
#include "i18n.hh"

class Game {
  public:
	Game(Window& window);
	~Game();
	/// Adds a screen to the manager
	void addScreen(std::unique_ptr<Screen> s) {
		screens.insert(std::make_pair(s->getName(), std::move(s)));
	}
	/// Switches active screen
	void activateScreen(std::string const& name);
	/// Does actual switching of screens (if necessary)
	void updateScreen();
	/// Prepare (slow loading operations) of the current screen for rendering
	void prepareScreen();
	/// Draws the current screen and possible transition effects
	void drawScreen();
	/// Reload OpenGL resources (after fullscreen toggle etc)
	void reloadGL() { if (currentScreen) currentScreen->reloadGL(); }
	/// Returns pointer to current Screen
	Screen* getCurrentScreen() { return currentScreen; }
	/// Returns pointer to Screen for given name
	Screen* getScreen(std::string const& name);
	/// Returns a reference to the window
	Window& window() { return m_window; }

	/// Draw a loading progress indication
	void loading(std::string const& message, float progress);
	/// Internal rendering function for loading indicator
	void drawLoading();
	/// Draw an error notification and quit
	void fatalError(std::string const& message);
	/// Set a message to flash in current screen
	void flashMessage(std::string const& message, float fadeIn=0.5f, float hold=1.5f, float fadeOut=1.0f);
	/// Create a new dialog message
	void dialog(std::string const& text);
	/// Close dialog and return true if it was opened in the first place
	bool closeDialog();
	/// Returns true if dialog is open
	bool isDialogOpen() { return !!m_dialog; }
	/// Draw dialogs & flash messages, called automatically by drawScreen
	void drawNotifications();

	/// Sets finished to true
	void finished();
	/// Returns finished state
	bool isFinished();
	/// Show performous logo
	void showLogo(bool show = true) { m_logoAnim.setTarget(show ? 1.0 : 0.0); }
	/// Draw the logo
	void drawLogo();
	void drawImages();
	void setImages(std::vector<Theme::Image>&&);
	///global playlist access
	PlayList& getCurrentPlayList() { return currentPlaylist; }
#ifdef USE_WEBSERVER
	void notificationFromWebserver(std::string message) { m_webserverMessage = message; }
	std::string subscribeWebserverMessages() { return m_webserverMessage; }
#endif

	Window& getWindow() { return m_window; }
	Audio& getAudio() { return m_audio; }

	TextureManager& getTextureManager();

private:
	Window& m_window;
	Audio m_audio;

public:
	input::Controllers controllers;

private:
	TextureManager m_textureManager;
	bool m_finished = false;
	typedef std::map<std::string, std::unique_ptr<Screen>> screenmap_t;
	screenmap_t screens;
	Screen* newScreen = nullptr;
	Screen* currentScreen = nullptr;
	PlayList currentPlaylist;
	// Flash messages members
	float m_timeToFadeIn;
	float m_timeToFadeOut;
	float m_timeToShow;
	std::string m_message;
	AnimValue m_messagePopup{ 0.0, 1.0 };
	SvgTxtTheme m_textMessage;
	float m_loadingProgress{ 0.0f };
	Texture m_logo;
	AnimValue m_logoAnim{ 0.0, 0.5 };
	AnimValue m_dialogTimeOut;
	// Dialog members
	std::unique_ptr<Dialog> m_dialog;
	std::vector<Theme::Image> m_images;
#ifdef USE_WEBSERVER
	std::string m_webserverMessage = "Trying to connect to webserver";
#endif
};

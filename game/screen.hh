#pragma once

#include "animvalue.hh"
#include "controllers.hh"
#include "dialog.hh"
#include "fbo.hh"
#include "opengl_text.hh"
#include "playlist.hh"
#include "singleton.hh"
#include "video_driver.hh"
#include "webserver.hh"

#include <SDL2/SDL_events.h>

#include <map>
#include <memory>
#include <string>


class Audio;

/// Abstract Class for screens
class Screen {
  public:
	/// counstructor
	Screen(std::string const& name): m_name(name) {}
	virtual ~Screen() {}
	/// Event handler for navigation events
	virtual void manageEvent(input::NavEvent const& event) = 0;
	/// Event handler for SDL events
	virtual void manageEvent(SDL_Event) {}
	/// prepare screen for drawing
	virtual void prepare() {}
	/// draws screen
	virtual void draw() = 0;
	/// enters screen
	virtual void enter() = 0;
	/// exits screen
	virtual void exit() = 0;
	/// reloads OpenGL textures but avoids restarting music etc.
	virtual void reloadGL() { exit(); enter(); }
	/// returns screen name
	std::string getName() const { return m_name; }
  private:
	std::string m_name;
};

/// Manager for screens and Playlist
/** manages screens
 * @see Singleton
 */
class Game: public Singleton <Game> {
  public:
	/// constructor
	Game(Window& window, Audio& audio, Songs& songs);
	~Game();
	/// Adds a screen to the manager
	void addScreen(std::unique_ptr<Screen> s) { 
		std::string screenName = s.get()->getName(); 
		std::pair<std::string, std::unique_ptr<Screen>> kv = std::make_pair(screenName, std::move(s));
		screens.insert(std::move(kv));
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

	/// Restarts Audio subsystem and begins playing menu music.
	void restartAudio();

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
	/// global playlist access
	PlayList& getCurrentPlayList() { return currentPlaylist; }
#ifdef USE_WEBSERVER
	void notificationFromWebserver(std::string message) { m_webserverMessage = message; }
	std::string subscribeWebserverMessages() { return m_webserverMessage; }
#endif

private:
	Audio& m_audio;
	Window& m_window;
	std::unique_ptr<WebServer> m_webserver;

public:
	input::Controllers controllers;

private:
	bool m_finished;
	typedef std::map<std::string, std::unique_ptr<Screen>> screenmap_t;
	screenmap_t screens;
	Screen* newScreen;
	Screen* currentScreen;
	PlayList currentPlaylist;
	// Flash messages members
	float m_timeToFadeIn;
	float m_timeToFadeOut;
	float m_timeToShow;
	std::string m_message;
	AnimValue m_messagePopup;
	SvgTxtTheme m_textMessage;
	float m_loadingProgress;
	Texture m_logo;
	AnimValue m_logoAnim;
	AnimValue m_dialogTimeOut;
	// Dialog members
	std::unique_ptr<Dialog> m_dialog;
#ifdef USE_WEBSERVER
	std::string m_webserverMessage = "Trying to connect to webserver";
#endif
};

// Declaration for the singleton pointer defined in game.cc
template<> Game* Singleton<Game>::ms_Singleton;


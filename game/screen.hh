#pragma once

#include "singleton.hh"
#include "animvalue.hh"
#include "opengl_text.hh"
#include "video_driver.hh"
#include "dialog.hh"
#include "fbo.hh"
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/scoped_ptr.hpp>
#include <SDL.h>
#include <string>

/// Abstract Class for screens
class Screen {
  public:
	/// counstructor
	Screen(std::string const& name): m_name(name) {}
	virtual ~Screen() {}
	/// eventhandler
	virtual void manageEvent(SDL_Event event) = 0;
	/// draws screen
	virtual void draw() = 0;
	/// enters screen
	virtual void enter() = 0;
	/// exits screen
	virtual void exit() = 0;
	/// returns screen name
	std::string getName() const { return m_name; }

  private:
	std::string m_name;
};

/// Manager for screens
/** manages screens
 * @see Singleton
 */
class ScreenManager: public Singleton <ScreenManager> {
  public:
	/// constructor
	ScreenManager(Window& window);
	~ScreenManager() { if (currentScreen) currentScreen->exit(); }
	/// Adds a screen to the manager
	void addScreen(Screen* s) { std::string tmp = s->getName(); screens.insert(tmp, s); };
	/// Switches active screen
	void activateScreen(std::string const& name);
	/// Does actual switching of screens (if necessary)
	void updateScreen();
	/// Draws the current screen and possible transition effects
	void drawScreen();
	/// Returns pointer to current Screen
	Screen* getCurrentScreen() { return currentScreen; };
	/// Returns pointer to Screen for given name
	Screen* getScreen(std::string const& name);
	/// Returns a reference to the window
	Window& window() { return m_window; };

	/// Draw a loading progress indication
	void loading(std::string const& message, float progress);
	/// Draw an error notification and quit
	void fatalError(std::string const& message);
	/// Set a message to flash in current screen
	void flashMessage(std::string const& message, float fadeIn=0.5f, float hold=1.5f, float fadeOut=1.0f);
	/// Create a new dialog message
	void dialog(std::string const& text);
	/// Close dialog and return true if it was opened in the first place
	bool closeDialog();
	/// Returns true if dialog is open
	bool isDialogOpen() { return m_dialog; }
	/// Draw dialogs & flash messages, called automatically by drawScreen
	void drawNotifications();

	/// Sets finished to true
	void finished() { m_finished = true; };
	/// Returns finished state
	bool isFinished() { return m_finished; };

	void showLogo(bool show = true) { m_logoAnim.setTarget(show ? 1.0 : 0.0); }
	void drawLogo();

  private:
	Window& m_window;
	bool m_finished;
	typedef boost::ptr_map<std::string, Screen> screenmap_t;
	screenmap_t screens;
	Screen* newScreen;
	Screen* currentScreen;
	// Flash messages members
	float m_timeToFadeIn;
	float m_timeToFadeOut;
	float m_timeToShow;
	std::string m_message;
	AnimValue m_messagePopup;
	SvgTxtTheme m_textMessage;
	Surface m_logo;
	AnimValue m_logoAnim;
	// Dialog members
	boost::scoped_ptr<Dialog> m_dialog;
};


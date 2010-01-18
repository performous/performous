#pragma once

#include "singleton.hh"
#include "animvalue.hh"
#include "opengl_text.hh"
#include <boost/ptr_container/ptr_map.hpp>
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
	ScreenManager();
	/// adds a screen to the manager
	void addScreen(Screen* s) { std::string tmp = s->getName(); screens.insert(tmp, s); };
	/// Switches active screen
	void activateScreen(std::string const& name);
	/// Does actual switching of screens (if necessary)
	void updateScreen();
	/// returns pointer to current Screen
	Screen* getCurrentScreen() { return currentScreen; };
	/// returns pointer to Screen for given name
	Screen* getScreen(std::string const& name);
	
	/// Get time to fade in/out the message
	float getFadeTime();
	/// Get time the message have to been showned
	float getShowTime();
	/// Set time to fade in/out the message
	bool setFadeTime(float fadeTime);
	/// Set time the message have to been showned
	bool setShowTime(float showTime);
	/// Set a messag to flash in current screen
	void FlashMessage(std::string const& name);
	/// Flash messages in current screen
	void FlashMessages();

	/// sets finished to true
	void finished() { m_finished=true; };
	/// returns finished state
	bool isFinished() { return m_finished; };

  private:
	bool m_finished;
	float m_timeToFade;
	float m_timeToShow;
	typedef boost::ptr_map<std::string, Screen> screenmap_t;
	screenmap_t screens;
	Screen* newScreen;
	Screen* currentScreen;
	AnimValue m_messagePopup;
	SvgTxtTheme m_textMessage;
	std::string m_message;
};


#ifndef __SCREEN_H__
#define __SCREEN_H__

#include <boost/ptr_container/ptr_map.hpp>
#include <boost/scoped_ptr.hpp>
#include "singleton.hh"
#include "audio.hh"
#include "record.hh"
#include "songs.hh"
#include <SDL/SDL.h>
#include <iostream>
#include <string>
#include <vector>

/// Abstract Class for screens
class CScreen {
  public:
	/// counstructor
	CScreen(std::string const& name): m_name(name) {}
	virtual ~CScreen() {}
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
 * @see CSingleton 
 */
class CScreenManager: public CSingleton <CScreenManager> {
  public:
	/// constructor
	CScreenManager(std::string const& theme);
	/// adds a screen to the manager
	void addScreen(CScreen* s) { std::string tmp = s->getName(); screens.insert(tmp, s); };
	/// activates screen
	void activateScreen(std::string const& name);
	/// returns pointer to current CScreen
	CScreen* getCurrentScreen() { return currentScreen; };
	/// returns pointer to CScreen for given name
	CScreen* getScreen(std::string const& name);

	/// sets finished to true
	void finished() { m_finished=true; };
	/// returns finished state
	bool isFinished() { return m_finished; };

	/// returns themename
	std::string getThemeName() const { return m_theme; };
	/// returns theme path
	std::string getThemePathFile(std::string const& file) const;

  private:
	bool m_finished;
	typedef boost::ptr_map<std::string, CScreen> screenmap_t;
	screenmap_t screens;
	CScreen* currentScreen;
	std::string m_theme;
};

#endif

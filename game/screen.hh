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


class CScreen {
  public:
	CScreen(std::string const& name): m_name(name) {}
	virtual ~CScreen() {}
	virtual void manageEvent(SDL_Event event) = 0;
	virtual void draw() = 0;
	virtual void enter() = 0;
	virtual void exit() = 0;
	std::string getName() const { return m_name; }
  private:
	std::string m_name;
};

class CScreenManager: public CSingleton <CScreenManager> {
  public:
	CScreenManager(std::string const& theme);
	void addScreen(CScreen* s) { std::string tmp = s->getName(); screens.insert(tmp, s); };
	void activateScreen(std::string const& name);
	CScreen* getCurrentScreen() { return currentScreen; };
	CScreen* getScreen(std::string const& name);

	CAudio* getAudio() { return audio.get(); };
	void setAudio(CAudio* _audio) { audio.reset(_audio); };

	bool getFullscreenStatus() { return m_fullscreen; };
	void setFullscreenStatus(bool fullscreen) { m_fullscreen = fullscreen; };

	Songs* getSongs() { return songs.get(); };
	void setSongs(Songs* _songs) { songs.reset(_songs); };

	void finished() { m_finished=true; };
	bool isFinished() { return m_finished; };

	std::string getThemeName() const { return m_theme; };
	std::string getThemePathFile(std::string const& file) const;
  private:
	bool m_finished;
	typedef boost::ptr_map<std::string, CScreen> screenmap_t;
	screenmap_t screens;
	CScreen* currentScreen;
	boost::scoped_ptr<CAudio> audio;
	boost::scoped_ptr<Songs> songs;
	bool m_fullscreen;
	std::string m_theme;
};

#endif

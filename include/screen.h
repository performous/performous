#ifndef __SCREEN_H__
#define __SCREEN_H__

#include "../config.h"

#include <boost/ptr_container/ptr_map.hpp>
#include <boost/scoped_ptr.hpp>
#include <singleton.h>
#include <audio.h>
#include <record.h>
#include <songs.h>
#include <video_driver.h>
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

	void setSDLScreen(SDL_Surface* _screen) { screen = _screen; };
	SDL_Surface* getSDLScreen() { return screen; };

	CAudio* getAudio() { return audio.get(); };
	void setAudio(CAudio* _audio) { audio.reset(_audio); };

	CVideoDriver* getVideoDriver() { return videoDriver; };
	void setVideoDriver(CVideoDriver* _videoDriver) {videoDriver=_videoDriver; };

	bool getFullscreenStatus() { return m_fullscreen; };
	void setFullscreenStatus(bool fullscreen) { m_fullscreen = fullscreen; };

	Songs* getSongs() { return songs.get(); };
	void setSongs(Songs* _songs) { songs.reset(_songs); };

	void finished() { m_finished=true; };
	bool isFinished() { return m_finished; };

	std::string getThemeName() { return m_theme; };
	std::string getThemePathFile(std::string const& file);
  private:
	bool m_finished;
	typedef boost::ptr_map<std::string, CScreen> screenmap_t;
	screenmap_t screens;
	CScreen* currentScreen;
	SDL_Surface* screen;
	boost::scoped_ptr<CAudio> audio;
	boost::scoped_ptr<Songs> songs;
	CVideoDriver* videoDriver;
	bool m_fullscreen;
	std::string m_theme;
  public:
	unsigned int m_ingameVolume, m_menuVolume;
};

#endif

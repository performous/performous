#ifndef __SCREEN_H__
#define __SCREEN_H__

#include "../config.h"

#include <boost/ptr_container/ptr_map.hpp>
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
	CScreen(const char* name, unsigned int width, unsigned int height):
	  m_name(name), m_width(width), m_height(height) {}
	virtual ~CScreen() {}
	virtual void manageEvent(SDL_Event event) = 0;
	virtual void draw() = 0;
	virtual void enter() = 0;
	virtual void exit() = 0;
	std::string getName() const { return m_name; }
  protected:
	std::string m_name; // Must be set by each constructor
	unsigned int m_width; // Must be set by each constructor
	unsigned int m_height; // Must be set by each constructor
};

class CScreenManager: public CSingleton <CScreenManager> {
  public:
	CScreenManager(unsigned int width, unsigned int height, std::string const& theme);
	~CScreenManager();
	void addScreen(CScreen* s) { std::string tmp = s->getName(); screens.insert(tmp, s); };
	void activateScreen(std::string const& name);
	CScreen* getCurrentScreen() { return currentScreen; };
	CScreen* getScreen(std::string const& name);

	void setSDLScreen(SDL_Surface* _screen) { screen = _screen; };
	SDL_Surface* getSDLScreen() { return screen; };

	unsigned int getWidth() { return m_width; };
	unsigned int getHeight() { return m_height; };
	
	CAudio* getAudio() { return audio; };
	void setAudio(CAudio* _audio) {audio=_audio; };

	CVideoDriver* getVideoDriver() { return videoDriver; };
	void setVideoDriver(CVideoDriver* _videoDriver) {videoDriver=_videoDriver; };
	
	bool getFullscreenStatus() { return m_fullscreen; };
	void setFullscreenStatus(bool fullscreen) { m_fullscreen = fullscreen; };

	CSongs* getSongs() { return songs; };
	void setSongs(CSongs* _songs) { songs = _songs; };
	void setNextSongId();
	void setPreviousSongId();
	void setSongId(int _id) { songId = _id; };
	int getSongId() { return songId; };
	CSong* getSong() { return &(*songs)[songId]; };

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
	CAudio* audio;
	CSongs* songs;
	int songId;
	CVideoDriver* videoDriver;
	bool m_fullscreen;
	unsigned int m_width;
	unsigned int m_height;
	std::string m_theme;
};

#endif

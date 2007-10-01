#ifndef __SCREEN_H__
#define __SCREEN_H__

#include "../config.h"

#include <singleton.h>
#include <audio.h>
#include <record.h>
#include <songs.h>
#include <video_driver.h>
#include <string>
#include <vector>

class CScreen {
  public:
	CScreen() {}
	CScreen(const char* name, unsigned int width, unsigned int height):
	  screenName(name), width(width), height(height) {}
	virtual ~CScreen() {}
	virtual void manageEvent(SDL_Event event) = 0;
	virtual void draw() = 0;
	virtual void enter() = 0;
	virtual void exit() = 0;
	const char* getName() { return screenName; }
  protected:
	const char* screenName; // Must be set by each constructor
	unsigned int width; // Must be set by each constructor
	unsigned int height; // Must be set by each constructor
};

class CScreenManager : public CSingleton <CScreenManager> {
  public:
	CScreenManager(unsigned int width, unsigned int height, std::string const& theme);
	~CScreenManager();
	void addScreen(CScreen* screen) { 
		screens.push_back(screen);
		fprintf(stdout,"Adding screen \"%s\" to screen manager\n",screen->getName());
	};
	void activateScreen(const char* name);
	CScreen* getCurrentScreen() { return currentScreen; };
	CScreen* getScreen(const char* name);

	void setSDLScreen(SDL_Surface* _screen) { screen = _screen; };
	SDL_Surface* getSDLScreen() { return screen; };

	unsigned int getWidth() { return m_width; };
	unsigned int getHeight() { return m_height; };
	
	void setDifficulty(unsigned int difficulty) { m_difficulty = difficulty; };
	unsigned int getDifficulty() { return m_difficulty; };

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
	CSong* getSong() { return songs->getSong(songId); };

	void finished(void) { m_finished=true; };
	bool isFinished(void) { return m_finished; };

	std::string getThemeName() { return m_theme; };
	std::string getThemePathFile(std::string const& file);
  private:
	bool m_finished;
	std::vector<CScreen*> screens;
	CScreen* currentScreen;
	SDL_Surface* screen;
	CAudio* audio;
	CSongs* songs;
	int songId;
	CVideoDriver* videoDriver;
	bool m_fullscreen;
	unsigned int m_width;
	unsigned int m_height;
	unsigned int m_difficulty;
	std::string m_theme;
};

#endif

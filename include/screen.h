#ifndef __SCREEN_H__
#define __SCREEN_H__

#include "../config.h"

#include <singleton.h>
#include <audio.h>
#include <record.h>
#include <songs.h>
#include <video_driver.h>

class CScreen {
	public:
	CScreen(){};
	virtual ~CScreen() {};
	virtual void manageEvent( SDL_Event event )=0;
	virtual void draw( void )=0;
	virtual void enter( void )=0;
	virtual void exit( void )=0;
	const char * getName( void ) {return screenName;};
	protected:
	const char * screenName; // Must be set by each constructor
};

class CScreenManager : public CSingleton <CScreenManager>{
	public:
	CScreenManager( int width , int height , const char * songs_dir , const char * theme_name="lima");
	~CScreenManager();
	void addScreen( CScreen * screen ) { 
		screens.push_back(screen);
		fprintf(stdout,"Adding screen \"%s\" to screen manager\n",screen->getName());
	};
	void activateScreen(const char * name);
	CScreen * getCurrentScreen( void ) {return currentScreen;};
	CScreen * getScreen(const char * name);

	void setSDLScreen( SDL_Surface * _screen ) { screen = _screen;};
	SDL_Surface * getSDLScreen( void ) { return screen;};

	int getWidth( void ) {return width;};
	int getHeight( void ) {return height;};
	
	void setDifficulty( unsigned int _difficulty ) { difficulty=_difficulty;};
	unsigned int getDifficulty( void ) {return difficulty;};

	CAudio * getAudio( void ) {return audio;};
	void setAudio( CAudio * _audio ) {audio=_audio;};

	CRecord * getRecord( void ) {return record;};
	void setRecord( CRecord * _record ) {record=_record;};
	
	CVideoDriver * getVideoDriver( void ) {return videoDriver;};
	void setVideoDriver( CVideoDriver * _videoDriver ) {videoDriver=_videoDriver;};
	
	bool getFullscreenStatus( void ) {return fullscreen;};
	void setFullscreenStatus( bool _fullscreen ) {fullscreen=_fullscreen;};

	CSongs * getSongs( void ) {return songs;};
	void setSongs( CSongs * _songs ) {
		songs=_songs;
	};
	void setNextSongId( void );
	void setPreviousSongId( void );
	void setSongId( int _id ) {songId = _id; };
	int getSongId( void ) {return songId; };
	CSong * getSong( void ) {return songs->getSong(songId);};

	void finished(void) { m_finished=true; };
	bool isFinished(void) { return m_finished; };

	const char * getSongsDirectory( void ) { return m_songs_dir; };
	const char * getThemeName( void ) { return m_theme_name; };
	void getThemePathFile( char * dest , const char * file);
	private:
	const char * m_songs_dir;
	const char * m_theme_name;
	std::vector <CScreen *> screens;
	CScreen * currentScreen;
	SDL_Surface * screen;
	CAudio * audio;
	CRecord * record;
	CSongs * songs;
	CVideoDriver * videoDriver;
	bool fullscreen;
	int songId;
	bool m_finished;
	int width;
	int height;
	unsigned int difficulty;
};

#endif

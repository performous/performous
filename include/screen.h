#ifndef __SCREEN_H__
#define __SCREEN_H__

#include "../config.h"

#include <singleton.h>
#include <audio.h>
#include <record.h>
#include <songs.h>

class CScreen {
	public:
	CScreen(){};
	virtual ~CScreen() {};
	virtual void manageEvent( SDL_Event event )=0;
	virtual void draw( void )=0;
	char * getName( void ) {return screenName;};
	protected:
	char * screenName; // Must be set by each constructor
};

class CScreenManager : public CSingleton <CScreenManager>{
	public:
	CScreenManager( int width , int height );
	~CScreenManager();
	void addScreen( CScreen * screen ) { 
		screens.push_back(screen);
		fprintf(stdout,"Adding screen \"%s\" to screen manager\n",screen->getName());
	};
	void activateScreen(char * name) {
		for( unsigned int i = 0 ; i < screens.size() ; i++ )
			if( !strcmp(screens[i]->getName(),name) )
				currentScreen=screens[i];
	};
	CScreen * getCurrentScreen( void ) {return currentScreen;};
	CScreen * getScreen(char * name) {
		for( unsigned int i = 0 ; i < screens.size() ; i++ )
			if( !strcmp(screens[i]->getName(),name) )
				return screens[i];
	}

	void setSDLScreen( SDL_Surface * _screen ) { screen = _screen;};
	SDL_Surface * getSDLScreen( void ) { return screen;};

	int getWidth( void ) {return width;};
	int getHeight( void ) {return height;};

	CAudio * getAudio( void ) {return audio;};
	void setAudio( CAudio * _audio ) {audio=_audio;};

	CRecord * getRecord( void ) {return record;};
	void setRecord( CRecord * _record ) {record=_record;};

	CSongs * getSongs( void ) {return songs;};
	void setSongs( CSongs * _songs ) {
		if(songs)
			delete songs;
		songs=_songs;
	};
	void setSongId( int _id ) {songId = _id; };
	int getSongId( void ) {return songId; };
	CSong * getSong( void ) {return songs->getSong(songId);};

	void finished(void) { m_finished=true; };
	bool isFinished(void) { return m_finished; };
	private:
	std::vector <CScreen *> screens;
	CScreen * currentScreen;
	SDL_Surface * screen;
	CAudio * audio;
	CRecord * record;
	CSongs * songs;
	int songId;
	bool m_finished;
	int width;
	int height;
};

#endif

#include <screen.h>

template<> CScreenManager* CSingleton<CScreenManager>::ms_CSingleton = NULL;

CScreenManager::CScreenManager( int _width , int _height , char * _songs_dir , char * _theme_name )
{
	m_finished=false;
	audio = NULL;
	record = NULL;
	songs = NULL;
	songId = 0;
	width = _width;
	height = _height;
	m_songs_dir = _songs_dir;
	m_theme_name = _theme_name;
}

CScreenManager::~CScreenManager()
{
	delete audio;
	delete record;
	delete songs;
	for( unsigned int i = 0 ; i < screens.size() ; i++ )
		delete screens[i];
}

void CScreenManager::getThemePathFile( char * dest , char * file)
{
	sprintf(dest,"%s/%s/%s",THEMES_DIR,m_theme_name,file);
}

#include <screen.h>

template<> CScreenManager* CSingleton<CScreenManager>::ms_CSingleton = NULL;

CScreenManager::CScreenManager( int _width , int _height , const char * _songs_dir , const char * _theme_name )
{
	m_finished=false;
	audio = NULL;
	songs = NULL;
	currentScreen = NULL;
	songId = 0;
	fullscreen=false;
	width = _width;
	height = _height;
	m_songs_dir = _songs_dir;
	m_theme_name = _theme_name;
}

CScreenManager::~CScreenManager()
{
	delete audio;
	// delete record;
	delete songs;
	for( unsigned int i = 0 ; i < screens.size() ; i++ )
		delete screens[i];
}

void CScreenManager::activateScreen(const char * name) {
	for( unsigned int i = 0 ; i < screens.size() ; i++ )
		if( !strcmp(screens[i]->getName(),name) ) {
			if( currentScreen != NULL )
				currentScreen->exit();
			currentScreen=screens[i];
			currentScreen->enter();
		}
}

CScreen * CScreenManager::getScreen(const char * name) {
	for( unsigned int i = 0 ; i < screens.size() ; i++ )
		if( !strcmp(screens[i]->getName(),name) )
			return screens[i];
	return NULL;
}

void CScreenManager::getThemePathFile( char * dest , const char * file)
{
	if( m_theme_name[0] == '/' )
		sprintf(dest,"%s/%s",m_theme_name,file);
	else
		sprintf(dest,"%s/%s/%s",THEMES_DIR,m_theme_name,file);
}

void CScreenManager::setPreviousSongId( void )
{
	if( songId >0 )
		songId--;
	else
		songId = songs->nbSongs()-1;
}

void CScreenManager::setNextSongId( void )
{
	if( songId > songs->nbSongs()-2 )
		songId = 0;
	else
		songId++;
}

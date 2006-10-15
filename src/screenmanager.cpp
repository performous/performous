#include <screen.h>

template<> CScreenManager* CSingleton<CScreenManager>::ms_CSingleton = NULL;

CScreenManager::CScreenManager( int _width , int _height )
{
	m_finished=false;
	audio = NULL;
	record = NULL;
	songs = NULL;
	songId = 0;
	width = _width;
	height = _height;
}

CScreenManager::~CScreenManager()
{
	delete audio;
	delete record;
	delete songs;
	for( unsigned int i = 0 ; i < screens.size() ; i++ )
		delete screens[i];
}

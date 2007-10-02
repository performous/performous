#include <configuration.h>
#include <screen.h>

CConfigurationFullscreen::CConfigurationFullscreen()
{
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	fullscreen = sm->getFullscreenStatus();
	description="Fullscreen mode";
}
CConfigurationFullscreen::~CConfigurationFullscreen()
{
}
bool CConfigurationFullscreen::isLast()
{
	return false;
}
bool CConfigurationFullscreen::isFirst()
{
	return false;
}
void CConfigurationFullscreen::setNext()
{
	fullscreen = !fullscreen;
	apply();
}
void CConfigurationFullscreen::setPrevious()
{
	fullscreen = !fullscreen;
	apply();
}
char * CConfigurationFullscreen::getValue()
{
	if(fullscreen)
		return (char*)"Fullscreen";
	else
		return (char*)"Windowed";
}
void CConfigurationFullscreen::apply()
{
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	if( sm->getFullscreenStatus() != fullscreen ) {
		SDL_WM_ToggleFullScreen(sm->getSDLScreen());
		sm->setFullscreenStatus(fullscreen);
	}
}

/****************************************************************************/

CConfigurationAudioVolume::CConfigurationAudioVolume()
{
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	audioVolume = sm->getAudio()->getVolume();
	description="Audio Volume";
}
CConfigurationAudioVolume::~CConfigurationAudioVolume()
{
}
bool CConfigurationAudioVolume::isLast()
{
	return (audioVolume>=100);
}
bool CConfigurationAudioVolume::isFirst()
{
	return (audioVolume<=0);
}
void CConfigurationAudioVolume::setNext()
{
	audioVolume++;
	apply();
}
void CConfigurationAudioVolume::setPrevious()
{
	audioVolume--;
	apply();
}
char * CConfigurationAudioVolume::getValue()
{
	sprintf(value,"%d%%",audioVolume);
	return value;
}
void CConfigurationAudioVolume::apply()
{
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	sm->getAudio()->setVolume(audioVolume);
}

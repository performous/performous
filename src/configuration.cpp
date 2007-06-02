#include <configuration.h>
#include <screen.h>

CConfigurationFullscreen::CConfigurationFullscreen()
{
	fullscreen=false;
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
		return "Fullscreen";
	else
		return "Windowed";
}
void CConfigurationFullscreen::apply()
{
	CScreenManager * sm = CScreenManager::getSingletonPtr();
	if( sm->getFullscreenStatus() != fullscreen ) {
		SDL_WM_ToggleFullScreen(sm->getSDLScreen());
		sm->setFullscreenStatus(fullscreen);
	}
}

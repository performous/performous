#include <configuration.h>
#include <screen.h>
#include <boost/lexical_cast.hpp>

CConfigurationFullscreen::CConfigurationFullscreen():
  CConfiguration("Fullscreen Mode"), m_fs(CScreenManager::getSingletonPtr()->getFullscreenStatus())
{}
std::string CConfigurationFullscreen::getValue() const {
	return m_fs ? "Fullscreen" : "Windowed";
}
void CConfigurationFullscreen::apply() {
	CScreenManager* sm = CScreenManager::getSingletonPtr();
	if( sm->getFullscreenStatus() != m_fs ) {
		SDL_WM_ToggleFullScreen(sm->getSDLScreen());
		sm->setFullscreenStatus(m_fs);
	}
}

CConfigurationAudioVolume::CConfigurationAudioVolume(std::string const& title, unsigned int& volume):
  CConfiguration(title), m_volume(volume)
{}
void CConfigurationAudioVolume::setNext() {
	if (m_volume >= 100) return;
	m_volume++; apply();
}
void CConfigurationAudioVolume::setPrevious() {
	if (m_volume <= 0) return;
	m_volume--; apply();
}
std::string CConfigurationAudioVolume::getValue() const {
	return boost::lexical_cast<std::string>(m_volume);
}
void CConfigurationAudioVolume::apply() {
	CScreenManager::getSingletonPtr()->getAudio()->setVolume(m_volume);
}


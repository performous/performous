#include "configuration.hh"
#include "screen.hh"
#include <boost/lexical_cast.hpp>

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


#include "configuration.hh"
#include "screen.hh"
#include <boost/lexical_cast.hpp>
#include <algorithm>

CConfigurationAudioVolume::CConfigurationAudioVolume(std::string const& title, CAudio& audio, GetFunc get, SetFunc set):
  CConfiguration(title), m_audio(audio), m_get(get), m_set(set)
{}
void CConfigurationAudioVolume::setNext() { (m_audio.*m_set)(std::min(100u, (m_audio.*m_get)() + 1)); }
void CConfigurationAudioVolume::setPrevious() { (m_audio.*m_set)(std::max(0, int((m_audio.*m_get)()) - 1)); }

std::string CConfigurationAudioVolume::getValue() const {
	(m_audio.*m_set)((m_audio.*m_get)());  // Hack to have the volume set when the control is entered
	return boost::lexical_cast<std::string>((m_audio.*m_get)());
}


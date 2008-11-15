#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

#include "audio.hh"
#include <string>

class CConfiguration {
  public:
	CConfiguration(std::string const& description): m_description(description) {};
	virtual ~CConfiguration() {};
	virtual void setNext() = 0;
	virtual void setPrevious() = 0;
	std::string const& getDescription() const { return m_description; };
	virtual std::string getValue() const = 0;
  private:
	std::string m_description;
};

class CConfigurationAudioVolume: public CConfiguration {
  public:
	typedef unsigned int (Audio::*GetFunc)();
	typedef void (Audio::*SetFunc)(unsigned int);
	CConfigurationAudioVolume(std::string const& title, Audio& audio, GetFunc get, SetFunc set);
	void setNext();
	void setPrevious();
	std::string getValue() const;
  private:
	Audio& m_audio;
  	GetFunc m_get;
  	SetFunc m_set;
};

#endif

#pragma once
#ifndef PERFORMOUS_CONFIGURATION_HH
#define PERFORMOUS_CONFIGURATION_HH

#include "audio.hh"
#include "util.hh"
#include <boost/any.hpp>
#include <map>
#include <string>

extern std::map<std::string, boost::any> config;

void readConfigfile( const std::string &_configfile, const std::string &_schemafile);

template<int min, int max> class Integer {
  public:
	Integer(int value): m_value(clamp(value, min, max)) {}
	Integer& operator=(int value) { m_value = clamp(value, min, max); return *this; }
	int val() const { return m_value; }
	int& val() { return m_value; }
  private:
	int m_value;
};


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

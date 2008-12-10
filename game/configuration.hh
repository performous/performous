#pragma once
#ifndef PERFORMOUS_CONFIGURATION_HH
#define PERFORMOUS_CONFIGURATION_HH

#include "audio.hh"
#include "util.hh"
#include <boost/any.hpp>
#include <map>
#include <string>

class ConfigItem {
  public:
	ConfigItem();
	ConfigItem( std::string _type, bool _is_default);
	ConfigItem& operator++();
	ConfigItem& operator--();
	ConfigItem& operator+=(const int& right);
	ConfigItem& operator-=(const int& right);
	ConfigItem& operator+=(const float& right);
	ConfigItem& operator-=(const float& right);
	ConfigItem& operator+=(const double& right);
	ConfigItem& operator-=(const double& right);
	void set_short_description( std::string _short_desc );
	void set_long_description( std::string _long_desc );
	int &i(void);
	bool &b(void);
	double &f(void);
	std::string &s(void);
	std::vector<std::string> &sl(void);
	friend std::ostream& operator <<(std::ostream &os,const ConfigItem &obj);
  private:
	std::string type;
	std::string short_desc;
	std::string long_desc;

	bool is_default;

	bool boolean_value;
	int integer_value;
	double double_value;
	std::string string_value;
	std::vector<std::string> string_list_value;

	double double_step;
	double double_min;
	double double_max;
	int integer_step;
	int integer_min;
	int integer_max;
};

extern std::map<std::string, ConfigItem> config;

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

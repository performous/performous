#pragma once
#ifndef PERFORMOUS_CONFIGURATION_HH
#define PERFORMOUS_CONFIGURATION_HH

#include "audio.hh"
#include "util.hh"
#include <boost/any.hpp>
#include <map>
#include <string>

/// configuration option
class ConfigItem {
	friend std::ostream& operator <<(std::ostream &os,const ConfigItem &obj);
  public:
	ConfigItem();
	/// constructor
	ConfigItem( std::string _type, bool _is_default);
	/// increments config value
	ConfigItem& operator++();
	/// decrements config value
	ConfigItem& operator--();
	/// adds to config value
	ConfigItem& operator+=(const int& right);
	/// substracts from config value
	ConfigItem& operator-=(const int& right);
	/// adds to config value
	ConfigItem& operator+=(const float& right);
	/// substracts from config value
	ConfigItem& operator-=(const float& right);
	/// adds to config value
	ConfigItem& operator+=(const double& right);
	/// substracts from config value
	ConfigItem& operator-=(const double& right);
	/// sets short decsription for confi item
	void set_short_description( std::string _short_desc );
	/// sets long description for config item
	void set_long_description( std::string _long_desc );
	/// returns integer
	int get_i(void);
	/// returns bool
	bool get_b(void);
	/// returns float
	double get_f(void);
	/// returns string
	std::string get_s(void);
	/// returns string list
	std::vector<std::string> get_sl(void);
	/// returns integer
	int &i(void);
	/// returns bool
	bool &b(void);
	/// returns float
	double &f(void);
	/// returns string
	std::string &s(void);
	/// returns string list
	std::vector<std::string> &sl(void);

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

void readConfigfile( const std::string &_configfile );

/// integer class
/** clamps to min, max
 */
template<int min, int max> class Integer {
  public:
	/// constructor
	Integer(int value): m_value(clamp(value, min, max)) {}
	/// assignment
	Integer& operator=(int value) { m_value = clamp(value, min, max); return *this; }
	/// returns int value
	int val() const { return m_value; }
	/// returns int value
	int& val() { return m_value; }

  private:
	int m_value;
};

/// abstract configuration base class
class Configuration {
  public:
	/// constructor
	Configuration(std::string const& description): m_description(description) {};
	virtual ~Configuration() {};
	/// next possible config value
	virtual void setNext() = 0;
	/// previous possible config value
	virtual void setPrevious() = 0;
	/// get config description
	std::string const& getDescription() const { return m_description; };
	/// get current config value
	virtual std::string getValue() const = 0;

  private:
	std::string m_description;
};

/// configuration for volume
class ConfigurationAudioVolume: public Configuration {
  public:
	/// typedef function pointer for getter
	typedef unsigned int (Audio::*GetFunc)();
	/// typedef function pointer for setter
	typedef void (Audio::*SetFunc)(unsigned int);
	/// constructor
	ConfigurationAudioVolume(std::string const& title, Audio& audio, GetFunc get, SetFunc set);
	void setNext();
	void setPrevious();
	std::string getValue() const;

  private:
	Audio& m_audio;
	GetFunc m_get;
	SetFunc m_set;
};

#endif

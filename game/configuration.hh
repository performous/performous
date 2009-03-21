#pragma once
#ifndef PERFORMOUS_CONFIGURATION_HH
#define PERFORMOUS_CONFIGURATION_HH

#include "audio.hh"
#include "util.hh"
#include <boost/any.hpp>
#include <boost/format.hpp>
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
	/// sets short decsription for config item
	void set_short_description( std::string _short_desc );
	/// sets long description for config item
	void set_long_description( std::string _long_desc );
	/// gets short decsription for config item
	std::string get_short_description() const;
	/// gets long description for config item
	std::string get_long_description() const;
	/// tells if the item is default or not
	bool is_default(void) const;
	/// returns the type
	std::string get_type(void) const;
	/// returns integer
	int get_i(void) const;
	/// returns bool
	bool get_b(void) const;
	/// returns float
	double get_f(void) const;
	/// returns string
	std::string get_s(void) const;
	/// returns string list
	std::vector<std::string> get_sl(void) const;
	/// returns integer
	int &i(bool _is_default = false);
	/// returns bool
	bool &b(bool _is_default = false);
	/// returns float
	double &f(bool _is_default = false);
	/// returns string
	std::string &s(bool _is_default = false);
	/// returns string list
	std::vector<std::string> &sl(bool _is_default = false);

  private:
	std::string type;
	std::string short_desc;
	std::string long_desc;

	bool m_is_default;

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
void writeConfigfile( const std::string &_configfile );

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
	Configuration() {};
	virtual ~Configuration() {};
	/// next possible config value
	virtual void setNext() = 0;
	/// previous possible config value
	virtual void setPrevious() = 0;
	/// get config description
	virtual std::string const getDescription() const = 0;
	/// get current config value
	virtual std::string getValue() const = 0;
};

/// a single configuration item
class ConfigurationItem : public Configuration {
  public:
	/// constructor
	ConfigurationItem(std::string _key): Configuration(), m_key(_key) { };
	virtual ~ConfigurationItem() {};
	/// next possible config value
	void setNext() {
		++config[m_key];
	};
	/// previous possible config value
	void setPrevious() {
		--config[m_key];
	};
	/// get current config value
	std::string getValue() const {
		ConfigItem item = config[m_key];
		if( item.get_type() == std::string("int") ) {
			return (boost::format("%d") % item.get_i()).str();
		} else if( item.get_type() == std::string("float") ) {
			return (boost::format("%.2f") % item.get_f()).str();
		} else if( item.get_type() == std::string("double") ) {
			return (boost::format("%.2f") % item.get_f()).str();
		} else if( item.get_type() == std::string("bool") ) {
			if( item.get_b() ) {
				return std::string("True");
			} else {
				return std::string("False");
			}
		} else {
			return std::string("Type not managed");
		}
	};
	/// get config description
	std::string const getDescription() const { 
		return config[m_key].get_short_description();
	};
  private:
	std::string m_key;
};

#endif

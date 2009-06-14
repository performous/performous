#pragma once
#ifndef PERFORMOUS_CONFIGURATION_HH
#define PERFORMOUS_CONFIGURATION_HH

#include "util.hh"
#include <boost/any.hpp>
#include <boost/format.hpp>
#include <boost/variant.hpp>
#include <map>
#include <string>
#include <vector>

namespace xmlpp { struct Element; }  // Forward declaration for libxml++ stuff

/// configuration option
class ConfigItem {
  public:
	typedef std::vector<std::string> StringList;
	ConfigItem() {}
	void update(xmlpp::Element& elem, int mode); //< Load XML config file, elem = Entry, mode = 0 for schema, 1 for system config and 2 for user config
	ConfigItem& operator++() { return incdec(1); } ///< increments config value
	ConfigItem& operator--() { return incdec(-1); } ///< decrements config value
	bool is_default() const; ///< Is the current value the same as the default value
	std::string get_type() const { return m_type; } ///< get the field type
	int& i(); ///< Access integer item
	bool& b(); ///< Access boolean item
	double& f(); ///< Access floating-point item
	std::string& s(); ///< Access string item
	StringList& sl(); ///< Access stringlist item
	void reset() { m_value = m_defaultValue; }
	std::string const& get_short_description() { return m_shortDesc; }
	std::string const& get_long_description() { return m_longDesc; }
	
  private:
	void verifyType(std::string const& t) const; ///< throws std::logic_error if t != type
	ConfigItem& incdec(int dir); ///< Increment/decrement by dir steps (must be -1 or 1)
	std::string m_type;
	std::string m_shortDesc;
	std::string m_longDesc;

	typedef boost::variant<bool, int, double, std::string, StringList> Value;
	Value m_value; ///< The current value
	Value m_defaultValue; ///< The value from factory/system config
	boost::variant<int, double> m_step, m_min, m_max;
};

typedef std::map<std::string, ConfigItem> Config;
extern Config config; ///< A global variable that contains all config items

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
		if (item.get_type() == "int") return (boost::format("%d") % item.i()).str();
		else if (item.get_type() == "float") return (boost::format("%.2f") % item.f()).str();
		else if( item.get_type() == "bool") return item.b() ? "Enabled" : "Disabled";
		else return std::string("Type not managed");
	};
	/// get config description
	std::string const getDescription() const { 
		return config[m_key].get_short_description();
	};
  private:
	std::string m_key;
};

#endif

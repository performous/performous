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
	void reset() { m_value = m_defaultValue; } ///< Reset to factory default
	std::string getValue() const; ///< Get a human-readable representation of the current value
	std::string const& getShortDesc() const { return m_shortDesc; }
	std::string const& getLongDesc() const { return m_longDesc; }
	
  private:
	template <typename T> void updateNumeric(xmlpp::Element& elem, int mode); ///< Used internally for loading XML
	void verifyType(std::string const& t) const; ///< throws std::logic_error if t != type
	ConfigItem& incdec(int dir); ///< Increment/decrement by dir steps (must be -1 or 1)
	std::string m_type;
	std::string m_shortDesc;
	std::string m_longDesc;

	typedef boost::variant<bool, int, double, std::string, StringList> Value;
	Value m_value; ///< The current value
	Value m_defaultValue; ///< The value from factory/system config
	boost::variant<int, double> m_step, m_min, m_max;
	boost::variant<int, double> m_multiplier;
	std::string m_unit;
};

typedef std::map<std::string, ConfigItem> Config;
extern Config config; ///< A global variable that contains all config items

/** Read config schema and configuration from XML files **/
void readConfig();

/** Write modified config options to user's config XML **/
void writeConfig();

struct MenuEntry {
	std::string name;
	std::string shortDesc;
	std::string longDesc;
	std::vector<std::string> items;
};

typedef std::vector<MenuEntry> ConfigMenu;
extern ConfigMenu configMenu;

#endif

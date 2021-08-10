#pragma once

#include "libxml++.hh"

#include <variant>
#include <map>
#include <string>
#include <vector>
#include <list>

// configuration option
class ConfigItem {
  public:
	typedef std::vector<std::string> StringList; ///< a list of strings
	typedef std::vector<std::string> OptionList; ///< a list of string options
	ConfigItem() : m_sel() {}
	ConfigItem(bool bval);
	ConfigItem(int ival);
	ConfigItem(float fval);
	ConfigItem(std::string sval);
	ConfigItem(OptionList opts);
	void update(xmlpp::Element& elem, int mode); ///< Load XML config file, elem = Entry, mode = 0 for schema, 1 for system config and 2 for user config
	ConfigItem& operator++() { return incdec(1); } ///< increments config value
	ConfigItem& operator--() { return incdec(-1); } ///< decrements config value
	/// Is the current value the same as the default value (factory setting or system setting)
	bool isDefault(bool factory = false) const { return isDefaultImpl(factory ? m_factoryDefaultValue : m_defaultValue); }
	std::string get_type() const { return m_type; } ///< get the field type
	int& i(); ///< Access integer item
	int const& i() const; ///< Access integer item
	bool& b(); ///< Access boolean item
	double& f(); ///< Access floating-point item
	std::string& s(); ///< Access string item
	StringList& sl(); ///< Access stringlist item
	OptionList& ol(); ///< Access optionlist item
	std::string& so(); ///< Access currently selected string option
	void select(int i); ///< Set optionlist selected item index
	void reset(bool factory = false) { m_value = factory ? m_factoryDefaultValue : m_defaultValue; } ///< Reset to default
	void makeSystem() { m_defaultValue = m_value; } ///< Make current value the system default (used when saving system config)
	std::string const& getName() const { return m_keyName; } ///< get the name for this ConfigItem in the schema.
	std::string const getValue() const; ///< Get a human-readable representation of the current value
	std::string const getOldValue() const { return m_oldValue; } ///< Get a human-readable representation of a previous value.
	void setOldValue(std::string const& value) { m_oldValue = value; } ///< Store the current value before changing it, for later comparison.
	std::string const& getShortDesc() const { return m_shortDesc; } ///< get the short description for this ConfigItem
	std::string const& getLongDesc() const { return m_longDesc; } ///< get the long description for this ConfigItem
	void addEnum(std::string name); ///< Dynamically adds an enum to all values
	void selectEnum(std::string const& name); ///< Set integer value by enum name
	std::string const getEnumName() const; ///< Returns the selected enum option's text

  private:
	template <typename T> void updateNumeric(xmlpp::Element& elem, int mode); ///< Used internally for loading XML
	void verifyType(std::string const& t) const; ///< throws std::logic_error if t != type
	ConfigItem& incdec(int dir); ///< Increment/decrement by dir steps (must be -1 or 1)
	std::string m_keyName; ///< The config key in the schema file.
	std::string m_type;
	std::string m_shortDesc;
	std::string m_longDesc;

	typedef std::variant<bool, int, double, std::string, StringList> Value;
	bool isDefaultImpl(Value const& defaultValue) const;
	Value m_value; ///< The current value
	Value m_factoryDefaultValue; ///< The value from config schema
	Value m_defaultValue; ///< The value from config schema or system config
	std::string m_oldValue; ///< A previous value, as output by getValue().
	std::vector<std::string> m_enums; ///< Enum value titles
	std::variant<int, double> m_step, m_min, m_max;
	std::variant<int, double> m_multiplier;
	std::string m_unit;
	int m_sel;
};

typedef std::map<std::string, ConfigItem> Config;
extern Config config; ///< A global variable that contains all config items

/** Read config schema and configuration from XML files **/
void readConfig();
void populateBackends(const std::list<std::string>& backendList);

class Game;

/** Write modified config options to user's or system-wide config XML **/
void writeConfig(Game &game, bool system = false);

/// struct for entries in menu
struct MenuEntry {
	std::string name; ///< name of the menu entry
	std::string shortDesc; ///< a short description
	std::string longDesc; ///< a longer description
	std::vector<std::string> items; ///< selectable options
};

int PaHostApiNameToHostApiTypeId(const std::string& name);

typedef std::vector<MenuEntry> ConfigMenu;
extern ConfigMenu configMenu;

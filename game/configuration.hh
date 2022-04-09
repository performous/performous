#pragma once

#include "libxml++.hh"

#include <variant>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <list>

// configuration option
class ConfigItem {
  public:
	typedef std::vector<std::string> StringList; ///< a list of strings
	struct OptionList {
            int m_sel = 0;
            std::vector<std::string> options;
            bool operator==(const OptionList&ol) const { return m_sel == ol.m_sel && options == ol.options; }
        }; ///< a list of string options
        struct Enum {
           std::set<std::string> names;
           std::string value;
           bool operator==(const Enum &e) const { return e.value == value; }
        };
        template<class T>
        struct Numerical {
            T m_step, m_min, m_max;
            T m_multiplier;
            T value;
            std::string m_unit;
            bool operator==(const Numerical<T> &e) const { return e.value == value; }
        };
	typedef std::variant<Numerical<int>, Enum, bool, Numerical<double>, std::string, StringList, OptionList> Value;
	ConfigItem(Value val = Value{});
	void update(xmlpp::Element& elem, int mode); ///< Load XML config file, elem = Entry, mode = 0 for schema, 1 for system config and 2 for user config
	ConfigItem& operator++() { return incdec(1); } ///< increments config value
	ConfigItem& operator--() { return incdec(-1); } ///< decrements config value
	/// Is the current value the same as the default value (factory setting or system setting)
	bool isDefault(bool factory = false) const { return isDefaultImpl(factory ? m_factoryDefaultValue : m_defaultValue); }
	int& i(); ///< Access integer item
	int const& i() const; ///< Access integer item
	bool& b(); ///< Access boolean item
	const bool& b() const; ///< Access boolean item
	double& f(); ///< Access floating-point item
	std::string& s(); ///< Access string item
	StringList& sl(); ///< Access stringlist item
	OptionList& ol(); ///< Access optionlist item
	std::string& so(); ///< Access currently selected string option
	void select(int i); ///< Set optionlist selected item index
	void reset(bool factory = false) { m_value = factory ? m_factoryDefaultValue : m_defaultValue; } ///< Reset to default
	void makeSystem() { m_defaultValue = m_value; } ///< Make current value the system default (used when saving system config)
	std::string const& getName() const { return m_keyName; } ///< get the name for this ConfigItem in the schema.
	std::string toString() const; ///< Get a human-readable representation of the current value
	std::string const& getShortDesc() const { return m_shortDesc; } ///< get the short description for this ConfigItem
	std::string const& getLongDesc() const { return m_longDesc; } ///< get the long description for this ConfigItem
	void addEnum(std::string name); ///< Dynamically adds an enum to all values
	void selectEnum(std::string const& name); ///< Set integer value by enum name
	std::string const getEnumName() const; ///< Returns the selected enum option's text

        std::string get_type_name() const;
	
        template <class Visitor>
        constexpr auto visit(Visitor&& visitor) const { return std::visit(std::forward<Visitor>(visitor), m_value); }
  private:
        template <typename T> auto getChecker() -> T&;
        template <typename T> const auto &getChecker() const;
	template <typename T> void updateNumeric(xmlpp::Element& elem, int mode); ///< Used internally for loading XML
	ConfigItem& incdec(int dir); ///< Increment/decrement by dir steps (must be -1 or 1)
	std::string m_keyName; ///< The config key in the schema file.
	std::string m_shortDesc;
	std::string m_longDesc;

	bool isDefaultImpl(Value const& defaultValue) const;
	Value m_value; ///< The current value
	Value m_factoryDefaultValue; ///< The value from config schema
	Value m_defaultValue; ///< The value from config schema or system config
};

using Config = std::map<std::string, ConfigItem>;
extern Config config; ///< A global variable that contains all config items

/** Read config schema and configuration from XML files **/
void readConfig();
void populateBackends(const std::vector<std::string>& backendList);
void populateLanguages(const std::map<std::string, std::string>& languages);

/** Write modified config options to user's or system-wide config XML **/
void writeConfig(bool system = false);

/// struct for entries in menu
struct MenuEntry {
	std::string name; ///< name of the menu entry
	std::string shortDesc; ///< a short description
	std::string longDesc; ///< a longer description
	std::vector<std::string> items; ///< selectable options
};

int PaHostApiNameToHostApiTypeId(const std::string& name);

using ConfigMenu = std::vector<MenuEntry>;
extern ConfigMenu configMenu;

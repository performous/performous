#pragma once

#include <variant>
#include <map>
#include <string>
#include <vector>
#include <list>
#include <functional>


class ConfigItem {
  public:
	using StringList = std::vector<std::string>; ///< a list of strings
	using OptionList = std::vector<std::string>; ///< a list of string options
	using Value = std::variant<bool, unsigned short, int, float, std::string, StringList>;
	using NumericValue = std::variant<unsigned short, int, float>;

	ConfigItem() = default;
	explicit ConfigItem(bool bval);
	explicit ConfigItem(int ival);
	explicit ConfigItem(unsigned short uival);
	explicit ConfigItem(float fval);
	ConfigItem(std::string sval);
	ConfigItem(OptionList opts);

	ConfigItem& operator++() { return incdec(1); } ///< increments config value
	ConfigItem& operator--() { return incdec(-1); } ///< decrements config value
	/// Is the current value the same as the default value (factory setting or system setting)
	bool isDefault(bool factory = false) const { return isDefaultImpl(factory ? m_factoryDefaultValue : m_defaultValue); }
	std::string get_type() const { return m_type; } ///< get the field type
	std::string getType() const { return m_type; } ///< get the field type
	void setType(std::string const& type) { m_type = type; }
	int& i(); ///< Access integer item
	int const& i() const; ///< Access integer item
	unsigned short& ui(); ///< Access unsigned integer item
	unsigned short const& ui() const; ///< Access unsigned integer item
	bool& b(); ///< Access boolean item
	float& f(); ///< Access floating-point item
	std::string& s(); ///< Access string item
	StringList& sl(); ///< Access stringlist item
	OptionList& ol(); ///< Access optionlist item
	std::string& so(); ///< Access currently selected string option
	void select(unsigned short index); ///< Set optionlist selected item index
	void reset(bool factory = false) { m_value = factory ? m_factoryDefaultValue : m_defaultValue; } ///< Reset to default
	void makeSystem() { m_defaultValue = m_value; } ///< Make current value the system default (used when saving system config)
	std::string const& getName() const { return m_keyName; } ///< get the name for this ConfigItem in the schema.
	void setName(std::string const& name) { m_keyName = name; } ///< get the name for this ConfigItem in the schema.
	std::string const& getShortDesc() const { return m_shortDesc; } ///< get the short description for this ConfigItem
	void setDescription(std::string const& text) { m_shortDesc = text; }
	std::string const& getLongDesc() const { return m_longDesc; } ///< get the long description for this ConfigItem
	Value& value() { return m_value; }
	const Value& value() const { return m_value; }
	void setLongDescription(std::string const& text) { m_longDesc = text; }
	void setValue(Value const& value) { m_value = value; }
	void setDefaultValue(Value const& value) { m_defaultValue = value; }
	void setFactoryDefaultValue(Value const& value) { m_factoryDefaultValue = value; }
	std::string const getValue() const; ///< Get a human-readable representation of the current value
	std::string const getOldValue() const { return m_oldValue; } ///< Get a human-readable representation of a previous value.
	void setOldValue(std::string const& value) { m_oldValue = value; } ///< Store the current value before changing it, for later comparison.
	void addEnum(std::string const& name); ///< Dynamically adds an enum to all values
	void selectEnum(std::string const& name); ///< Set integer value by enum name
	std::string const getEnumName() const; ///< Returns the selected enum option's text
	std::vector<std::string>& getEnum() { return m_enums; }
	unsigned short getSelection() const { return m_sel; }

	NumericValue getMin() const;
	void setMin(NumericValue);
	NumericValue getMax() const;
	void setMax(NumericValue);
	NumericValue getStep() const;
	void setStep(NumericValue);
	NumericValue getMultiplier() const;
	void setMultiplier(NumericValue);
	std::string const& getUnit() const;
	void setUnit(std::string const&);

	void setGetValueFunction(std::function<std::string(ConfigItem const&)> f) { m_getValue = f; }

  private:
	void verifyType(std::string const& t) const; ///< throws std::logic_error if t != type
	ConfigItem& incdec(int dir); ///< Increment/decrement by dir steps (must be -1 or 1)
	bool isDefaultImpl(Value const& defaultValue) const;

  private:
	std::string m_keyName; ///< The config key in the schema file.
	std::string m_type;
	std::string m_shortDesc;
	std::string m_longDesc;

	Value m_value; ///< The current value
	Value m_factoryDefaultValue; ///< The value from config schema
	Value m_defaultValue; ///< The value from config schema or system config
	std::string m_oldValue; ///< A previous value, as output by getValue().
	std::vector<std::string> m_enums; ///< Enum value titles
	NumericValue m_step{1};
	NumericValue m_min, m_max;
	NumericValue m_multiplier;
	std::string m_unit;
	unsigned short m_sel = 0;
	std::function<std::string(ConfigItem const&)> m_getValue;
};

using ConfigItemMap = std::map<std::string, ConfigItem>;

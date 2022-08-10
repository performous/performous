#include "configitem.hh"

#include "i18n.hh"
#include "util.hh"

#include <fmt/format.h>
#include <algorithm>
#include <future>
#include <iomanip>
#include <stdexcept>
#include <iostream>
#include <cmath>

ConfigItemMap config;

ConfigItem::ConfigItem(bool bval)
: m_type("bool"), m_value(bval) {
}

ConfigItem::ConfigItem(int ival)
: m_type("int"), m_value(ival) {
}

ConfigItem::ConfigItem(unsigned short uival)
: m_type("uint"), m_value(uival), m_sel() {
}

ConfigItem::ConfigItem(float fval)
: m_type("float"), m_value(fval) {
}

ConfigItem::ConfigItem(std::string sval)
: m_type("string"), m_value(sval) {
}

ConfigItem::ConfigItem(OptionList opts)
: m_type("option_list"), m_value(opts) {
}

ConfigItem::NumericValue ConfigItem::getMin() const {
	return m_min;
}

void ConfigItem::setMin(NumericValue min) {
	m_min = min;
}

ConfigItem::NumericValue ConfigItem::getMax() const {
	return m_max;
}

void ConfigItem::setMax(NumericValue max) {
	m_max = max;
}

ConfigItem::NumericValue ConfigItem::getStep() const {
	return m_step;
}

void ConfigItem::setStep(NumericValue step) {
	m_step = step;
}

ConfigItem::NumericValue ConfigItem::getMultiplier() const {
	return m_multiplier;
}

void ConfigItem::setMultiplier(NumericValue multiplier) {
	m_multiplier = multiplier;
}

std::string const& ConfigItem::getUnit() const {
	return m_unit;
}

void ConfigItem::setUnit(std::string const& unit) {
	m_unit = unit;
}

ConfigItem& ConfigItem::incdec(int dir) {
	if (m_type == "int") {
		int& val = std::get<int>(m_value);
		int step = std::get<int>(m_step);
		val = clamp(((val + dir * step)/ step) * step, std::get<int>(m_min), std::get<int>(m_max));
	} else if (m_type == "uint") {
		unsigned short& val = std::get<unsigned short>(m_value);
		int value = static_cast<int>(val);
		int step = static_cast<int>(std::get<unsigned short>(m_step));
		int min = static_cast<int>(std::get<unsigned short>(m_min));
		int max = static_cast<int>(std::get<unsigned short>(m_max));
		val = static_cast<unsigned short>(clamp(((value + dir * step) / step) * step, min, max));
	} else if (m_type == "float") {
		auto& val = std::get<float>(m_value);
		auto step = std::get<float>(m_step);
		val = clamp(static_cast<float>(round((val + static_cast<float>(dir) * step) / step) * step), std::get<float>(m_min), std::get<float>(m_max));
	} else if (m_type == "bool") {
		bool& val = std::get<bool>(m_value);
		val = !val;
	} else if (m_type == "option_list") {
		auto s = static_cast<unsigned short>(std::get<OptionList>(m_value).size());
		m_sel = static_cast<unsigned short>(m_sel + dir + s) % s;
	}
	return *this;
}

bool ConfigItem::isDefaultImpl(ConfigItem::Value const& defaultValue) const {
	if (m_type == "bool") return std::get<bool>(m_value) == std::get<bool>(defaultValue);
	if (m_type == "int") return std::get<int>(m_value) == std::get<int>(defaultValue);
	if (m_type == "uint") return ui() == std::get<unsigned short>(defaultValue);
	if (m_type == "float") return std::get<float>(m_value) == std::get<float>(defaultValue);
	if (m_type == "string") return std::get<std::string>(m_value) == std::get<std::string>(defaultValue);
	if (m_type == "string_list") return std::get<StringList>(m_value) == std::get<StringList>(defaultValue);
	if (m_type == "option_list") return std::get<OptionList>(m_value) == std::get<OptionList>(defaultValue);
	throw std::logic_error("ConfigItem::is_default doesn't know type '" + m_type + "'");
}

void ConfigItem::verifyType(std::string const& type) const {
	if (type == m_type)
		return;
	const auto name = getName();
	if (m_type.empty())
		throw std::logic_error("Config item " + name + ", requested_type=" + type + " used in C++ but missing from config schema");

	throw std::logic_error("Config item type mismatch: item=" + name + ", type=" + m_type + ", requested=" + type);
}

int& ConfigItem::i() {
	verifyType("int");
	return std::get<int>(m_value);
}
int const& ConfigItem::i() const {
	verifyType("int");
	return std::get<int>(m_value);
}
unsigned short& ConfigItem::ui() {
	verifyType("uint");
	return std::get<unsigned short>(m_value);
}
unsigned short const& ConfigItem::ui() const {
	verifyType("uint");
	return std::get<unsigned short>(m_value);
}
bool& ConfigItem::b() {
	verifyType("bool");
	return std::get<bool>(m_value);
}
float& ConfigItem::f() {
	verifyType("float");
	return std::get<float>(m_value);
}
std::string& ConfigItem::s() {
	verifyType("string");
	return std::get<std::string>(m_value);
}
ConfigItem::StringList& ConfigItem::sl() {
	verifyType("string_list");
	return std::get<StringList>(m_value);
}
ConfigItem::OptionList& ConfigItem::ol() {
	verifyType("option_list");
	return std::get<OptionList>(m_value);
}
std::string& ConfigItem::so() {
	verifyType("option_list");
	return std::get<OptionList>(m_value).at(m_sel);
}

void ConfigItem::select(unsigned short index) {
	verifyType("option_list");
	m_sel = clamp<unsigned short>(index, 0, static_cast<unsigned short>(std::get<OptionList>(m_value).size()-1));
}

namespace {
	template <typename T, typename VariantAll, typename VariantNum>
	std::string numericFormat(VariantAll const& value, VariantNum const& multiplier, VariantNum const& step) {
		// Find suitable precision (not very useful for integers, but this code is generic...)
		T m = std::get<T>(multiplier);
		T s = static_cast<T>(std::abs(m * std::get<T>(step)));
		unsigned precision = 0;
		while (s > static_cast<T>(0) && (static_cast<T>(s *= static_cast<T>(10))) < static_cast<T>(10))
			++precision;
		// Format the output
		return fmt::format("{:.{}f}", double(m) * std::get<T>(value), precision);
	}
}

std::string const ConfigItem::getValue() const {
	if(m_getValue)
		return m_getValue(*this);

	if (m_type == "int") {
		return numericFormat<int>(m_value, m_multiplier, m_step) + _(m_unit);
	}
	if (m_type == "uint") {
		unsigned short val = ui();
		if (val < m_enums.size())
			return m_enums[val];
		return numericFormat<unsigned short>(m_value, m_multiplier, m_step) + _(m_unit);
	}
	if (m_type == "float")
		return numericFormat<float>(m_value, m_multiplier, m_step) + _(m_unit);
	if (m_type == "bool")
		return std::get<bool>(m_value) ? _("Enabled") : _("Disabled");
	if (m_type == "string")
		return std::get<std::string>(m_value);
	if (m_type == "string_list") {
		StringList const& sl = std::get<StringList>(m_value);
		return sl.size() == 1 ? "{" + sl[0] + "}" : fmt::format(_("{:d} items"), sl.size());
	}
	if (m_type == "option_list")
		return std::get<OptionList>(m_value).at(m_sel);

	throw std::logic_error("ConfigItem::getValue doesn't know type '" + m_type + "'");
}

void ConfigItem::addEnum(std::string const& name) {
	verifyType("uint");
	if (find(m_enums.begin(), m_enums.end(), name) == m_enums.end())
		m_enums.push_back(name);

	m_min = static_cast<unsigned short>(0);
	m_max = static_cast<unsigned short>(m_enums.size() - 1);
	m_step = static_cast<unsigned short>(1);
}

void ConfigItem::selectEnum(std::string const& name) {
	auto it = std::find(m_enums.begin(), m_enums.end(), name);
	if (it == m_enums.end())
		throw std::runtime_error("Enum value " + name + " not found in " + m_shortDesc);
	ui() = static_cast<unsigned short>(it - m_enums.begin());
}


std::string const ConfigItem::getEnumName() const {
	unsigned short const& val = ui();
	if (val < m_enums.size())
		return m_enums[val];

	return {};
}


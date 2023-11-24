#pragma once

#include "value.hh"
#include "json.hh"

#include <map>

struct JsonToValueConverter {
	JsonToValueConverter(std::map<std::string, Value> const& values);

	Value convert(nlohmann::json const& config);

private:
	std::map<std::string, Value> const& m_values;
};

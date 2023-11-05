#pragma once

#include "value.hh"
#include "json.hh"

struct JsonToValueConverter {
	Value convert(nlohmann::json const& config);
};

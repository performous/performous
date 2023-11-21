#include "json_to_value_converter.hh"

#include <iostream>

JsonToValueConverter::JsonToValueConverter(std::map<std::string, Value> const& values)
	: m_values(values) {
}

Value JsonToValueConverter::convert(nlohmann::json const& valueConfig) {
	if (valueConfig.is_string()) {
		auto const id = valueConfig.get<std::string>();
		auto const it = m_values.find(id);

		if (it != m_values.end())
			return it->second;

		std::clog << "theme/error: no value with id '" << id << "' was defined!" << std::endl;

		return 0.f;
	}
	if (!valueConfig.is_object()) {
		return value::Float(valueConfig.get<float>());
	}
	if (valueConfig.contains("value")) {
		return value::Float(valueConfig.at("value").get<float>());
	}
	if (valueConfig.contains("negate")) {
		return value::Negate(convert(valueConfig.at("negate").at("value")));
	}
	if (valueConfig.contains("min")) {
		auto values = std::vector<Value>();

		for (auto const& subValueConfig : valueConfig.at("min")) {
			values.emplace_back(convert(subValueConfig));
		}

		return value::Min(values);
	}
	if (valueConfig.contains("max")) {
		auto values = std::vector<Value>();

		for (auto const& subValueConfig : valueConfig.at("max")) {
			values.emplace_back(convert(subValueConfig));
		}

		return value::Max(values);
	}
	if (valueConfig.contains("time")) {
		return value::Time();
	}
	if (valueConfig.contains("multiply")) {
		return value::Multiply(convert(valueConfig.at("multiply").at("value0")), convert(valueConfig.at("multiply").at("value1")));
	}
	if (valueConfig.contains("add")) {
		return value::Add(convert(valueConfig.at("add").at("value0")), convert(valueConfig.at("add").at("value1")));
	}
	if (valueConfig.contains("subtract")) {
		return value::Subtract(convert(valueConfig.at("subtract").at("value0")), convert(valueConfig.at("subtract").at("value1")));
	}
	if (valueConfig.contains("random")) {
		auto min = Value(0.0f);
		auto max = Value(1.0f);

		if (valueConfig.at("random").contains("min"))
			min = convert(valueConfig.at("random").at("min"));
		if (valueConfig.at("random").contains("max"))
			max = convert(valueConfig.at("random").at("max"));

		return value::Random(min, max);
	}
	if (valueConfig.contains("sinus")) {
		return value::Sinus(convert(valueConfig.at("sinus").at("degree")));
	}
	if (valueConfig.contains("squareroot")) {
		return value::Sinus(convert(valueConfig.at("squareroot").at("value")));
	}

	throw std::runtime_error("value configuration contains no supported type!");
}

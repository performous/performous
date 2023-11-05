#include "json_to_value_converter.hh"

namespace {

	Value parseValue(nlohmann::json const& valueConfig) {
		if (valueConfig.contains("value")) {
			return values::Float(valueConfig.at("value").get<float>());
		}
		if (valueConfig.contains("min")) {
			auto values = std::vector<Value>();

			for (auto const& subValueConfig : valueConfig.at("min")) {
				values.emplace_back(parseValue(subValueConfig));
			}

			return values::Min(values);
		}
		if (valueConfig.contains("max")) {
			auto values = std::vector<Value>();

			for (auto const& subValueConfig : valueConfig.at("max")) {
				values.emplace_back(parseValue(subValueConfig));
			}

			return values::Max(values);
		}
		if (valueConfig.contains("time")) {
			return values::Time();
		}
		if (valueConfig.contains("multiply")) {
			return values::Multiply(parseValue(valueConfig.at("multiply").at("value0")), parseValue(valueConfig.at("multiply").at("value1")));
		}
		if (valueConfig.contains("add")) {
			return values::Multiply(parseValue(valueConfig.at("add").at("value0")), parseValue(valueConfig.at("add").at("value1")));
		}
		if (valueConfig.contains("random")) {
			auto min = Value(0.0f);
			auto max = Value(1.0f);

			if (valueConfig.at("random").contains("min"))
				min = parseValue(valueConfig.at("random").at("min"));
			if (valueConfig.at("random").contains("max"))
				max = parseValue(valueConfig.at("random").at("max"));

			return values::Random(min, max);
		}
		if (valueConfig.contains("sinus")) {
			return values::Sinus(parseValue(valueConfig.at("sinus").at("degree")));
		}

		throw std::runtime_error("value configuration contains no supported type!");
	}

}

Value JsonToValueConverter::convert(nlohmann::json const& config) {
	return parseValue(config);
}

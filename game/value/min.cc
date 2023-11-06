#include "min.hh"

#include "value.hh"

MinValue::MinValue(std::vector<Value> const& values)
	: m_values(values) {
}

float MinValue::get() const {
	if (m_values.empty())
		return 0.0f;

	auto value = m_values.at(0).get();

	for (auto i = 1U; i < m_values.size(); ++i)
		value = std::min(value, m_values[i].get());

	return value;
}

MinValue::operator float() const {
	return get();
}

ValuePtr MinValue::clone() const {
	auto clonedValues = std::vector<Value>{};

	for (auto const& value : m_values)
		clonedValues.emplace_back(value.clone());

	return value::Min(clonedValues);
}

namespace value {
	ValuePtr Min(std::vector<Value> const& values) {
		return std::make_shared<MinValue>(values);
	}
}

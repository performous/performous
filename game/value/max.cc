#include "max.hh"

#include "value.hh"

MaxValue::MaxValue(std::vector<Value> const& values)
	: m_values(values) {
}

float MaxValue::get() const {
	if (m_values.empty())
		return 0.0f;

	auto value = m_values.at(0).get();

	for (auto i = 1U; i < m_values.size(); ++i)
		value = std::max(value, m_values[i].get());

	return value;
}

MaxValue::operator float() const {
	return get();
}

namespace values {
	ValuePtr Max(std::vector<Value> const& values) {
		return std::make_shared<MaxValue>(values);
	}
}

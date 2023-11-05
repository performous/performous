#include "sinus.hh"

#include "value.hh"

#include <cmath>

SinusValue::SinusValue(Value const& value)
	: m_value(value) {
}

float SinusValue::get() const {
	auto const value = m_value * M_PI / 180.0;

	return static_cast<float>(::sin(value));
}

SinusValue::operator float() const {
	return get();
}

namespace values {
	ValuePtr Sinus(Value const& value) {
		return std::make_shared<SinusValue>(value);
	}
}

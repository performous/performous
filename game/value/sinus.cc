#include "sinus.hh"

#include "value.hh"
#include "utils/math.hh"

#include <cmath>

SinusValue::SinusValue(Value const& value)
	: m_value(value) {
}

float SinusValue::get() const {
	auto const value = m_value * pi() / 180.0;

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

#include "squareroot.hh"

#include "value.hh"

#include <cmath>

SquareRootValue::SquareRootValue(Value const& value)
	: m_value(value) {
}

float SquareRootValue::get() const {
	auto const value = ::sqrt(m_value);

	return static_cast<float>(value);
}

SquareRootValue::operator float() const {
	return get();
}

ValuePtr SquareRootValue::clone() const {
	return value::SquareRoot(m_value.clone());
}

namespace value {
	ValuePtr SquareRoot(Value const& value) {
		return std::make_shared<SquareRootValue>(value);
	}
}

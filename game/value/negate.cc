#include "negate.hh"

NegateValue::NegateValue(Value const& value)
	: m_value(value) {
}

float NegateValue::get() const {
	return -m_value.get();
}

NegateValue::operator float() const {
	return get();
}

ValuePtr NegateValue::clone() const {
	return value::Negate(m_value.clone());
}

namespace value {
	ValuePtr Negate(Value const& value) {
		return std::make_shared<NegateValue>(value);
	}
}

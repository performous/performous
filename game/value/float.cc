#include "float.hh"

#include "value.hh"

FloatValue::FloatValue(float value)
	: m_value(value) {
}

float FloatValue::get() const {
	return m_value;
}

FloatValue::operator float() const {
	return m_value;
}

namespace values {
	ValuePtr Float(float value) {
		return std::make_shared<FloatValue>(value);
	}
}

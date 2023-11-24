#include "value.hh"

#include <cmath>
#include <stdexcept>

Value::Value(float value)
	: m_value(value::Float(value)) {
}

Value::Value(ValuePtr ptr)
	: m_value(ptr) {
}

Value::Value(Value const& origin) {
	m_value = origin.m_value->clone();
}

Value& Value::operator=(Value const& origin) {
	m_value = origin.m_value->clone();

	return *this;
}

float Value::get() const {
	return m_value->get();
}

Value::operator float() const {
	return m_value->get();
}

ValuePtr Value::clone() const {
	return m_value->clone();
}

ValuePtr Value::ptr() const {
	return m_value;
}

#include "add.hh"

#include "value.hh"

AddValue::AddValue(Value const& valueA, Value const& valueB)
	: m_valueA(valueA), m_valueB(valueB) {
}

float AddValue::get() const {
	return m_valueA.get() + m_valueB.get();
}

AddValue::operator float() const {
	return get();
}

ValuePtr AddValue::clone() const {
	return value::Add(m_valueA.clone(), m_valueB.clone());
}

namespace value {
	ValuePtr Add(Value const& valueA, Value const& valueB) {
		return std::make_shared<AddValue>(valueA, valueB);
	}
}

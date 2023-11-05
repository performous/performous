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

namespace values {
	ValuePtr Add(Value const& valueA, Value const& valueB) {
		return std::make_shared<AddValue>(valueA, valueB);
	}
}

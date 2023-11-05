#include "multiply.hh"

#include "value.hh"

MultiplyValue::MultiplyValue(Value const& valueA, Value const& valueB)
	: m_valueA(valueA), m_valueB(valueB) {
}

float MultiplyValue::get() const {
	return m_valueA.get() * m_valueB.get();
}

MultiplyValue::operator float() const {
	return get();
}

namespace values {
	ValuePtr Multiply(Value const& valueA, Value const& valueB) {
		return std::make_shared<MultiplyValue>(valueA, valueB);
	}
}

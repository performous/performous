#include "subtract.hh"

SubtractValue::SubtractValue(Value const& valueA, Value const& valueB)
	: m_valueA(valueA), m_valueB(valueB) {
}

float SubtractValue::get() const {
	return m_valueA.get() - m_valueB.get();
}

SubtractValue::operator float() const {
	return get();
}

ValuePtr SubtractValue::clone() const {
	return value::Subtract(m_valueA.clone(), m_valueB.clone());
}

namespace value {
	ValuePtr Subtract(Value const& valueA, Value const& valueB) {
		return std::make_shared<SubtractValue>(valueA, valueB);
	}
}

#include "mix.hh"

#include "utils/math.hh"

MixValue::MixValue(Value const& valueA, Value const& valueB, Value const& a)
	: m_valueA(valueA), m_valueB(valueB), m_a(a) {
}

float MixValue::get() const {
	return mix(m_valueA.get(), m_valueB.get(), m_a.get());
}

MixValue::operator float() const {
	return get();
}

ValuePtr MixValue::clone() const {
	return value::Mix(m_valueA.clone(), m_valueB.clone(), m_a.clone());
}

namespace value {
	ValuePtr Mix(Value const& valueA, Value const& valueB, Value const& a) {
		return std::make_shared<MixValue>(valueA, valueB, a);
	}
}

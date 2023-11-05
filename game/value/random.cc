#include "random.hh"

#include "value.hh"

#include <cstdlib>

RandomValue::RandomValue(Value const& min, Value const& max)
	: m_min(min), m_max(max) {
}

float RandomValue::get() const {
	return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * (m_max.get() - m_min.get()) + m_min.get();
}

RandomValue::operator float() const {
	return get();
}

namespace values {
	ValuePtr Random(Value const& min, Value const& max) {
		return std::make_shared<RandomValue>(min, max);
	}

	ValuePtr Random() {
		return Random(0.0f, 0.1f);
	}
}

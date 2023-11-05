#include "time.hh"

#include "value.hh"

TimeValue::TimeValue(TimePoint const& timepoint)
	: m_start(timepoint) {
}

float TimeValue::get() const {
	auto const value = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - m_start).count();

	return float(value) * 0.000001f;
}

TimeValue::operator float() const {
	return get();
}

namespace values {
	ValuePtr Time() {
		return std::make_shared<TimeValue>();
	}
}

#pragma once

#include "ivalue.hh"

#include <chrono>

class TimeValue : public IValue {
public:
	using TimePoint = decltype(std::chrono::steady_clock::now());

	TimeValue(TimePoint const& timepoint = std::chrono::steady_clock::now());

	float get() const override;
	operator float() const override;

	ValuePtr clone() const override;

private:
	TimePoint m_start;
};

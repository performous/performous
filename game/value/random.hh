#pragma once

#include "value.hh"

class RandomValue : public IValue {
public:
	RandomValue(Value const& min, Value const& max);

	float get() const override;
	operator float() const override;

private:
	Value m_min;
	Value m_max;
};

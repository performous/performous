#pragma once

#include "value.hh"

class SinusValue : public IValue {
public:
	SinusValue(Value const& value);

	float get() const override;
	operator float() const override;

	ValuePtr clone() const override;

private:
	Value m_value;
};

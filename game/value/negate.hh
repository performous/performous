#pragma once

#include "value.hh"

class NegateValue : public IValue {
public:
	NegateValue(Value const& value);

	float get() const override;
	operator float() const override;

	ValuePtr clone() const override;

private:
	Value m_value;
};

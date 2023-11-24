#pragma once

#include "value.hh"

class SquareRootValue : public IValue {
public:
	SquareRootValue(Value const& value);

	float get() const override;
	operator float() const override;

	ValuePtr clone() const override;

private:
	Value m_value;
};

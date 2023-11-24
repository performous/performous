#pragma once

//x * (1 - a) + y * a

#include "value.hh"

class MixValue : public IValue {
public:
	MixValue(Value const& valueA, Value const& valueB, Value const& a);

	float get() const override;
	operator float() const override;

	ValuePtr clone() const override;

private:
	Value m_valueA;
	Value m_valueB;
	Value m_a;
};

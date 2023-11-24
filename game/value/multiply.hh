#pragma once

#include "value.hh"

class MultiplyValue : public IValue {
public:
	MultiplyValue(Value const& valueA, Value const& valueB);

	float get() const override;
	operator float() const override;

	ValuePtr clone() const override;

private:
	Value m_valueA;
	Value m_valueB;
};

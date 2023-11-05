#pragma once

#include "value.hh"

class AddValue : public IValue {
public:
	AddValue(Value const& valueA, Value const& valueB);

	float get() const override;
	operator float() const override;

private:
	Value m_valueA;
	Value m_valueB;
};

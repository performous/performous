#pragma once

#include "ivalue.hh"

class FloatValue : public IValue {
public:
	FloatValue(float value = 0.f);

	float get() const override;
	operator float() const override;

private:
	float m_value;
};

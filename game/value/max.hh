#pragma once

#include "value.hh"

#include <vector>

class MaxValue : public IValue {
public:
	MaxValue(std::vector<Value> const& values);

	float get() const override;
	operator float() const override;

private:
	std::vector<Value> m_values;
};

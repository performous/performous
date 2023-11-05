#pragma once

#include "value.hh"

#include <vector>

class MinValue : public IValue {
public:
	MinValue(std::vector<Value> const& values);

	float get() const override;
	operator float() const override;

private:
	std::vector<Value> m_values;
};

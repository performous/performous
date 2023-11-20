#pragma once

#include "value.hh"
#include "constant_value_provider.hh"

#include <string>

class ConstantValue : public IValue {
public:
	ConstantValue(std::string const&, ConstantValueProviderPtr);

	float get() const override;
	operator float() const override;

	ValuePtr clone() const override;

private:
	std::string m_constant;
	ConstantValueProviderPtr m_provider;
};

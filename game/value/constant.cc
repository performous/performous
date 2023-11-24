#include "constant.hh"

#include <iostream>

ConstantValue::ConstantValue(std::string const& name, ConstantValueProviderPtr provider)
	: m_constant(name), m_provider(provider) {
}

float ConstantValue::get() const {
	return m_provider->getValue(m_constant);
}

ConstantValue::operator float() const {
	return get();
}

ValuePtr ConstantValue::clone() const {
	return value::Constant(m_constant, m_provider);
}

namespace value {
	ValuePtr Constant(std::string const& name, ConstantValueProviderPtr provider) {
		return std::make_shared<ConstantValue>(name, provider);
	}
}


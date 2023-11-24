#include "constant_value_provider.hh"

void ConstantValueProvider::setValue(std::string const& name, float value) {
	m_values[name] = value;
}

float ConstantValueProvider::getValue(std::string const& name) const {
	auto const it = m_values.find(name);

	if (it != m_values.end())
		return it->second;

	return 0.f;
}



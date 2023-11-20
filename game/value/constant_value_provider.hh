#pragma once

#include <map>
#include <memory>
#include <string>

class ConstantValueProvider {
public:
	void setValue(std::string const&, float);
	float getValue(std::string const&) const;

private:
	std::map<std::string, float> m_values;
};

using ConstantValueProviderPtr = std::shared_ptr<ConstantValueProvider>;

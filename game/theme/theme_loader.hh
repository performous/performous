#pragma once

#include "theme.hh"

#include <memory>
#include <string>

using ThemePtr = std::shared_ptr<Theme>;

class ThemeLoader {
public:
	template<class ThemeType>
	std::shared_ptr<ThemeType> load(std::string const& screenName) {
		return std::static_pointer_cast<ThemeType>(load(screenName));
	}

private:
	ThemePtr load(std::string const& screenName);
};

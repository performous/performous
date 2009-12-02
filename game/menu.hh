#pragma once

#include "opengl_text.hh"
#include "surface.hh"
#include <boost/noncopyable.hpp>
#include <string>

/// struct for menu options
struct MenuOption {
	MenuOption();
	MenuOption(const std::string scrn, const std::string img);
	/// screen to activate when option is pressed
	std::string screen;
	/// image to show when option is selected
	Surface image;
};

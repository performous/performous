#pragma once

#include "opengl_text.hh"
#include "surface.hh"
#include <boost/noncopyable.hpp>
#include <string>

/// struct for menu options
struct MenuOption {
	MenuOption();
	MenuOption(const std::string& nm, const std::string& scrn, const std::string& img, const std::string& comm);
	/// option name (it will be displayed as this)
	std::string name;
	/// screen to activate when option is pressed
	std::string screen;
	/// image to show when option is selected
	Surface image;
	/// extended information about the option selected
	std::string comment;
};

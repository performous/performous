#include "menu.hh"

#include "fs.hh"
#include "configuration.hh"

MenuOption::MenuOption()
{}
MenuOption::MenuOption(const std::string nm, const std::string scrn, const std::string img, const std::string comm):
	name(nm),
	screen(scrn),
	image(getThemePath(img)),
	comment(comm)
{}

#include "menu.hh"

#include "fs.hh"
#include "configuration.hh"

MenuOption::MenuOption()
{}
MenuOption::MenuOption(const std::string scrn, const std::string img) : screen(scrn), image(getThemePath(img))
{}

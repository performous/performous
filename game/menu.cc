#include "menu.hh"
#include "screen.hh"
#include "surface.hh"
#include "fs.hh"
#include "configuration.hh"
#include "instrumentgraph.hh"

MenuOption::MenuOption(const std::string& nm, const std::string& comm):
	type(CLOSE_SUBMENU),
	name(nm),
	comment(comm),
	value(NULL),
	newValue()
{}

MenuOption::MenuOption(const std::string& nm, const std::string& comm, ConfigItem* val):
	type(CHANGE_VALUE),
	name(nm),
	comment(comm),
	value(val),
	newValue()
{}

MenuOption::MenuOption(const std::string& nm, const std::string& comm, ConfigItem* val, ConfigItem newval):
	type(SET_AND_CLOSE),
	name(nm),
	comment(comm),
	value(val),
	newValue(newval)
{}

MenuOption::MenuOption(const std::string& nm, const std::string& comm, MenuOptions opts):
	type(OPEN_SUBMENU),
	name(nm),
	comment(comm),
	value(NULL),
	newValue(),
	options(opts)
{}


MenuOption::MenuOption(const std::string& nm, const std::string& comm, const std::string& scrn, const std::string& img):
	type(ACTIVATE_SCREEN),
	name(nm),
	comment(comm),
	value(NULL),
	newValue(scrn)
{
	if (!img.empty()) image.reset(new Surface(getThemePath(img)));
}



void Menu::add(MenuOption opt) {
	root_options.push_back(opt);
	options = root_options; // Set current menu to root
	current = options.begin(); // Reset iterator
}

void Menu::move(int dir) {
	if (dir > 0 && current != (--options.end())) ++current;
	else if (dir < 0 && current != options.begin()) --current;
}

void Menu::action(int dir) {
	switch (current->type) {
		case MenuOption::OPEN_SUBMENU:
			options = current->options;
			current = options.begin();
			m_level++;
			break;
		case MenuOption::CHANGE_VALUE:
			if (current->value) {
				if (dir > 0) ++(*(current->value));
				else if (dir < 0) --(*(current->value));
			}
			break;
		case MenuOption::SET_AND_CLOSE:
			if (current->value) *(current->value) = current->newValue;
			// Fall-through to closing
		case MenuOption::CLOSE_SUBMENU:
			// TODO: Handle more than one level of submenus
			if (m_level == 0) close();
			else m_level--;
			options = root_options;
			current = options.begin();
			break;
		case MenuOption::ACTIVATE_SCREEN:
			ScreenManager* sm = ScreenManager::getSingletonPtr();
			std::string screen = current->newValue.s();
			if (screen.empty()) sm->finished();
			else sm->activateScreen(screen);
			break;
	}
}

void Menu::clear() {
	options.clear();
	root_options.clear();
	current = options.end();
}

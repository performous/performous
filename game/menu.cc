#include "menu.hh"
#include "screen.hh"
#include "surface.hh"
#include "fs.hh"
#include "configuration.hh"
#include "instrumentgraph.hh"

MenuOption::MenuOption(const std::string& nm, const std::string& comm):
	type(CLOSE_SUBMENU),
	value(NULL),
	newValue(),
	name(nm),
	comment(comm),
	namePtr(NULL),
	commentPtr(NULL)
{}

MenuOption::MenuOption(const std::string& nm, const std::string& comm, ConfigItem* val):
	type(CHANGE_VALUE),
	value(val),
	newValue(),
	name(nm),
	comment(comm),
	namePtr(NULL),
	commentPtr(NULL)
{}

MenuOption::MenuOption(const std::string& nm, const std::string& comm, ConfigItem* val, ConfigItem newval):
	type(SET_AND_CLOSE),
	value(val),
	newValue(newval),
	name(nm),
	comment(comm),
	namePtr(NULL),
	commentPtr(NULL)
{}

MenuOption::MenuOption(const std::string& nm, const std::string& comm, MenuOptions opts):
	type(OPEN_SUBMENU),
	value(NULL),
	newValue(),
	options(opts),
	name(nm),
	comment(comm),
	namePtr(NULL),
	commentPtr(NULL)
{}


MenuOption::MenuOption(const std::string& nm, const std::string& comm, const std::string& scrn, const std::string& img):
	type(ACTIVATE_SCREEN),
	value(NULL),
	newValue(scrn),
	name(nm),
	comment(comm),
	namePtr(NULL),
	commentPtr(NULL)
{
	if (!img.empty()) image.reset(new Surface(getThemePath(img)));
}


Menu::Menu():
	m_open(true),
	m_level(0)
{
	menu_stack.push_back(&root_options);
	current_it = menu_stack.back()->end();
}

void Menu::add(MenuOption opt) {
	root_options.push_back(opt);
	menu_stack.clear();
	menu_stack.push_back(&root_options);
	current_it = menu_stack.back()->begin(); // Reset iterator
}

void Menu::move(int dir) {
	if (dir > 0 && current_it != (--menu_stack.back()->end())) ++current_it;
	else if (dir < 0 && current_it != menu_stack.back()->begin()) --current_it;
}

void Menu::action(int dir) {
	switch (current_it->type) {
		case MenuOption::OPEN_SUBMENU:
			if (current_it->options.empty()) break;
			menu_stack.push_back(&current_it->options);
			current_it = menu_stack.back()->begin();
			m_level++;
			break;
		case MenuOption::CHANGE_VALUE:
			if (current_it->value) {
				if (dir > 0) ++(*(current_it->value));
				else if (dir < 0) --(*(current_it->value));
			}
			break;
		case MenuOption::SET_AND_CLOSE:
			if (current_it->value) *(current_it->value) = current_it->newValue;
			// Fall-through to closing
		case MenuOption::CLOSE_SUBMENU:
			if (menu_stack.size() > 1) menu_stack.pop_back();
			else close();
			current_it = menu_stack.back()->begin();
			break;
		case MenuOption::ACTIVATE_SCREEN:
			ScreenManager* sm = ScreenManager::getSingletonPtr();
			std::string screen = current_it->newValue.s();
			clear();
			if (screen.empty()) sm->finished();
			else sm->activateScreen(screen);
			break;
	}
}

void Menu::clear() {
	menu_stack.clear();
	root_options.clear();
	menu_stack.push_back(&root_options);
	current_it = menu_stack.back()->end();
}

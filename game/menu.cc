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
	dimensions(),
	m_open(true)
{
	clear();
}

void Menu::add(MenuOption opt) {
	root_options.push_back(opt);
	clear(true); // Adding resets menu stack
}

void Menu::move(int dir) {
	if (dir > 0 && selection_stack.back() < menu_stack.back()->size() - 1) ++selection_stack.back();
	else if (dir < 0 && selection_stack.back() > 0) --selection_stack.back();
}

void Menu::select(unsigned sel) {
	if (sel < menu_stack.back()->size())
		selection_stack.back() = sel;
}

void Menu::action(int dir) {
	switch (current().type) {
		case MenuOption::OPEN_SUBMENU:
			if (current().options.empty()) break;
			menu_stack.push_back(&current().options);
			selection_stack.push_back(0);
			break;
		case MenuOption::CHANGE_VALUE:
			if (current().value) {
				if (dir > 0) ++(*(current().value));
				else if (dir < 0) --(*(current().value));
			}
			break;
		case MenuOption::SET_AND_CLOSE:
			if (current().value) *(current().value) = current().newValue;
			// Fall-through to closing
		case MenuOption::CLOSE_SUBMENU:
			if (menu_stack.size() > 1) {
				menu_stack.pop_back();
				selection_stack.pop_back();
			} else close();
			break;
		case MenuOption::ACTIVATE_SCREEN:
			ScreenManager* sm = ScreenManager::getSingletonPtr();
			std::string screen = current().newValue.s();
			clear();
			if (screen.empty()) sm->finished();
			else sm->activateScreen(screen);
			break;
	}
}

void Menu::clear(bool save_root) {
	if (!save_root) root_options.clear();
	menu_stack.clear();
	selection_stack.clear();
	menu_stack.push_back(&root_options);
	selection_stack.push_back(0);
}

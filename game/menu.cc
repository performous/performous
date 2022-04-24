#include "menu.hh"
#include "screen.hh"
#include "texture.hh"
#include "fs.hh"
#include "game.hh"

MenuOption::MenuOption(std::string const& nm, std::string const& comm, MenuImage img):
  type(), value(), newValue(), callback(), image(img), name(nm), comment(comm), namePtr(), commentPtr()
{}

std::string MenuOption::getName() const {
	if (namePtr) return *namePtr;
	if (!name.empty()) return name;
	if (value) return value->getValue();
	return "";
}

std::string MenuOption::getVirtName() const {
	return virtualName.empty() ? std::string() : virtualName;
}

const std::string& MenuOption::getComment() const { return commentPtr ? *commentPtr : comment; }

bool MenuOption::isActive() const {
	if (type == Type::OPEN_SUBMENU && options.empty()) return false;
	if (type == Type::CHANGE_VALUE) {
		if (!value) return false;
		if (value->get_type() == "option_list" && value->ol().size() <= 1) return false;
	}
	return true;
}


Menu::Menu(): dimensions(), m_open(true) { clear(); }

void Menu::add(MenuOption opt) {
	root_options.push_back(opt);
	clear(true); // Adding resets menu stack
}

void Menu::move(int dir) {
	if (dir > 0 && selection_stack.back() < menu_stack.back()->size() - 1) ++selection_stack.back();
	else if (dir < 0 && selection_stack.back() > 0) --selection_stack.back();
}

void Menu::select(size_t sel) {
	if (sel < menu_stack.back()->size()) selection_stack.back() = sel;
}

void Menu::action(int dir) {
	switch (current().type) {
		case MenuOption::Type::OPEN_SUBMENU: {
			if (current().options.empty()) break;
			menu_stack.push_back(&current().options);
			selection_stack.push_back(0);
			break;
		}
		case MenuOption::Type::CHANGE_VALUE: {
			if (current().value) {
				if (current().value->getName() == "audio/backend") {
					current().value->setOldValue(current().value->getValue());
				}
				else if (current().value->getName() == "graphic/stereo3d") {
					std::string oldValue((current().value->getValue() == _("Disabled")) ? "0" : "1");
					current().value->setOldValue(oldValue);
				}
				if (dir > 0) ++(*(current().value));
				else if (dir < 0) --(*(current().value));

				if (current().value->getName() == "game/language") {
					auto &value = config["game/language"];
					Game::getSingletonPtr()->setLanguage(value.getValue());
				}
			}
			break;
		}
		case MenuOption::Type::SET_AND_CLOSE:
			if (current().value) *(current().value) = current().newValue;
			[[fallthrough]];  // Continuing to CLOSE_SUBMENU is intentional
		case MenuOption::Type::CLOSE_SUBMENU: {
			closeSubmenu();
			break;
		}
		case MenuOption::Type::ACTIVATE_SCREEN: {
			Game* gm = Game::getSingletonPtr();
			std::string screen = current().newValue.s();
			clear();
			if (screen.empty()) gm->finished();
			else gm->activateScreen(screen);
			break;
		}
		case MenuOption::Type::CALLBACK_FUNCTION: {
			if (current().callback) current().callback();
			break;
		}
	}
}

void Menu::clear(bool save_root) {
	if (!save_root) root_options.clear();
	menu_stack.clear();
	selection_stack.clear();
	menu_stack.push_back(&root_options);
	selection_stack.push_back(0);
}

void Menu::closeSubmenu() {
	if (menu_stack.size() > 1) {
		menu_stack.pop_back();
		selection_stack.pop_back();
	} else close();
}

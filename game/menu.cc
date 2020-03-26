#include "menu.hh"
#include "screen.hh"
#include "texture.hh"
#include "fs.hh"


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
	if (type == OPEN_SUBMENU && options.empty()) return false;
	if (type == CHANGE_VALUE) {
		if (!value) return false;
		if (value->get_type() == "option_list" && value->ol().size() <= 1) return false;
	}
	return true;
}


Menu::Menu(): dimensions(), m_open(true) { clear(); }

void Menu::sortOptions() {
    std::sort(std::begin(root_options), std::end(root_options),
    [] (const auto &a, const auto &b) {
            const auto &an = a.getName();
            const auto &bn = b.getName();
            /* If two entries have the same name, take the address of object as
             * sort criterion as they can only be equal IIF they are the same */
            if (an == bn)
                return &a < &b;
            return an < bn;
    });
}

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
		case MenuOption::OPEN_SUBMENU: {
			if (current().options.empty()) break;
			menu_stack.push_back(&current().options);
			selection_stack.push_back(0);
			break;
		}
		case MenuOption::CHANGE_VALUE: {
			if (current().value) {
				if (current().value->getShortDesc() == config["audio/backend"].getShortDesc()) {
					current().value->oldValue = current().value->getValue();
				}
				else if (current().value->getShortDesc() == config["graphic/stereo3d"].getShortDesc()) {
					current().value->oldValue = (current().value->getValue() == _("Disabled")) ? "0" : "1";
				}
				if (dir > 0) ++(*(current().value));
				else if (dir < 0) --(*(current().value));
			}
			break;
		}
		case MenuOption::SET_AND_CLOSE:
			if (current().value) *(current().value) = current().newValue;
			[[fallthrough]];  // Continuing to CLOSE_SUBMENU is intentional
		case MenuOption::CLOSE_SUBMENU: {
			closeSubmenu();
			break;
		}
		case MenuOption::ACTIVATE_SCREEN: {
			Game* gm = Game::getSingletonPtr();
			std::string screen = current().newValue.s();
			clear();
			if (screen.empty()) gm->finished();
			else gm->activateScreen(screen);
			break;
		}
		case MenuOption::CALLBACK_FUNCTION: {
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

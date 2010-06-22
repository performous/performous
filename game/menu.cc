#include "menu.hh"
#include "fs.hh"
#include "configuration.hh"
#include "instrumentgraph.hh"

MenuOption::MenuOption(const std::string nm, const std::string comm):
	name(nm),
	comment(comm)
{}


MainMenuOption::MainMenuOption(const std::string nm, const std::string scrn, const std::string img, const std::string comm):
	MenuOption(nm, comm),
	screen(scrn),
	image(getThemePath(img))
{}


InstrumentMenuOption::InstrumentMenuOption(const std::string nm, const std::string comm, InstrumentMenuAdjustFunc fn1, InstrumentMenuValueFunc fn2):
	MenuOption(nm, comm),
	adjust(fn1),
	getValue(fn2),
	value()
{}


void InstrumentMenu::add(InstrumentMenuOption opt) {
	// Cache the caption
	if (opt.getValue) opt.value = (owner.*opt.getValue)();
	else opt.value = opt.name;
	options.push_back(opt);
	// Reset iterator
	current = options.begin();
}

void InstrumentMenu::move(int dir) {
	if (dir > 0 && current != (--options.end())) ++current;
	else if (dir < 0 && current != options.begin()) --current;
}

void InstrumentMenu::changeValue(int dir) {
	if (!current->adjust) return;
	(owner.*current->adjust)(dir); // adjust
	if (current->getValue) current->value = (owner.*current->getValue)(); // cache value
}

void InstrumentMenu::refreshValues() {
	for (InstrumentMenuOptions::iterator it = options.begin(); it != options.end(); ++it) {
		if (it->getValue) it->value = (owner.*it->getValue)();
		else it->value = it->name;
	}
}

#include "menu.hh"
#include "fs.hh"
#include "configuration.hh"
#include "instrumentgraph.hh"

MenuOption::MenuOption(const std::string nm):
	name(nm)
{}


MainMenuOption::MainMenuOption(const std::string nm, const std::string scrn, const std::string img, const std::string comm):
	MenuOption(nm),
	screen(scrn),
	image(getThemePath(img)),
	comment(comm)
{}


InstrumentMenuOption::InstrumentMenuOption(const std::string nm, InstrumentMenuAdjustFunc fn1, InstrumentMenuValueFunc fn2):
	MenuOption(nm),
	adjust(fn1),
	getValue(fn2)
{}


void InstrumentMenu::add(InstrumentMenuOption opt) {
	options.push_back(opt);
	// Cache new value
	options.back().value = (owner.*(options.back().getValue))();
	// Reset iterator
	current = options.begin();
}

void InstrumentMenu::move(int dir) {
	if (dir > 0 && current != (--options.end())) ++current;
	else if (dir < 0 && current != options.begin()) --current;
}

void InstrumentMenu::changeValue(int dir) {
	(owner.*current->adjust)(dir); // adjust
	current->value = (owner.*current->getValue)(); // cache value
}

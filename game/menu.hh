#pragma once

#include "opengl_text.hh"
#include "surface.hh"
#include <boost/noncopyable.hpp>
#include <string>
#include <vector>


/// struct for menu options
struct MenuOption {
	MenuOption();
	MenuOption(const std::string nm, const std::string comm);
	/// option name (it will be displayed as this)
	std::string name;
	/// extended information about the option selected
	std::string comment;
};


/// struct for main menu options
struct MainMenuOption: public MenuOption {
	MainMenuOption() {};
	MainMenuOption(const std::string nm, const std::string scrn, const std::string img, const std::string comm);
	/// screen to activate when option is pressed
	std::string screen;
	/// image to show when option is selected
	Surface image;
};


class InstrumentGraph;
typedef void (InstrumentGraph::*InstrumentMenuAdjustFunc)(int dir);
typedef std::string (InstrumentGraph::*InstrumentMenuValueFunc)() const;

/// struct for instrument joining menu option
struct InstrumentMenuOption: public MenuOption {
	InstrumentMenuOption(const std::string nm, const std::string comm,
	  InstrumentMenuAdjustFunc fn1, InstrumentMenuValueFunc fn2 = NULL);
	/// function to call to adjust the value
	InstrumentMenuAdjustFunc adjust;
	/// function to call to get the value
	InstrumentMenuValueFunc getValue;
	/// cached value
	std::string value;
};

typedef std::vector<InstrumentMenuOption> InstrumentMenuOptions;

/// Menu for selecting difficulty etc.
struct InstrumentMenu {
	/// constructor
	InstrumentMenu(InstrumentGraph& ig): owner(ig), current(options.end()) {}
	/// add a menu option
	void add(InstrumentMenuOption opt);
	/// move the selection
	void move(int dir = 1);
	/// adjust the selected value
	void changeValue(int dir = 1);
	/// refresh the cached values
	void refreshValues();
	/// clear items
	void clear() { options.clear(); }

	InstrumentGraph& owner;
	InstrumentMenuOptions options;
	InstrumentMenuOptions::iterator current;
};

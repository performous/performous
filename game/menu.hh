#pragma once

#include "opengl_text.hh"
#include "surface.hh"
#include "configuration.hh"
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <string>
#include <vector>

class MenuOption;
typedef std::vector<MenuOption> MenuOptions;

/// struct for menu options
struct MenuOption {
	enum Type { CLOSE_SUBMENU, OPEN_SUBMENU, CHANGE_VALUE, SET_AND_CLOSE} type;

	/// Construct a submenu closer
	MenuOption(const std::string& nm, const std::string& comm);
	/// Construct a value changer
	MenuOption(const std::string& nm, const std::string& comm, ConfigItem* val);
	/// Construct a value setter
	MenuOption(const std::string& nm, const std::string& comm, ConfigItem* val, ConfigItem newval);
	/// Construct a submenu opener
	MenuOption(const std::string& nm, const std::string& comm, MenuOptions opts);
	/// option name (it will be displayed as this)
	std::string name;
	/// extended information about the option selected
	std::string comment;
	/// value
	ConfigItem* value;
	/// value-to-be-set
	ConfigItem newValue;
	/// submenu
	MenuOptions options;
};


/// struct for main menu options
struct MainMenuOption {
	MainMenuOption() {};
	MainMenuOption(const std::string nm, const std::string scrn, const std::string img, const std::string comm);
	/// option name (it will be displayed as this)
	std::string name;
	/// extended information about the option selected
	std::string comment;
	/// screen to activate when option is pressed
	std::string screen;
	/// image to show when option is selected
	Surface image;
};


/// Menu for selecting difficulty etc.
struct Menu {
	/// constructor
	Menu(): current(options.end()) { }
	/// add a menu option
	void add(MenuOption opt);
	/// move the selection
	void move(int dir = 1);
	/// adjust the selected value
	void action(int dir = 1);
	/// clear items
	void clear() { options.clear(); }

	MenuOptions options;
	MenuOptions root_options;
	MenuOptions::iterator current;
};

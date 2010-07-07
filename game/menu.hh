#pragma once

#include "opengl_text.hh"
#include "configuration.hh"
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>

class Surface;
class MenuOption;

typedef std::vector<MenuOption> MenuOptions;

/// struct for menu options
struct MenuOption {
	enum Type { CLOSE_SUBMENU, OPEN_SUBMENU, CHANGE_VALUE, SET_AND_CLOSE, ACTIVATE_SCREEN } type;

	/// Construct a submenu closer
	MenuOption(const std::string& nm, const std::string& comm);
	/// Construct a value changer
	MenuOption(const std::string& nm, const std::string& comm, ConfigItem* val);
	/// Construct a value setter
	MenuOption(const std::string& nm, const std::string& comm, ConfigItem* val, ConfigItem newval);
	/// Construct a submenu opener
	MenuOption(const std::string& nm, const std::string& comm, MenuOptions opts);
	/// Construct a screen changer
	MenuOption(const std::string& nm, const std::string& comm, const std::string& scrn, const std::string& img = "");
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
	/// image to use with option
	boost::shared_ptr<Surface> image;
};


/// Menu for selecting difficulty etc.
struct Menu {
	/// constructor
	Menu(): current_it(options.end()), m_open(true), m_level(0) { }
	/// add a menu option
	void add(MenuOption opt);
	/// move the selection
	void move(int dir = 1);
	/// adjust the selected value
	void action(int dir = 1);
	/// clear items
	void clear();

	bool empty() const { return options.empty(); }
	bool isOpen() const { return m_open; }
	void open() { m_open = true; }
	void close() { m_open = false; }
	void toggle() { m_open = !m_open; }
	void moveToLast() { current_it = --(options.end()); }

	MenuOptions::iterator& currentRef() { return current_it; }
	const MenuOptions::const_iterator current() const { return current_it; }
	const MenuOptions::const_iterator begin() const { return options.begin(); }
	const MenuOptions::const_iterator end() const { return options.end(); }
	const MenuOptions getOptions() const { return options; }

  private:
	MenuOptions::iterator current_it;
	MenuOptions options;
	MenuOptions root_options;

	bool m_open;
	int m_level;

};

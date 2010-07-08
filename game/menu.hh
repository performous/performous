#pragma once

#include "opengl_text.hh"
#include "configuration.hh"
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>

class Surface;
struct MenuOption;

typedef std::vector<MenuOption> MenuOptions;
typedef std::vector<MenuOptions*> SubmenuStack;

/// Struct for menu options
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
	/// Sets name to follow a reference
	void setDynamicName(std::string& nm) { namePtr = &nm; }
	/// Sets comment to follow a reference
	void setDynamicComment(std::string& comm) { commentPtr = &comm; }
	/// Return name
	const std::string& getName() const { if (namePtr) return *namePtr; else return name; }
	/// Return comment
	const std::string& getComment() const { if (commentPtr) return *commentPtr; else return comment; }
	/// Value
	ConfigItem* value;
	/// Value-to-be-set
	ConfigItem newValue;
	/// Submenu
	MenuOptions options;
	/// Image to use with option
	boost::shared_ptr<Surface> image;
  private:
	std::string name;        /// Option name (it will be displayed as this)
	std::string comment;     /// Extended information about the option displayed usually when selected
	std::string* namePtr;    /// Optional pointer to dynamically changing name
	std::string* commentPtr; /// Optional pointer to dynamically changing comment
};


/// Menu for selecting difficulty etc.
struct Menu {
	/// constructor
	Menu();
	/// add a menu option
	void add(MenuOption opt);
	/// move the selection
	void move(int dir = 1);
	/// adjust the selected value
	void action(int dir = 1);
	/// clear items
	void clear();

	bool empty() const { return (menu_stack.empty() || (menu_stack.size() == 1 && menu_stack.back()->empty())); }
	bool isOpen() const { return m_open; }
	void open() { m_open = true; }
	void close() { m_open = false; }
	void toggle() { m_open = !m_open; }
	void moveToLast() { current_it = --(menu_stack.back()->end()); }

	MenuOption& back() { return root_options.back(); }
	MenuOptions::iterator& currentRef() { return current_it; }
	const MenuOptions::const_iterator current() const { return current_it; }
	const MenuOptions::const_iterator begin() const { return menu_stack.back()->begin(); }
	const MenuOptions::const_iterator end() const { return menu_stack.back()->end(); }
	const MenuOptions getOptions() const { return *menu_stack.back(); }

  private:
	MenuOptions::iterator current_it;
	MenuOptions root_options;
	SubmenuStack menu_stack;

	bool m_open;
	int m_level;

};

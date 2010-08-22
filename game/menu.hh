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
typedef std::vector<MenuOptions*> SubmenuStack;

/// Struct for menu options
class MenuOption {
  public:
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
	/// Check if this option can be selected
	bool isActive() const;
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
class Menu {
  public:
	/// constructor
	Menu();
	/// add a menu option
	void add(MenuOption opt);
	/// move the selection
	void move(int dir = 1);
	/// set selection
	void select(unsigned sel);
	/// adjust the selected value
	void action(int dir = 1);
	/// clear items
	void clear(bool save_root = false);

	bool empty() const { return (menu_stack.empty() || (menu_stack.size() == 1 && menu_stack.back()->empty())); }
	bool isOpen() const { return m_open; }
	void open() { m_open = true; }
	void close() { m_open = false; }
	void toggle() { m_open = !m_open; }
	void moveToLast() { selection_stack.back() = menu_stack.back()->size() - 1; }

	MenuOption& current() { return menu_stack.back()->at(selection_stack.back()); }
	MenuOption& back() { return root_options.back(); }
	const MenuOptions::const_iterator begin() const { return menu_stack.back()->begin(); }
	const MenuOptions::const_iterator end() const { return menu_stack.back()->end(); }
	const MenuOptions getOptions() const { return *menu_stack.back(); }

	Dimensions dimensions;

  private:
	MenuOptions root_options;
	SubmenuStack menu_stack;
	std::vector<size_t> selection_stack;

	bool m_open;
};

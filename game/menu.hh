#pragma once

#include "opengl_text.hh"
#include "configuration.hh"
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class Texture;
class MenuOption;
class Menu;

typedef std::vector<MenuOption> MenuOptions;
typedef std::vector<MenuOptions*> SubmenuStack;
typedef std::function<void ()> MenuOptionCallback;
typedef std::shared_ptr<Texture> MenuImage;

/// Struct for menu options
class MenuOption {
public:
	enum class Type { CLOSE_SUBMENU, OPEN_SUBMENU, CHANGE_VALUE, SET_AND_CLOSE, ACTIVATE_SCREEN, CALLBACK_FUNCTION } type;

	/// Construct a menu option. Default function is to close the menu.
	/// @param nm Name (menu item title)
	/// @param comm Comment
	/// @param img Image filename
	MenuOption(const std::string& nm, const std::string& comm, MenuImage img = MenuImage());

	MenuOption(const MenuOption&) = delete;
	MenuOption &operator=(const MenuOption&) = delete;

	MenuOption(MenuOption&&) = default;
	MenuOption &operator=(MenuOption&&) = default;

	/// Make the option change values of a ConfigItem.
	void changer(ConfigItem& val, std::string virtOptName = {}) {
		type = Type::CHANGE_VALUE;
		value = &val;
		if (!virtOptName.empty()) { virtualName = virtOptName; }
	}
	/// Make the option set a given value for ConfigItem and close the menu.
	void setter(ConfigItem& val, ConfigItem newval) { type = Type::SET_AND_CLOSE; value = &val; newValue = newval; }
	/// Make the option open a submenu
	void submenu(MenuOptions opts) { type = Type::OPEN_SUBMENU; options = std::move(opts); }
	/// Make the option activate a screeen
	void screen(std::string const& scrn) { type = Type::ACTIVATE_SCREEN; newValue = scrn; }
	/// Make the option call a callback
	void call(MenuOptionCallback f) { type = Type::CALLBACK_FUNCTION; callback = f; }
	/// Sets name to follow a reference
	void setDynamicName(std::string& nm) { namePtr = &nm; }
	/// Sets comment to follow a reference
	void setDynamicComment(std::string& comm) { commentPtr = &comm; }
	/// Return name
	std::string getName() const;
	/// Return virtual name (for options living only inside the screen)
	std::string getVirtName() const;
	/// Return comment
	const std::string& getComment() const;
	/// Check if this option can be selected
	bool isActive() const;
	ConfigItem* value;  ///< Setting to be adjusted
	ConfigItem newValue;  ///< Value to be set or screen name
	MenuOptions options;  ///< Submenu
	MenuOptionCallback callback;  ///< Callback function
	MenuImage image;  ///< Image to use with option
private:
	std::string virtualName; ///< Non-localized name for referring to options that exist only on-screen.
	std::string name;        ///< Option name (it will be displayed as this)
	std::string comment;     ///< Extended information about the option displayed usually when selected
	std::string* namePtr;    ///< Optional pointer to dynamically changing name
	std::string* commentPtr; ///< Optional pointer to dynamically changing comment
};


/// Menu for selecting difficulty etc.
class Menu {
public:
	/// constructor
	Menu();
	/// add a menu option, returns the one added
	MenuOption &add(MenuOption opt);
	/// move the selection
	void move(int dir = 1);
	/// set selection
	void select(unsigned sel);
	/// adjust the selected value
	void action(int dir = 1);
	/// clear items
	void clear(bool save_root = false);
	/// closes submenu or if in root menu, closes the whole menu
	void closeSubmenu();

	bool empty() const { return (menu_stack.empty() || (menu_stack.size() == 1 && menu_stack.back()->empty())); }
	bool isOpen() const { return m_open; }
	unsigned getSubmenuLevel() const { return static_cast<unsigned>(menu_stack.size() - 1); }
	void open() { m_open = true; }
	void close() { m_open = false; }
	void toggle() { m_open = !m_open; }
	void moveToLast() { selection_stack.back() = static_cast<unsigned>(menu_stack.back()->size() - 1); }

	unsigned curIndex() { return selection_stack.back(); }
	MenuOption& current() { return menu_stack.back()->at(static_cast<size_t>(selection_stack.back())); }
	MenuOption& back() { return root_options.back(); }
	const MenuOptions::const_iterator begin() const { return menu_stack.back()->begin(); }
	const MenuOptions::const_iterator end() const { return menu_stack.back()->end(); }
	const MenuOptions &getOptions() const { return *menu_stack.back(); }

	Dimensions dimensions;

private:
	MenuOptions root_options;
	SubmenuStack menu_stack;
	std::vector<unsigned> selection_stack;

	bool m_open;
};

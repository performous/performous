#pragma once

#include "configitem.hh"

#include "libxml++.hh"

#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <list>



using Config = ConfigItemMap;
using Config = std::map<std::string, ConfigItem>;
extern Config config; ///< A global variable that contains all config items

/** Read config schema and configuration from XML files **/
void readConfig();
void populateBackends(const std::list<std::string>& backendList);
void populateLanguages(const std::map<std::string, std::string>& languages);

/** Write modified config options to user's or system-wide config XML **/
void writeConfig(bool system = false);

/// struct for entries in menu
struct MenuEntry {
	std::string name; ///< name of the menu entry
	std::string shortDesc; ///< a short description
	std::string longDesc; ///< a longer description
	std::vector<std::string> items; ///< selectable options
};

unsigned short LanguageToLanguageId(const std::string& name);
using ConfigMenu = std::vector<MenuEntry>;
extern ConfigMenu configMenu;

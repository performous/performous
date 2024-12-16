#include "audio.hh"
#include "configuration.hh"

#include "fs.hh"
#include "util.hh"
#include "i18n.hh"
#include "libxml++.hh"
#include "screen_intro.hh"
#include "util.hh"
#include "game.hh"
#include <fmt/format.h>

#include <algorithm>
#include <future>
#include <iomanip>
#include <stdexcept>
#include <iostream>
#include <cmath>
#include <map>
#include <string>


class ConfigItemXMLLoader {
public:
	void update(ConfigItem&, xmlpp::Element& elem, unsigned mode); ///< Load XML config file, elem = Entry, mode = 0 for schema, 1 for system config and 2 for user config

private:
	template <typename T> void updateNumeric(ConfigItem& item, xmlpp::Element& elem, unsigned mode); ///< Used internally for loading XML
	void verifyType(std::string const& t) const; ///< throws std::logic_error if t != type
};

namespace {
	std::string getText(xmlpp::Element const& elem) {
		auto n = xmlpp::get_first_child_text(elem);  // Returns NULL if there is no text
		return n ? std::string(n->get_content()) : std::string();
	}

	std::string getText(xmlpp::Element const& elem, std::string const& path) {
		auto ns = elem.find(path);
		if (ns.empty()) return std::string();
		return getText(dynamic_cast<xmlpp::Element const&>(*ns[0]));
	}

	template <typename T, typename V> void setLimits(xmlpp::Element& e, V& min, V& max, V& step) {
		xmlpp::Attribute* a = e.get_attribute("min");
		if (a) min = sconv<T>(a->get_value());
		a = e.get_attribute("max");
		if (a) max = sconv<T>(a->get_value());
		a = e.get_attribute("step");
		if (a) step = sconv<T>(a->get_value());
	}
	template <typename T> void setLimits(xmlpp::Element& e, ConfigItem& item) {
		setLimits<T>(e, item.m_min, item.m_max, item.m_step);
	}
}

template <typename T> void ConfigItemXMLLoader::updateNumeric(ConfigItem& item, xmlpp::Element& elem, unsigned mode) {
	auto ns = elem.find("limits");
	if (!ns.empty())
		setLimits<T>(dynamic_cast<xmlpp::Element&>(*ns[0]), item);
	else if (mode == 0)
		throw xmlpp::XMLError(elem, "child element limits missing");
	ns = elem.find("ui");
	// Default values
	if (mode == 0) {
		item.m_unit = {};
		item.m_multiplier = static_cast<T>(1);
	}
	if (!ns.empty()) {
		xmlpp::Element& e = dynamic_cast<xmlpp::Element&>(*ns[0]);
		try {
			item.m_unit = xmlpp::getAttribute(e, "unit");
		}
		catch (...) {
		}
		std::string m;
		try {
			m = xmlpp::getAttribute(e, "multiplier");
			item.m_multiplier = sconv<T>(m);
		}
		catch (xmlpp::XMLError&) {
		}
		catch (std::exception&) {
			throw xmlpp::XMLError(e, "attribute multiplier='" + m + "' value invalid");
		}
	}
}

void ConfigItemXMLLoader::update(ConfigItem& item, xmlpp::Element& elem, unsigned mode) {
	try {
		if (mode == 0) {
			item.setName(xmlpp::getAttribute(elem, "name"));
			item.setType(xmlpp::getAttribute(elem, "type"));
			if (item.getType().empty())
				throw std::runtime_error("Entry type attribute is missing");
			// Menu text
			item.setDescription(getText(elem, "short"));
			item.setLongDescription(getText(elem, "long"));
		}
		else {
			std::string type{xmlpp::getAttribute(elem, "type")};
			if (item.getType() == "uint" && type == "int") {
				type = "uint"; // Convert old config values.
				std::clog << "configuration/info: Converting config value: " + getAttribute(elem, "name") + ", to type uint." << std::endl;
			}
			if (!type.empty() && type != item.getType())
				throw std::runtime_error("Entry type mismatch: " + getAttribute(elem, "name") + ": schema type = " + item.getType() + ", config type = " + type);
		}
		if (item.getType() == "bool") {
			const auto value_string = xmlpp::getAttribute(elem, "value");
			if (value_string == "true")
				item.setValue(true);
			else if (value_string == "false")
				item.setValue(false);
			else
				throw std::runtime_error("Invalid boolean value '" + value_string + "'");
		}
		else if (item.getType() == "int") {
			const auto value_string = xmlpp::getAttribute(elem, "value");
			if (!value_string.empty())
				item.setValue(std::stoi(value_string));
			updateNumeric<int>(item, elem, mode);
		} else if (item.getType() == "uint") {
			std::string value_string = xmlpp::getAttribute(elem, "value");
			if (!value_string.empty())
				item.setValue(static_cast<unsigned short>(std::stoi(value_string)));
			// Enum handling
			if (mode == 0) {
				auto n2 = elem.find("limits/enum");
				if (!n2.empty()) {
					for (auto it2 = n2.begin(), end2 = n2.end(); it2 != end2; ++it2) {
						xmlpp::Element& elem2 = dynamic_cast<xmlpp::Element&>(**it2);
						if (!getText(elem2).empty()) item.getEnum().push_back(getText(elem2));
					}
					item.m_min = static_cast<unsigned short>(0);
					item.m_max = static_cast<unsigned short>(item.getEnum().size() - 1);
					item.m_step = static_cast<unsigned short>(1);
				}
			}
			updateNumeric<unsigned short>(item, elem, mode);
		}
		else if (item.getType() == "float") {
			std::string value_string = xmlpp::getAttribute(elem, "value");
			if (!value_string.empty()) item.setValue(std::stof(value_string));
			updateNumeric<float>(item, elem, mode);
		}
		else if (item.getType() == "string") {
			item.setValue(getText(elem, "stringvalue"));
		}
		else if (item.getType() == "string_list" || item.getType() == "option_list") {
			//TODO: Option list should also update selection (from attribute?)
			std::vector<std::string> value;
			auto n2 = elem.find("stringvalue");
			for (auto it2 = n2.begin(), end2 = n2.end(); it2 != end2; ++it2) {
				value.push_back(getText(dynamic_cast<xmlpp::Element const&>(**it2)));
			}
			item.setValue(value);
		}
		else if (!item.getType().empty())
			throw std::runtime_error("Invalid value type in config schema: " + item.getType());
		// Schema sets all defaults, system config sets the system default
		if (mode < 1) item.setFactoryDefaultValue(item.value());
		if (mode < 2) item.setDefaultValue(item.value());
	}
	catch (std::exception& e) {
		const auto line = elem.get_line();
		throw std::runtime_error(std::to_string(line) + ": Error while reading entry: " + e.what());
	}
}

// These are set in readConfig, once the paths have been bootstrapped.
fs::path systemConfFile;
fs::path userConfFile;

namespace {
	std::string getValue_language(ConfigItem const& item) {
		unsigned short autoLanguageType = 1337;
		unsigned short val = LanguageToLanguageId(item.getEnumName()); // In the case of the language, val is the real value while m_value is the enum case for its cosmetic name.
		std::string languageName = (val != autoLanguageType) ? item.getEnumName() : "Auto";
		return languageName;
	}
	std::string getValue_audio_backend(ConfigItem const& item) {
		if (item.getName() != "audio/backend")
			throw std::logic_error("getValue_audio_backend: item must be 'audio/backend' but is '" + item.getName() + "'");

		int AutoBackendType = 1337;
		static int val = std::get<unsigned short>(item.value());
		if (val != std::get<unsigned short>(item.value()))
			val = PaHostApiNameToHostApiTypeId(item.getEnumName()); // In the case of the audio backend, val is the real value while m_value is the enum case for its cosmetic name.
		int hostApi = Pa_HostApiTypeIdToHostApiIndex(PaHostApiTypeId(val));
		std::ostringstream oss;
		oss << "audio/info: Trying the selected Portaudio backend...";
		if (val != AutoBackendType) {
			oss << " found at index: " << hostApi;
		}
		else {
			oss << " not found; but this is normal when Auto is selected."; // Auto is not a real PaHostApiTypeId, so it will always return paHostApiNotFound
		}
		oss << std::endl;
		std::clog << oss.str();
		if ((hostApi != paHostApiNotFound) || (val == AutoBackendType)) {
			std::string backendName = (val != AutoBackendType) ? Pa_GetHostApiInfo(hostApi)->name : "Auto";
			std::clog << "audio/info: Currently selected audio backend is: " << backendName << std::endl;
			return backendName;
		}
		else std::clog << "audio/warning: Currently selected audio backend is unavailable on this system, will default to Auto." << std::endl;
		return "Auto";
	}
}

void writeConfig(Game& game, bool system) {
	xmlpp::Document doc;
	auto nodeRoot = doc.create_root_node("performous");
	auto dirty = false;
	for (auto& elem : config) {
		ConfigItem& item = elem.second;
		auto const name = elem.first;
		auto const type = item.getType();

		if (item.isDefault(system) && name != "audio/backend" && name != "graphic/stereo3d") {
			continue; // No need to save settings with default values
		}

		dirty = true;
		xmlpp::Element* entryNode = xmlpp::add_child_element(nodeRoot, "entry");

		entryNode->set_attribute("type", type);
		entryNode->set_attribute("name", name);
		if (name == "audio/backend") {
			std::string currentBackEnd = Audio::backendConfig().getOldValue();
			int oldValue = PaHostApiNameToHostApiTypeId(currentBackEnd);
			int newValue = PaHostApiNameToHostApiTypeId(item.getEnumName());
			if (currentBackEnd != item.getEnumName() && !currentBackEnd.empty()) {
				entryNode->set_attribute("value", std::to_string(newValue));
				std::clog << "audio/info: Audio backend changed; will now restart audio subsystem." << std::endl;
				Audio::backendConfig().selectEnum(item.getEnumName());

				auto& audio = game.getAudio();

				audio.restart();
				audio.playMusic(game, findFile("menu.ogg"), true); // Start music again
			}
			else {
				entryNode->set_attribute("value", std::to_string(oldValue));
			}
		}
		else if (name == "game/language") {
			auto currentLanguageStr = TranslationEngine::getCurrentLanguage().second;
			auto newLanguagestr = item.getEnumName();
			auto currentLanguageId = LanguageToLanguageId(currentLanguageStr);
			auto newLanguageId = LanguageToLanguageId(newLanguagestr);
			if ((newLanguagestr == "Auto" || currentLanguageId != newLanguageId) && !config["game/language"].getOldValue().empty()) {
				std::cout << "Wanting to change something, old value: '" << currentLanguageStr << "' new value: '" << newLanguagestr << "'" << std::endl;
				entryNode->set_attribute("value", std::to_string(newLanguageId));
				config["game/language"].selectEnum(newLanguagestr);
				TranslationEngine::setLanguage(newLanguagestr, true);
			}
			else {
				entryNode->set_attribute("value", std::to_string(currentLanguageId));
			}
		}
		else if (type == "int") entryNode->set_attribute("value", std::to_string(item.i()));
		else if (type == "uint") entryNode->set_attribute("value",std::to_string(item.ui()));
		else if (type == "bool") entryNode->set_attribute("value", item.b() ? "true" : "false");
		else if (type == "float") entryNode->set_attribute("value", std::to_string(item.f()));
		else if (type == "string") xmlpp::add_child_element(entryNode, "stringvalue")->add_child_text(item.s());
		else if (type == "string_list") {
			for (auto const& str : item.sl()) {
				xmlpp::add_child_element(entryNode, "stringvalue")->add_child_text(str);
			}
		}
		else if (type == "option_list") {
			//TODO: Write selected also (as attribute?)
			auto const selectedValue = item.so();
			xmlpp::add_child_element(entryNode, "stringvalue")->add_child_text(selectedValue);

			for (auto const& str : item.ol()) {
				if(str != selectedValue)
					xmlpp::add_child_element(entryNode, "stringvalue")->add_child_text(str);
			}
		}
	}
	fs::path const& conf = system ? systemConfFile : userConfFile;
	std::string tmp = conf.string() + "tmp";
	try {
		create_directories(conf.parent_path());
		if (dirty) doc.write_to_file_formatted(tmp, "UTF-8");
		if (exists(conf)) remove(conf);
		if (dirty) {
			rename(tmp, conf);
			std::cerr << "Saved configuration to " << conf << std::endl;
		}
		else {
			std::cerr << "Using default settings, no configuration file needed." << std::endl;
		}
	}
	catch (...) {
		throw std::runtime_error("Unable to save " + conf.string());
	}
	if (!system) return;
	// Tell the items that we have changed the system default
	for (auto& elem : config) elem.second.makeSystem();
	// User config is no longer needed
	if (exists(userConfFile)) remove(userConfFile);
}

ConfigMenu configMenu;

void readMenuXML(xmlpp::Node* node) {
	xmlpp::Element& elem = dynamic_cast<xmlpp::Element&>(*node);
	MenuEntry me;
	me.name = xmlpp::getAttribute(elem, "name");
	me.shortDesc = getText(elem, "short");
	me.longDesc = getText(elem, "long");
	configMenu.push_back(me);
}

void readConfigXML(fs::path const& file, unsigned mode) {
	if (!fs::exists(file)) {
		std::clog << "config/info: Skipping " << file << " (not found)" << std::endl;
		return;
	}
	std::clog << "config/info: Parsing " << file << std::endl;
	xmlpp::DomParser domParser(file.string());
	try {
		auto n = domParser.get_document()->get_root_node()->find("/performous/menu/entry");
		if (!n.empty()) {
			configMenu.clear();
			std::for_each(n.begin(), n.end(), readMenuXML);
		}
		n = domParser.get_document()->get_root_node()->find("/performous/entry");
		for (auto nodeit = n.begin(), end = n.end(); nodeit != end; ++nodeit) {
			xmlpp::Element& elem = dynamic_cast<xmlpp::Element&>(**nodeit);
			std::string name = xmlpp::getAttribute(elem, "name");
			if (name.empty()) throw std::runtime_error(file.string() + " element Entry missing name attribute");
			auto it = config.find(name);
			if (mode == 0) { // Schema
				if (it != config.end())
					throw std::runtime_error("Configuration schema contains the same value twice: " + name);
				auto& item = config[name];
				ConfigItemXMLLoader().update(item, elem, 0);
				// Add the item to menu, if not hidden
				bool hidden = false;
				try {
					if (xmlpp::getAttribute(elem, "hidden") == "true") hidden = true;
				}
				catch (xmlpp::XMLError&) {}
				if (!hidden) {
					for (auto& elem : configMenu) {
						const auto prefix = elem.name + '/';
						if (name.substr(0, prefix.size()) == prefix) {
							elem.items.push_back(name);
							break;
						}
					}
				}
				if (item.getName() == "game/language")
					item.setGetValueFunction(getValue_language);
				else if (item.getName() == "audio/backend")
					item.setGetValueFunction(getValue_audio_backend);
			}
			else {
				if (it == config.end()) {
					std::clog << "config/warning:   Entry " << name << " ignored (does not exist in config schema)." << std::endl;
					continue;
				}
				ConfigItemXMLLoader().update(it->second, elem, mode);
			}
		}
	}
	catch (xmlpp::XMLError& e) {
		int line = e.elem.get_line();
		std::string name = e.elem.get_name();
		throw std::runtime_error(file.string() + ":" + std::to_string(line) + " element " + name + " " + e.message);
	}
	catch (std::exception& e) {
		throw std::runtime_error(file.string() + ":" + e.what());
	}
}

unsigned short LanguageToLanguageId(const std::string& name) {
	if (name == "Asturian") return 1;
	if (name == "Danish") return 2;
	if (name == "German") return 3;
	if (name == "English") return 4;
	if (name == "Spanish") return 5;
	if (name == "Persian") return 6;
	if (name == "Finnish") return 7;
	if (name == "French") return 8;
	if (name == "Hungarian") return 9;
	if (name == "Italian") return 10;
	if (name == "Japanese") return 11;
	if (name == "Dutch") return 12;
	if (name == "Polish") return 13;
	if (name == "Portuguese") return 14;
	if (name == "Slovak") return 15;
	if (name == "Swedish") return 16;
	if (name == "Chinese") return 17;

	return 1337; // if no name matched return "Auto" which translates to computer language OR English.
}

void readConfig() {
	// Find config schema
	fs::path schemaFile = getSchemaFilename();
	systemConfFile = getSysConfigDir() / "config.xml";
	userConfFile = getConfigDir() / "config.xml";
	readConfigXML(schemaFile, 0);  // Read schema and defaults
	readConfigXML(systemConfFile, 1);  // Update defaults with system config
	readConfigXML(userConfFile, 2);  // Read user settings
	pathInit();
	// Populate themes
	ConfigItem& ci = config["game/theme"];
	for (auto const& theme : getThemes())
		ci.addEnum(theme);
	if (ci.ui() == 1337)
		ci.selectEnum("default");  // Select the default theme if nothing is selected
}

void populateBackends(const std::list<std::string>& backendList) {
	ConfigItem& backendConfig = config["audio/backend"];
	for (std::string const& backend : backendList) backendConfig.addEnum(backend);
	static std::string selectedBackend = std::string();
	selectedBackend = backendConfig.getValue();
	backendConfig.selectEnum(selectedBackend);
	backendConfig.setOldValue(backendConfig.getEnumName());
}

void populateLanguages(const std::map<std::string, std::string>& languages) {
	ConfigItem& languageConfig = config["game/language"];
	for (auto const& language : languages) {
		languageConfig.addEnum(language.second);
	}
	languageConfig.selectEnum(languageConfig.getValue());
	languageConfig.setOldValue(languageConfig.getEnumName());
}

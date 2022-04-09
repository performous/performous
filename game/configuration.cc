#include "audio.hh"
#include "configuration.hh"
#include "libxml++-impl.hh"

#include "fs.hh"
#include "util.hh"
#include "i18n.hh"
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

template<typename... Fs> struct Dispatch: Fs... { using Fs::operator()...; };
template<typename... Fs> Dispatch(Fs...) -> Dispatch<Fs...>;

Config config;

ConfigItem::ConfigItem(Value val): m_value(std::move(val)) { }

ConfigItem& ConfigItem::incdec(int dir) {
	std::visit(
        Dispatch{
                [&] (Numerical<int> &value) {
        	value.value = clamp(((value.value + dir * value.m_step)/ value.m_step) * value.m_step, value.m_min, value.m_max);
        },
        [&] (Numerical<double> &value) {
        	value.value = clamp(round((value.value + dir * value.m_step) / value.m_step) * value.m_step, value.m_min, value.m_max);
        },
        [&] (bool& val) {
        	val = !val;
        },
        [&] (OptionList &ol) {
        	size_t s = ol.options.size();
        	ol.m_sel = (ol.m_sel + dir + s) % s;
        },
        [&] (Enum &e) {
           auto it = e.names.find(e.value);
           if (dir == 1) {
		   if (it == e.names.end() || ++it == e.names.end())
			   e.value = *e.names.begin();
		   else
			   e.value = *it;
           } else {
		   if (it == e.names.begin() || --it == e.names.begin())
			   e.value = *e.names.rbegin();
		   else
			   e.value = *it;
	   }
        },
        [&] (auto &) {
           throw std::logic_error("Calling incdec on an improper ConfigItem<" + std::to_string(m_value.index()) + ">");
        }
       }, m_value);
	return *this;
}

bool ConfigItem::isDefaultImpl(ConfigItem::Value const& defaultValue) const {
      return defaultValue == m_value;
}

template <typename T> auto ConfigItem::getChecker() -> T& {
    return const_cast<T&>(((const ConfigItem *)(this))->getChecker<T>());
}

template <typename T> const auto &ConfigItem::getChecker() const {
    if (!std::holds_alternative<T>(m_value))
       throw std::logic_error("Config item type mismatch: '" + m_keyName + "', type=" + std::to_string(m_value.index()) + ", requested=" + std::to_string(Value(T()).index()));

   return std::get<T>(m_value);
}

int& ConfigItem::i() { return getChecker<Numerical<int>>().value; }
int const& ConfigItem::i() const { return getChecker<Numerical<int>>().value; }
bool& ConfigItem::b() { return getChecker<bool>(); }
const bool& ConfigItem::b() const { return getChecker<bool>(); }
double& ConfigItem::f() {  return getChecker<Numerical<double>>().value; }
std::string& ConfigItem::s() { return std::get<std::string>(m_value); }
ConfigItem::StringList& ConfigItem::sl() { return getChecker<StringList>(); }
ConfigItem::OptionList& ConfigItem::ol() { return getChecker<OptionList>(); }

std::string& ConfigItem::so() {  return ol().options.at(ol().m_sel); }

void ConfigItem::select(int i) { ol().m_sel = clamp<int>(i, 0, ol().options.size()-1); }

namespace {
	template <typename T> std::string numericFormat(const ConfigItem::Numerical<T>& value) {
		// Find suitable precision (not very useful for integers, but this code is generic...)
		T s = std::abs(value.m_multiplier * value.m_step);
		unsigned precision = 0;
		while (s > 0.0 && (s *= 10) < 10) ++precision;
		// Not quite sure how to format this with FMT
		return fmt::format("{:.{}g}", double(value.m_multiplier) * value.value, precision);
	}

	std::string getText(xmlpp::Element const& elem) {
		auto n = xmlpp::get_first_child_text(elem);  // Returns NULL if there is no text
		return n ? std::string(n->get_content()) : std::string();
	}

	std::string getText(xmlpp::Element const& elem, std::string const& path) {
		auto ns = elem.find(path);
		if (ns.empty()) return std::string();
		return getText(dynamic_cast<xmlpp::Element const&>(*ns[0]));
	}
}

std::string ConfigItem::toString() const {
        return std::visit(Dispatch{
        [] (const Enum &e) {
		return e.value;
        },
        [] (const Numerical<int> &val) {
		return numericFormat(val) + _(val.m_unit);
	},
        [] (const Numerical<double> &val) {
            return numericFormat(val) + _(val.m_unit);
        },
        [] (const bool& val) {
	    return val ? _("Enabled") : _("Disabled");
        },
        [] (const std::string& val) {
            return val;
        },
        [] (const StringList &sl) {
	    return sl.size() == 1 ? "{" + sl[0] + "}" : fmt::format(_("{:d} items"), sl.size());
	},
        [] (const OptionList &ol) {
	    return ol.options.at(ol.m_sel);
        }}, m_value);
}

namespace {
	struct XMLError {
		XMLError(xmlpp::Element& e, std::string msg): elem(e), message(msg) {}
		xmlpp::Element& elem;
		std::string message;
	};
	std::string getAttribute(xmlpp::Element& elem, std::string const& attr) {
		xmlpp::Attribute* a = elem.get_attribute(attr);
		if (!a) throw XMLError(elem, "attribute " + attr + " not found");
		return a->get_value();
	}
	template <typename T, typename V> void setLimits(xmlpp::Element& e, V& min, V& max, V& step) {
		xmlpp::Attribute* a = e.get_attribute("min");
		if (a) min = sconv<T>(a->get_value());
		a = e.get_attribute("max");
		if (a) max = sconv<T>(a->get_value());
		a = e.get_attribute("step");
		if (a) step = sconv<T>(a->get_value());
	}
}

void ConfigItem::addEnum(std::string name) {
        auto &e = getChecker<Enum>();
        e.names.insert(name);
}

void ConfigItem::selectEnum(std::string const& name) {
        auto &e = getChecker<Enum>();

	auto it = std::find(e.names.begin(), e.names.end(), name);
	if (it == e.names.end()) throw std::runtime_error("Enum value " + name + " not found in " + m_shortDesc);
	e.value = name;
}


std::string const ConfigItem::getEnumName() const {
        return getChecker<Enum>().value;
}

template <typename T> void ConfigItem::updateNumeric(xmlpp::Element& elem, int mode) {
        auto &n = getChecker<Numerical<T>>();

	auto ns = elem.find("limits");
	if (!ns.empty()) setLimits<T>(dynamic_cast<xmlpp::Element&>(*ns[0]), n.m_min, n.m_max, n.m_step);
	else if (mode == 0) throw XMLError(elem, "child element limits missing");
        n.value = clamp((n.value/ n.m_step) * n.m_step, n.m_min, n.m_max);
	ns = elem.find("ui");
	// Default values
	if (mode == 0) {
		n.m_unit.clear();
		n.m_multiplier = static_cast<T>(1);
	}
	if (!ns.empty()) {
		xmlpp::Element& e = dynamic_cast<xmlpp::Element&>(*ns[0]);
		try { n.m_unit = getAttribute(e, "unit"); } catch (...) {}
		std::string m;
		try {
			m = getAttribute(e, "multiplier");
			n.m_multiplier = sconv<T>(m);
		} catch (XMLError&) {}
		catch (std::exception&) { throw XMLError(e, "attribute multiplier='" + m + "' value invalid"); }
	}
}

void ConfigItem::update(xmlpp::Element& elem, int mode) try {
        std::string m_type;
	if (mode == 0) {
		m_keyName = getAttribute(elem, "name");
		m_type = getAttribute(elem, "type");
		if (m_type.empty()) throw std::runtime_error("Entry type attribute is missing");
		// Menu text
		m_shortDesc = getText(elem, "short");
		m_longDesc = getText(elem, "long");
	} else {
		m_type = getAttribute(elem, "type");
		if (m_type.empty()) throw std::runtime_error("Entry type attribute is missing: " + getAttribute(elem, "name"));
                if (mode == 1) m_value = m_factoryDefaultValue;
                if (mode == 2) m_value = m_defaultValue;
	}
	if (m_type == "bool") {
		std::string value_string = getAttribute(elem, "value");
		bool value;
		if (value_string == "true") value = true;
			else if (value_string == "false") value = false;
				else throw std::runtime_error("Invalid boolean value '" + value_string + "'");
		m_value = value;
	} else if (m_type == "enum") {
		std::string value_string = getAttribute(elem, "value");
		if (value_string.empty()) throw std::runtime_error("Value for enum '" + m_keyName + "' is missing");
                auto n2 = elem.find("limits/enum");
                auto e = mode == 0 ? Enum{} : std::get<Enum>(m_value);
                for (auto it2 = n2.begin(), end2 = n2.end(); it2 != end2; ++it2) {
                    xmlpp::Element& elem2 = dynamic_cast<xmlpp::Element&>(**it2);
                    e.names.insert(getText(elem2));
                }
                e.value = value_string;
                m_value = e;
	} else if (m_type == "int") {
		std::string value_string = getAttribute(elem, "value");
		if (!value_string.empty()) {
			auto n2 = elem.find("limits/enum");
			if (!std::holds_alternative<Enum>(m_value) && m_type != "enum" && (mode != 0 || n2.empty())) {
                                auto v = mode == 0 ? Numerical<int>{} : std::get<Numerical<int>>(m_value);
				v.value = std::stoi(value_string);
                                m_value = v;
				updateNumeric<int>(elem, mode);
			} else {
				// Hack for backward compat Enum handling
				Enum &e = getChecker<Enum>();
				for (auto it2 = n2.begin(), end2 = n2.end(); it2 != end2; ++it2) {
					xmlpp::Element& elem2 = dynamic_cast<xmlpp::Element&>(**it2);
					e.names.insert(getText(elem2));
				}
				e.value = value_string;
			}
		}
	} else if (m_type == "float") {
		std::string value_string = getAttribute(elem, "value");
		if (!value_string.empty()) {
                    auto v = mode == 0 ? Numerical<double>{} : std::get<Numerical<double>>(m_value);
                    v.value = std::stod(value_string);
                    m_value = v;
                }
		updateNumeric<double>(elem, mode);
	} else if (m_type == "string") {
		m_value = getText(elem, "stringvalue");
	} else if (m_type == "string_list" || m_type == "option_list") {
		//TODO: Option list should also update selection (from attribute?)
		std::vector<std::string> value;
		auto n2 = elem.find("stringvalue");
		for (auto it2 = n2.begin(), end2 = n2.end(); it2 != end2; ++it2) {
			value.push_back(getText(dynamic_cast<xmlpp::Element const&>(**it2)));
		}
		m_value = value;
	} else if (!m_type.empty()) throw std::runtime_error("Invalid value type in config schema: " + m_type);
	// Schema sets all defaults, system config sets the system default
	if (mode < 1) m_factoryDefaultValue = m_defaultValue = m_value;
	if (mode < 2) m_defaultValue = m_value;
} catch (std::exception& e) {
        int line = elem.get_line();
        throw std::runtime_error(std::to_string(line) + ": Error while reading entry: " + e.what());
}

std::string ConfigItem::get_type_name() const {
    return std::visit(Dispatch{
        [] (const Enum &) { return "enum"; },
        [] (const Numerical<int> &) { return "int"; },
        [] (const Numerical<double> &) { return "float"; },
        [] (const bool& ) { return "bool"; },
        [] (const std::string&) { return "string"; },
        [] (const StringList &) { return "string_list"; },
        [] (const OptionList &) { return "option_list"; },
    }, m_value);
}

// These are set in readConfig, once the paths have been bootstrapped.
fs::path systemConfFile;
fs::path userConfFile;

void writeConfig(bool system) {
	xmlpp::Document doc;
	auto nodeRoot = doc.create_root_node("performous");
	bool dirty = false;
	for (auto& elem: config) {
		const ConfigItem& item = elem.second;
		std::string name = elem.first;
		if (item.isDefault(system) && name != "audio/backend" && name != "graphic/stereo3d") continue; // No need to save settings with default values
		dirty = true;
		xmlpp::Element* entryNode = xmlpp::add_child_element(nodeRoot, "entry");
		entryNode->set_attribute("name", name);
		std::string type = item.get_type_name();
		entryNode->set_attribute("type", type);
		item.visit(Dispatch{
				[&] (const ConfigItem::Numerical<int> &val) { entryNode->set_attribute("value", std::to_string(val.value)); },
				[&] (const ConfigItem::Numerical<double> &val) { entryNode->set_attribute("value", std::to_string(val.value)); },
				[&] (const ConfigItem::Enum &e) { entryNode->set_attribute("value", e.value); },
				[&] (const bool &val) { entryNode->set_attribute("value", val ? "true" : "false"); },
				[&] (const std::string &val) { xmlpp::add_child_element(entryNode, "stringvalue")->add_child_text(val); },
				[&] (const ConfigItem::StringList &val) { for (auto const& str: val) xmlpp::add_child_element(entryNode, "stringvalue")->add_child_text(str); },
				[&] (const ConfigItem::OptionList &val) {
					//TODO: Write selected also (as attribute?)
					for (auto const& str: val.options) xmlpp::add_child_element(entryNode, "stringvalue")->add_child_text(str); }
				}); 
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
		} else {
			std::cerr << "Using default settings, no configuration file needed." << std::endl;
		}
	} catch (...) {
		throw std::runtime_error("Unable to save " + conf.string());
	}
	if (!system) return;
	// Tell the items that we have changed the system default
	for (auto& elem: config) elem.second.makeSystem();
	// User config is no longer needed
	if (exists(userConfFile)) remove(userConfFile);
}

ConfigMenu configMenu;

void readMenuXML(xmlpp::Node* node) {
	xmlpp::Element& elem = dynamic_cast<xmlpp::Element&>(*node);
	MenuEntry me;
	me.name = getAttribute(elem, "name");
	me.shortDesc = getText(elem, "short");
	me.longDesc = getText(elem, "long");
	configMenu.push_back(me);
}

void readConfigXML(fs::path const& file, int mode) {
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
			std::string name = getAttribute(elem, "name");
			if (name.empty()) throw std::runtime_error(file.string() + " element Entry missing name attribute");
			auto it = config.find(name);
			if (mode == 0) { // Schema
				if (it != config.end()) throw std::runtime_error("Configuration schema contains the same value twice: " + name);
				config[name].update(elem, 0);
				// Add the item to menu, if not hidden
				bool hidden = false;
				try { if (getAttribute(elem, "hidden") == "true") hidden = true; } catch (XMLError&) {}
				if (!hidden) {
					for (auto& elem: configMenu) {
						std::string prefix = elem.name + '/';
						if (name.substr(0, prefix.size()) == prefix) { elem.items.push_back(name); break; }
					}
				}
			} else {
				if (it == config.end()) {
					std::clog << "config/warning:   Entry " << name << " ignored (does not exist in config schema)." << std::endl;
					continue;
				}
				it->second.update(elem, mode);
			}
		}
	} catch (XMLError& e) {
		int line = e.elem.get_line();
		std::string name = e.elem.get_name();
		throw std::runtime_error(file.string() + ":" + std::to_string(line) + " element " + name + " " + e.message);
	} catch (std::exception& e) {
		throw std::runtime_error(file.string() + ":" + e.what());
	}
}

int PaHostApiNameToHostApiTypeId (const std::string& name) {
	if (name == "Auto") return 1337;
	if (name == "Windows DirectSound") return 1;
	if (name == "MME") return 2;
	if (name == "ASIO") return 3;
	if (name == "Core Audio" || name == "CoreAudio") return 5;
	if (name == "OSS") return 7; // Not an error, stupid PortAudio.
	if (name == "ALSA") return 8;
	if (name == "Windows WDM-KS") return 11;
	if (name == "JACK Audio Connection Kit") return 12;
	if (name == "Windows WASAPI") return 13;

        // workaround former enum stored as int
        try { return std::stoi(name); }
        catch (const std::exception &) { /* whatever, the port audio was not found, can use the common fallback */ }

	throw std::runtime_error("Invalid PortAudio HostApiTypeId '" + name + "'Specified.");
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
	for (std::string const& theme: getThemes()) ci.addEnum(theme);
	if (ci.getEnumName() == "") ci.selectEnum("default");  // Select the default theme if nothing is selected
}

void populateBackends (const std::vector<std::string>& backendList) {
	ConfigItem& backendConfig = config["audio/backend"];
	for (std::string const& backend: backendList) backendConfig.addEnum(backend);
}

// Helper function to convert deprecated lang format of the config.xml.
// Should be eventually removed as we drop the backward compat with this old
// game versions
static std::string LanguageIdToLanguage(const unsigned int& id) {
	switch (id) {
		case 1: return "Asturian";
		case 2: return "Danish";
		case 3: return "German";
		case 4: return "English";
		case 5: return "Spanish";
		case 6: return "Persian";
		case 7: return "Finnish";
		case 8: return "French";
		case 9: return "Hungarian";
		case 10: return "Italian";
		case 11: return "Japanese";
		case 12: return "Dutch";
		case 13: return "Polish";
		case 14: return "Portuguese";
		case 15: return "Slovak";
		case 16: return "Swedish";
		case 17: return "Chinese";
	}
	return "Auto"; // if no name matched (may be the magic 1337 value return "Auto" which translates to computer language OR English.
}

void populateLanguages(const std::map<std::string, std::string>& languages) {
	ConfigItem& languageConfig = config["game/language"];
	for (auto const& language : languages) {
		languageConfig.addEnum(language.second);
	}

	// workaround former configuation that was storing lang as a number
        const auto &selected = [&] { 
             const auto &l = languageConfig.toString();
             try { // try to check if value stored is a number
                 auto id = std::stoi(languageConfig.toString());
                 return LanguageIdToLanguage(id);
             } catch (const std::exception &) {
                 return l; //return non numeric value; should be the lang itself
             }
	}();

	languageConfig.selectEnum(selected);
}

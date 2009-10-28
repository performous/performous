#include "configuration.hh"

#include "fs.hh"
#include "util.hh"
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <libxml++/libxml++.h>
#include <algorithm>
#include <iomanip>
#include <stdexcept>
#include <iostream>
#include <cmath>

Config config;

ConfigItem& ConfigItem::incdec(int dir) {
	if (m_type == "int") {
		int& val = boost::get<int>(m_value);
		int step = boost::get<int>(m_step);
		val = clamp(((val + dir * step)/ step) * step, boost::get<int>(m_min), boost::get<int>(m_max));
	} else if (m_type == "float") {
		double& val = boost::get<double>(m_value);
		double step = boost::get<double>(m_step);
		val = clamp(round((val + dir * step) / step) * step, boost::get<double>(m_min), boost::get<double>(m_max));
	} else if (m_type == "bool") {
		bool& val = boost::get<bool>(m_value);
		val = !val;
	}
	return *this;
}

bool ConfigItem::isDefaultImpl(ConfigItem::Value const& defaultValue) const {
	if (m_type == "bool") return boost::get<bool>(m_value) == boost::get<bool>(defaultValue);
	if (m_type == "int") return boost::get<int>(m_value) == boost::get<int>(defaultValue);
	if (m_type == "float") return boost::get<double>(m_value) == boost::get<double>(defaultValue);
	if (m_type == "string") return boost::get<std::string>(m_value) == boost::get<std::string>(defaultValue);
	if (m_type == "string_list") return boost::get<StringList>(m_value) == boost::get<StringList>(defaultValue);
	throw std::logic_error("ConfigItem::is_default doesn't know type '" + m_type + "'");
}

void ConfigItem::verifyType(std::string const& type) const {
	if (type == m_type) return;
	std::string name = "unknown";
	// Try to find this item in the config map
	for (Config::const_iterator it = config.begin(); it != config.end(); ++it) {
		if (&it->second == this) { name = it->first; break; }
	}
	if (m_type.empty()) throw std::logic_error("Config item " + name + " used in C++ but missing from config schema");
	throw std::logic_error("Config item type mismatch: item=" + name + ", type=" + m_type + ", requested=" + type);
}

int& ConfigItem::i() { verifyType("int"); return boost::get<int>(m_value); }
bool& ConfigItem::b() { verifyType("bool"); return boost::get<bool>(m_value); }
double& ConfigItem::f() { verifyType("float"); return boost::get<double>(m_value); }
std::string& ConfigItem::s() { verifyType("string"); return boost::get<std::string>(m_value); }
ConfigItem::StringList& ConfigItem::sl() { verifyType("string_list"); return boost::get<StringList>(m_value); }

namespace {
	template <typename T, typename VariantAll, typename VariantNum> std::string numericFormat(VariantAll const& value, VariantNum const& multiplier, VariantNum const& step) {
		T m = boost::get<T>(multiplier);
		// Find suitable precision (not very useful for integers, but this code is generic...)
		T s = std::abs(m * boost::get<T>(step));
		unsigned precision = 0;
		while (s > 0.0 && (s *= 10) < 10) ++precision;
		// Format the output
		boost::format fmter("%f");
		fmter % boost::io::group(std::setprecision(precision), double(m) * boost::get<T>(value));
		return fmter.str();
	}
}

std::string ConfigItem::getValue() const {
	if (m_type == "int") return numericFormat<int>(m_value, m_multiplier, m_step) + m_unit;
	if (m_type == "float") return numericFormat<double>(m_value, m_multiplier, m_step) + m_unit;
	if (m_type == "bool") return boost::get<bool>(m_value) ? "Enabled" : "Disabled";
	if (m_type == "string") return boost::get<std::string>(m_value);
	if (m_type == "string_list") {
		StringList const& sl = boost::get<StringList>(m_value);
		return sl.size() == 1 ? "{" + sl[0] + "}" : (boost::format("%d items") % sl.size()).str();
	}
	throw std::logic_error("ConfigItem::getValue doesn't know type '" + m_type + "'");
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
	std::string getLocaleText(xmlpp::Element& elem, std::string const& name) {
		std::string str;
		xmlpp::NodeSet n = elem.find("locale/" + name + "/text()"); // TODO: could pick specific locale
		for (xmlpp::NodeSet::const_iterator it = n.begin(), end = n.end(); it != end; ++it) {
			xmlpp::TextNode& elem2 = dynamic_cast<xmlpp::TextNode&>(**it);
			str = elem2.get_content();
		}
		return str;
	}

	template <typename T, typename V> void setLimits(xmlpp::Element& e, V& min, V& max, V& step) {
		std::string value = getAttribute(e, "min");
		if (!value.empty()) min = boost::lexical_cast<T>(value);
		value = getAttribute(e, "max");
		if (!value.empty()) max = boost::lexical_cast<T>(value);
		value = getAttribute(e, "step");
		if (!value.empty()) step = boost::lexical_cast<T>(value);
	}
}

template <typename T> void ConfigItem::updateNumeric(xmlpp::Element& elem, int mode) {
	xmlpp::NodeSet ns = elem.find("limits");
	if (!ns.empty()) setLimits<T>(dynamic_cast<xmlpp::Element&>(*ns[0]), m_min, m_max, m_step);
	else if (mode == 0) throw XMLError(elem, "child element limits missing");
	ns = elem.find("ui");
	// Default values
	if (mode == 0) {
		m_unit.clear();
		m_multiplier = static_cast<T>(1);
	}
	if (!ns.empty()) {
		xmlpp::Element& e = dynamic_cast<xmlpp::Element&>(*ns[0]);
		try { m_unit = getAttribute(e, "unit"); } catch (...) {}
		std::string m;
		try {
			m = getAttribute(e, "multiplier");
			m_multiplier = boost::lexical_cast<T>(m);
		} catch (XMLError&) {}
		catch (boost::bad_lexical_cast&) { throw XMLError(e, "attribute multiplier='" + m + "' value invalid"); }
	}
}

void ConfigItem::update(xmlpp::Element& elem, int mode) {
	if (mode == 0) {
		m_type = getAttribute(elem, "type");
		if (m_type.empty()) throw std::runtime_error("Entry type attribute is missing");
	}
	if (m_type == "bool") {
		std::string value_string = getAttribute(elem, "value");
		bool value;
		if (value_string == "true") value = true;
		else if (value_string == "false") value = false;
		else throw std::runtime_error("Invalid boolean value '" + value_string + "'");
		m_value = value;
	} else if (m_type == "int") {
		std::string value_string = getAttribute(elem, "value");
		if (!value_string.empty()) m_value = boost::lexical_cast<int>(value_string);
		updateNumeric<int>(elem, mode);
	} else if (m_type == "float") {
		std::string value_string = getAttribute(elem, "value");
		if (!value_string.empty()) m_value = boost::lexical_cast<double>(value_string);
		updateNumeric<double>(elem, mode);
	} else if (m_type == "string") {
		xmlpp::NodeSet n2 = elem.find("stringvalue/text()");
		// FIXME: WTF does this loop do? Does find actually return many elements and why?
		std::string value;
		for (xmlpp::NodeSet::const_iterator it2 = n2.begin(), end2 = n2.end(); it2 != end2; ++it2) {
			xmlpp::TextNode& elem2 = dynamic_cast<xmlpp::TextNode&>(**it2);
			value = elem2.get_content();
		}
		m_value = value;
	} else if (m_type == "string_list") {
		std::vector<std::string> value;
		xmlpp::NodeSet n2 = elem.find("stringvalue/text()");
		for (xmlpp::NodeSet::const_iterator it2 = n2.begin(), end2 = n2.end(); it2 != end2; ++it2) {
			xmlpp::TextNode& elem2 = dynamic_cast<xmlpp::TextNode&>(**it2);
			value.push_back(elem2.get_content());
		}
		m_value = value;
	}

	{
		// Update short description
		xmlpp::NodeSet n2 = elem.find("locale/short/text()");
		for (xmlpp::NodeSet::const_iterator it2 = n2.begin(), end2 = n2.end(); it2 != end2; ++it2) {
			xmlpp::TextNode& elem2 = dynamic_cast<xmlpp::TextNode&>(**it2);
			m_shortDesc = elem2.get_content();
		}
	}
	{
		// Update long description
		xmlpp::NodeSet n2 = elem.find("locale/long/text()");
		for (xmlpp::NodeSet::const_iterator it2 = n2.begin(), end2 = n2.end(); it2 != end2; ++it2) {
			xmlpp::TextNode& elem2 = dynamic_cast<xmlpp::TextNode&>(**it2);
			m_longDesc = elem2.get_content();
		}
	}
	if (mode < 1) m_factoryDefaultValue = m_defaultValue = m_value;
	if (mode < 2) m_defaultValue = m_value;
}

fs::path systemConfFile = "/etc/xdg/performous/config.xml";
fs::path userConfFile = getHomeDir() / ".config" / "performous" / "config.xml";

void writeConfig(bool system) {
	xmlpp::Document doc;
	xmlpp::Node* nodeRoot = doc.create_root_node("performous");
	bool dirty = false;
	for (Config::iterator it = config.begin(); it != config.end(); ++it) {
		ConfigItem& item = it->second;
		std::string name = it->first;
		if (item.isDefault(system)) continue; // No need to save settings with default values
		dirty = true;
		xmlpp::Element* entryNode = nodeRoot->add_child("entry");
		entryNode->set_attribute("name", name);
		std::string type = item.get_type();
		entryNode->set_attribute("type", type);
		if (type == "int") entryNode->set_attribute("value",boost::lexical_cast<std::string>(item.i()));
		else if (type == "bool") entryNode->set_attribute("value", item.b() ? "true" : "false");
		else if (type == "float") entryNode->set_attribute("value",boost::lexical_cast<std::string>(item.f()));
		else if (item.get_type() == "string") entryNode->add_child("stringvalue")->add_child_text(item.s());
		else if (item.get_type() == "string_list") {
			std::vector<std::string> const& value = item.sl();
			for(std::vector<std::string>::const_iterator it = value.begin(); it != value.end(); ++it) {
				xmlpp::Element* stringvalueNode = entryNode->add_child("stringvalue");
				stringvalueNode->add_child_text(*it);
			}
		}
	}
	fs::path const& conf = system ? systemConfFile : userConfFile;
	std::string tmp = conf.string() + "tmp";
	try {
		create_directory(conf.parent_path());
		if (dirty) doc.write_to_file_formatted(tmp, "UTF-8");
		if (exists(conf)) remove(conf);
		if (dirty) {
			rename(tmp, conf);
			std::cerr << "Saved configuration to \"" << conf << "\"" << std::endl;
		} else {
			std::cerr << "Using default settings, no configuration file needed." << std::endl;
		}
	} catch (...) {
		throw std::runtime_error("Unable to save " + conf.string());
	}
	if (!system) return;
	// Tell the items that we have changed the system default
	for (Config::iterator it = config.begin(); it != config.end(); ++it) it->second.makeSystem();
	// User config is no longer needed
	if (exists(userConfFile)) remove(userConfFile);
}

ConfigMenu configMenu;

void readMenuXML(xmlpp::Node* node) {
	xmlpp::Element& elem = dynamic_cast<xmlpp::Element&>(*node);
	MenuEntry me;
	me.name = getAttribute(elem, "name");
	me.shortDesc = getLocaleText(elem, "short");
	me.longDesc = getLocaleText(elem, "long");
	configMenu.push_back(me);
}

void readConfigXML(fs::path const& file, int mode) {
	if (!boost::filesystem::exists(file)) {
		std::cout << "Skipping " << file << " (not found)" << std::endl;
		return;
	}
	std::cout << "Parsing " << file << std::endl;
	xmlpp::DomParser domParser(file.string());
	try {
		xmlpp::NodeSet n = domParser.get_document()->get_root_node()->find("/performous/menu/entry");
		if (!n.empty()) {
			configMenu.clear();
			std::for_each(n.begin(), n.end(), readMenuXML);
		}
		n = domParser.get_document()->get_root_node()->find("/performous/entry");
		for (xmlpp::NodeSet::const_iterator nodeit = n.begin(), end = n.end(); nodeit != end; ++nodeit) {
			xmlpp::Element& elem = dynamic_cast<xmlpp::Element&>(**nodeit);
			std::string name = getAttribute(elem, "name");
			if (name.empty()) throw std::runtime_error(file.string() + " element Entry missing name attribute");
			Config::iterator it = config.find(name);
			if (mode == 0) { // Schema
				if (it != config.end()) throw std::runtime_error("Configuration schema contains the same value twice: " + name);
				config[name].update(elem, 0);
				// Add the item to menu, if not hidden
				bool hidden = false;
				try { if (getAttribute(elem, "hidden") == "true") hidden = true; } catch (XMLError&) {}
				if (!hidden) {
					for (ConfigMenu::iterator it = configMenu.begin(), end = configMenu.end(); it != end; ++it) {
						std::string prefix = it->name + '/';
						if (name.substr(0, prefix.size()) == prefix) { it->items.push_back(name); break; }
					}
				}
			} else {
				if (it == config.end()) {
					std::cout << "  Entry " << name << " ignored (does not exist in config schema)." << std::endl;
					continue;
				}
				it->second.update(elem, mode);
			}
		}
	} catch (XMLError& e) {
		int line = e.elem.get_line();
		std::string name = e.elem.get_name();
		throw std::runtime_error(file.string() + ":" + boost::lexical_cast<std::string>(line) + " element " + name + " " + e.message);
	}
}

void readConfig() {
	// Find config schema
	std::string schemafile;
	{
		typedef std::vector<std::string> ConfigList;
		ConfigList config_list;
		char const* env_data_dir = getenv("PERFORMOUS_DATA_DIR");
		if (env_data_dir) {
			std::string config_file(env_data_dir);
			config_file.append("/config/schema.xml");
			config_list.push_back(config_file);
		}
		config_list.push_back("/usr/local/share/games/performous/config/schema.xml");
		config_list.push_back("/usr/local/share/performous/config/schema.xml");
		config_list.push_back("/usr/share/games/performous/config/schema.xml");
		config_list.push_back("/usr/share/performous/config/schema.xml");
		ConfigList::const_iterator it = std::find_if(config_list.begin(), config_list.end(), static_cast<bool(&)(boost::filesystem::path const&)>(boost::filesystem::exists));
		if (it == config_list.end()) {
			std::ostringstream oss;
			oss << "No config schema file found. The following locations were tried:\n";
			std::copy(config_list.begin(), config_list.end(), std::ostream_iterator<std::string>(oss, "\n"));
			oss << "Install the file or define environment variable PERFORMOUS_DATA_DIR\n";
			throw std::runtime_error(oss.str());
		}
		schemafile = *it;
	}
	readConfigXML(schemafile, 0);  // Read schema and defaults
	readConfigXML(systemConfFile, 1);  // Update defaults with system config
	readConfigXML(userConfFile, 2);  // Read user settings
}


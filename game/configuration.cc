#include "configuration.hh"
#include "screen.hh"
#include "util.hh"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <libxml++/libxml++.h>
#include <math.h>
#include <algorithm>
#include <stdexcept>

Config config;

template <typename T> ConfigItem::ConfigItem(std::string type, T const& defaultValue, std::string short_desc, std::string long_desc):
  m_type(m_type), m_value(defaultValue), m_defaultValue(defaultValue)
{
	if (m_type == "int") {
		m_min = std::numeric_limits<int>::min();
		m_max = std::numeric_limits<int>::max();
		m_step = 1;
	} else if (m_type == "float") {
		m_min = -getInf();
		m_max = getInf();
		m_step = 0.01;
	}
}

ConfigItem& ConfigItem::incdec(int dir) {
	if (m_type == "int") {
		int& val = boost::get<int>(m_value);
		int step = boost::get<int>(m_step);
		val = clamp(((val + dir * step)/ step) * step, boost::get<int>(m_min), boost::get<int>(m_max));
	} else if (m_type == "float") {
		double& val = boost::get<double>(m_value);
		double step = boost::get<double>(m_step);
		val = clamp(roundf((val + dir * step) / step) * step, boost::get<double>(m_min), boost::get<double>(m_max));
	} else if (m_type == "bool") {
		bool& val = boost::get<bool>(m_value);
		val = !val;
	}
	return *this;
}

bool ConfigItem::is_default() const {
	if (m_type == "bool") return boost::get<std::string>(m_value) == boost::get<std::string>(m_defaultValue);
	if (m_type == "int") return boost::get<int>(m_value) == boost::get<int>(m_defaultValue);
	if (m_type == "float") return boost::get<double>(m_value) == boost::get<double>(m_defaultValue);
	if (m_type == "string") return boost::get<std::string>(m_value) == boost::get<std::string>(m_defaultValue);
	if (m_type == "string_list") return boost::get<StringList>(m_value) == boost::get<StringList>(m_defaultValue);
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
	} else if (m_type == "float") {
		std::string value_string = getAttribute(elem, "value");
		if (!value_string.empty()) m_value = boost::lexical_cast<double>(value_string);
	} else if (m_type == "string") {
		xmlpp::NodeSet n2 = elem.find("stringvalue/text()");
		// FIXME: WTF does this loop do? Does find actually return many elements and why?
		std::string value("");
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
	if (mode < 2) m_defaultValue = m_value;
}

void writeConfigfile( const std::string &_configfile ) {
/*
	std::cout << "Saving configuration file \"" << _configfile << "\"" << std::endl;
	xmlpp::Document doc;
	xmlpp::Node* nodeRoot = doc.create_root_node("performous");
	for(std::map<std::string, ConfigItem>::const_iterator itr = config.begin(); itr != config.end(); ++itr) {
		ConfigItem item = (*itr).second;
		std::string name = (*itr).first;
		if(! item.is_default() ) {
			xmlpp::Element* entryNode = nodeRoot->add_child("entry");
			entryNode->set_attribute("name", name);

			if( item.get_type() == "int" ) {
				entryNode->set_attribute("type", "int");
				entryNode->set_attribute("value",boost::lexical_cast<std::string>(item.get_i()));
			} else if( item.get_type() == "bool" ) {
				entryNode->set_attribute("type", "bool");
				if( item.get_b()) {
					entryNode->set_attribute("value","true");
				} else {
					entryNode->set_attribute("value","false");
				}
			} else if( item.get_type() == "float" ) {
				entryNode->set_attribute("type", "float");
				entryNode->set_attribute("value",boost::lexical_cast<std::string>(item.get_f()));
			} else if( item.get_type() == "string" ) {
				entryNode->set_attribute("type", "string");
				xmlpp::Element* stringvalueNode = entryNode->add_child("stringvalue");
				stringvalueNode->add_child_text(item.get_s());
			} else if( item.get_type() == "string_list" ) {
				entryNode->set_attribute("type", "string_list");
				std::vector<std::string> value = item.get_sl();
				for(std::vector<std::string>::const_iterator it = value.begin(); it != value.end(); ++it) {
					xmlpp::Element* stringvalueNode = entryNode->add_child("stringvalue");
					stringvalueNode->add_child_text(*it);
				}
			}
		}
	}
	doc.write_to_file_formatted(_configfile);
	*/
}

void readConfigXML(std::string const& file, int mode) {
	if (!boost::filesystem::exists(file)) {
		std::cout << "Skipping " << file << " (not found)" << std::endl;
		return;
	}
	std::cout << "Parsing " << file << std::endl;
	xmlpp::DomParser domParser(file);
	try {
		xmlpp::NodeSet n = domParser.get_document()->get_root_node()->find("/performous/entry");
		for (xmlpp::NodeSet::const_iterator it = n.begin(), end = n.end(); it != end; ++it) {
			xmlpp::Element& elem = dynamic_cast<xmlpp::Element&>(**it);
			std::string name = getAttribute(elem, "name");
			if (name.empty()) throw std::runtime_error(file + " element Entry missing name attribute");
			Config::iterator it = config.find(name);
			if (mode == 0) { // Schema
				if (it != config.end()) throw std::runtime_error("Configuration schema contains the same value twice: " + name);
				config[name].update(elem, 0);
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
		throw std::runtime_error(file + ":" + boost::lexical_cast<std::string>(line) + " element " + name + " " + e.message);
	}
}

void readConfigfile(std::string const& userConf) {
	// Find config schema
	std::string schemafile;
	{
		typedef std::vector<std::string> ConfigList;
		ConfigList config_list;
		char const* env_config = getenv("PERFORMOUS_DEFAULT_CONFIG_FILE");
		if (env_config) config_list.push_back(env_config);
		config_list.push_back("/usr/local/share/games/performous/config/performous.xml");
		config_list.push_back("/usr/local/share/performous/config/performous.xml");
		config_list.push_back("/usr/share/games/performous/config/performous.xml");
		config_list.push_back("/usr/share/performous/config/performous.xml");
		ConfigList::const_iterator it = std::find_if(config_list.begin(), config_list.end(), static_cast<bool(&)(boost::filesystem::path const&)>(boost::filesystem::exists));
		if (it == config_list.end()) {
			std::ostringstream oss;
			oss << "No config schema file found. The following locations were tried:\n";
			std::copy(config_list.begin(), config_list.end(), std::ostream_iterator<std::string>(oss, "\n"));
			oss << "Install the file or define environment variable PERFORMOUS_DEFAULT_CONFIG_FILE\n";
			throw std::runtime_error(oss.str());
		}
		schemafile = *it;
	}
	readConfigXML(schemafile, 0);  // Read schema and defaults
	readConfigXML("/etc/xdg/performous/performous.xml", 1);  // Update defaults with system config
	readConfigXML(userConf, 2);  // Read user settings
}


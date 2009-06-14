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

void assignConfigItem( ConfigItem &_item, std::string _type, xmlpp::Element& _elem ) {
/*
	if( _type == std::string("bool") ) {
		std::string value_string = _elem.get_attribute("value")->get_value();
		bool value = false;
		if( value_string == std::string("true") || value_string == std::string("1") )
			value = true;

		_item.b() = value;
	} else if( _type == std::string("int") ) {
		std::string value_string = _elem.get_attribute("value")->get_value();
		int value = 0;
		if( !value_string.empty() )
			value = boost::lexical_cast<int>(value_string);

		_item.i() = value;
	} else if( _type == std::string("float") ) {
		std::string value_string = _elem.get_attribute("value")->get_value();
		double value = 0;
		if( !value_string.empty() )
			value = boost::lexical_cast<double>(value_string);

		_item.f() = value;
	} else if( _type == std::string("string") ) {
		xmlpp::NodeSet n2 = _elem.find("stringvalue/text()");
		std::string value("");
		for (xmlpp::NodeSet::const_iterator it2 = n2.begin(), end2 = n2.end(); it2 != end2; ++it2) {
			xmlpp::TextNode& elem2 = dynamic_cast<xmlpp::TextNode&>(**it2);
			value = elem2.get_content();
		}
		_item.s() = value;
	} else if( _type == std::string("string_list") ) {
		std::vector<std::string> value;

		xmlpp::NodeSet n2 = _elem.find("stringvalue/text()");
		for (xmlpp::NodeSet::const_iterator it2 = n2.begin(), end2 = n2.end(); it2 != end2; ++it2) {
			xmlpp::TextNode& elem2 = dynamic_cast<xmlpp::TextNode&>(**it2);
			value.push_back(elem2.get_content());
		}
		_item.sl() = value;
	} else {
		std::cout <<  "  Found unknown type " << _type << std::endl;
	}

	{
		// Update short description
		xmlpp::NodeSet n2 = _elem.find("locale/short/text()");
		for (xmlpp::NodeSet::const_iterator it2 = n2.begin(), end2 = n2.end(); it2 != end2; ++it2) {
			xmlpp::TextNode& elem2 = dynamic_cast<xmlpp::TextNode&>(**it2);
			_item.set_short_description(elem2.get_content());
		}
	}
	{
		// Update long description
		xmlpp::NodeSet n2 = _elem.find("locale/long/text()");
		for (xmlpp::NodeSet::const_iterator it2 = n2.begin(), end2 = n2.end(); it2 != end2; ++it2) {
			xmlpp::TextNode& elem2 = dynamic_cast<xmlpp::TextNode&>(**it2);
			_item.set_long_description(elem2.get_content());
		}
	}
	*/
}

void writeConfigfile( const std::string &_configfile )
{
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

void readConfigfile( const std::string &_configfile )
{
/*
	xmlpp::NodeSet n;
	xmlpp::DomParser domParser;

	// looking for schemafile in:
	// $PERFORMOUS_DEFAULT_CONFIG_FILE
	// /usr/share/performous
	// /usr/share/games/performous
	// /usr/local/share/performous
	// /usr/local/share/games/performous
	std::string schemafile("NOT_FOUND");
	std::vector<std::string> config_list;
	char *env_config = getenv("PERFORMOUS_DEFAULT_CONFIG_FILE");
	if( env_config != NULL )
		config_list.push_back(std::string(env_config));
	config_list.push_back(std::string("/usr/share/performous")+std::string("/config/performous.xml"));
	config_list.push_back(std::string("/usr/share/games/performous")+std::string("/config/performous.xml"));
	config_list.push_back(std::string("/usr/local/share/performous")+std::string("/config/performous.xml"));
	config_list.push_back(std::string("/usr/local/share/games/performous")+std::string("/config/performous.xml"));
	for( unsigned int i = 0 ; i < config_list.size() ; ++i ) {
		std::cout << "Testing config file \"" << config_list[i] << "\": ";
		if( boost::filesystem::exists(config_list[i]) ) {
			std::cout << "FOUND" << std::endl;
			schemafile = config_list[i];
			break;
		} else {
			std::cout << "NOT FOUND" << std::endl;
		}
	}

	std::cout << "Opening default configuration file \"" << schemafile << "\"" << std::endl;
	domParser.parse_file(schemafile);
	n = domParser.get_document()->get_root_node()->find("/performous/entry");
	for (xmlpp::NodeSet::const_iterator it = n.begin(), end = n.end(); it != end; ++it) {
		xmlpp::Element& elem = dynamic_cast<xmlpp::Element&>(**it);
		std::string name = elem.get_attribute("name")->get_value();
		std::string type = elem.get_attribute("type")->get_value();
		if( name.empty() || type.empty() ) {
			std::cout << "  name or type attribute is missing or empty" << std::endl;
			continue;
		}

		ConfigItem item(m_type, true);
		assignConfigItem(item, type, elem );
		config[name] = item;

	}
	std::cout << "Found " << config.size() << " default configuration items" << std::endl;

	std::string globalConfigFile("/etc/xdg/performous/performous.xml");
	std::cout << "Opening global configuration file \"" << globalConfigFile << "\"" << std::endl;
	if( !boost::filesystem::exists(globalConfigFile) ) {
		std::cout << "  Cannot open global configuration file" << std::endl;
	} else {
		try {
			domParser.parse_file(globalConfigFile);
			n = domParser.get_document()->get_root_node()->find("/performous/entry");
			for (xmlpp::NodeSet::const_iterator it = n.begin(), end = n.end(); it != end; ++it) {
				xmlpp::Element& elem = dynamic_cast<xmlpp::Element&>(**it);
				std::string name = elem.get_attribute("name")->get_value();
				std::string type = elem.get_attribute("type")->get_value();
				if( name.empty() || type.empty() ) {
					std::cout << "  name or type attribute is missing or empty" << std::endl;
					continue;
				}

				if( config.find(name) == config.end() ) {
					std::cout << "  Cannot find \"" << name << "\" key inside default config file, discarding" << std::endl;
					continue;
				}

				ConfigItem item = config.find(name)->second;
				assignConfigItem(item, type, elem );
				config[name] = item;

			}
		} catch( ... ) {
		}
	}

	std::cout << "Opening user configuration file \"" << _configfile << "\"" << std::endl;
	if( !boost::filesystem::exists(_configfile) ) {
		std::cout << "  Cannot open user configuration file (using defaults)" << std::endl;
		return;
	}
	try {
		domParser.parse_file(_configfile);
		n = domParser.get_document()->get_root_node()->find("/performous/entry");
		for (xmlpp::NodeSet::const_iterator it = n.begin(), end = n.end(); it != end; ++it) {
			xmlpp::Element& elem = dynamic_cast<xmlpp::Element&>(**it);
			std::string name = elem.get_attribute("name")->get_value();
			std::string type = elem.get_attribute("type")->get_value();
			if( name.empty() || type.empty() ) {
				std::cout << "  name or type attribute is missing or empty" << std::endl;
				continue;
			}

			if( config.find(name) == config.end() ) {
				std::cout << "  Cannot find \"" << name << "\" key inside default config file, discarding" << std::endl;
				continue;
			}

			ConfigItem item = config.find(name)->second;
			assignConfigItem(item, type, elem );
			config[name] = item;

		}
	} catch( ... ) {
	}
	*/
}


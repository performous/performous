#include "configuration.hh"
#include "screen.hh"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <algorithm>
#include <libxml++/libxml++.h>


ConfigItem::ConfigItem() {};
ConfigItem::ConfigItem( std::string _type, bool _is_default) : is_default(_is_default), boolean_value(false), integer_value(), double_value(), string_value() {
	type = _type;
	if(type == std::string("int") ) {
		integer_min = std::numeric_limits<int>::min();
		integer_max = std::numeric_limits<int>::max();
		integer_step = 1;
	} else if(type == std::string("float") ) {
		double_min = std::numeric_limits<double>::min();
		double_max = std::numeric_limits<double>::max();
		double_step = 0.01;
	}
}
ConfigItem& ConfigItem::operator++() {
	if( type == std::string("int") ) {
		integer_value += integer_step;
		integer_value = (integer_value / integer_step)*integer_step;
	} else if( type == std::string("float") ) {
		double_value = double_value + double_step;
	}
	return *this;
}
ConfigItem& ConfigItem::operator--() {
	if( type == std::string("int") ) {
		integer_value -= integer_step;
		integer_value = (integer_value / integer_step)*integer_step;
	} else if( type == std::string("float") ) {
		double_value = double_value - double_step;
	}
	return *this;
}
ConfigItem& ConfigItem::operator+=(const int& right) {
	if( type == std::string("int") ) {
		integer_value += right;
	} else if( type == std::string("float") ) {
		double_value += right;
	}
	return *this;
}
ConfigItem& ConfigItem::operator-=(const int& right) {
	return *this+=(-right);
}
ConfigItem& ConfigItem::operator+=(const float& right) {
	if( type == std::string("float") ) {
		double_value += right;
	} else if( type == std::string("int") ) {
		integer_value += right;
	}
	return *this;
}
ConfigItem& ConfigItem::operator-=(const float& right) {
	return *this+=(-right);
}
ConfigItem& ConfigItem::operator+=(const double& right) {
	if( type == std::string("float") ) {
		double_value += right;
	} else if( type == std::string("int") ) {
		integer_value += right;
	}
	return *this;
}
ConfigItem& ConfigItem::operator-=(const double& right) {
	return *this+=(-right);
}
void ConfigItem::set_short_description( std::string _short_desc ) {
	short_desc = _short_desc;
}
void ConfigItem::set_long_description( std::string _long_desc ) {
	long_desc = _long_desc;
}
int &ConfigItem::i(void) {
	return integer_value;
}
bool &ConfigItem::b(void) {
	return boolean_value;
}
double &ConfigItem::f(void) {
	return double_value;
}
std::string &ConfigItem::s(void) {
	return string_value;
}
std::vector<std::string> &ConfigItem::sl(void) {
	return string_list_value;
}
std::ostream& operator <<(std::ostream &os,const ConfigItem &obj) {
	os << "  Type: " << obj.type << std::endl;
	if( obj.type == std::string("string") ) {
		os << "  Value: \"" << obj.string_value << "\"" << std::endl;
	} else if(obj.type == std::string("int") ) {
		os << "  Value: " << obj.integer_value << std::endl;
	} else if(obj.type == std::string("float") ) {
		os << "  Value: " << obj.double_value << std::endl;
	} else if(obj.type == std::string("bool") ) {
		os << "  Value: " << (obj.boolean_value?"true":"false") << std::endl;
	} else if(obj.type == std::string("string_list") ) {
		os << "  Value: ";
		for( unsigned int i = 0 ; i < obj.string_list_value.size() ; i++ ) {
			os << "\"" << obj.string_list_value[i] << "\"";
			if( i != obj.string_list_value.size() -1 )
				os << ", ";
		}
		os << std::endl;
	}
	return os;
}

std::map<std::string, ConfigItem> config; // "name" is the key

void assignConfigItem( ConfigItem &_item, std::string _type, xmlpp::Element& _elem ) {
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
}

void readConfigfile( const std::string &_configfile )
{
	xmlpp::NodeSet n;
	xmlpp::DomParser domParser;

	// looking for schemafile in:
	// /usr/share/performous
	// /usr/share/games/performous
	// /usr/local/share/performous
	// /usr/local/share/games/performous
	// $PERFORMOUS_DEFAULT_CONFIG_FILE
	std::string schemafile("NOT_FOUND");
	std::vector<std::string> config_list;
	config_list.push_back(std::string("/usr/share/performous")+std::string("/config/performous.xml"));
	config_list.push_back(std::string("/usr/share/games/performous")+std::string("/config/performous.xml"));
	config_list.push_back(std::string("/usr/local/share/performous")+std::string("/config/performous.xml"));
	config_list.push_back(std::string("/usr/local/share/games/performous")+std::string("/config/performous.xml"));
	char *env_config = getenv("PERFORMOUS_DEFAULT_CONFIG_FILE");
	if( env_config != NULL )
		config_list.push_back(std::string(env_config));
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

	std::cout << "Openning default configuration file \"" << schemafile << "\"" << std::endl;
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

		ConfigItem item(type, true);
		assignConfigItem(item, type, elem );
		config[name] = item;

	}
	std::cout << "Found " << config.size() << " default configuration items" << std::endl;

	std::string globalConfigFile("/etc/xdg/performous/performous.xml");
	std::cout << "Openning global configuration file \"" << globalConfigFile << "\"" << std::endl;
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
		
				ConfigItem item(type, true);
				assignConfigItem(item, type, elem );
				config[name] = item;
		
			}
		} catch( ... ) {
		}
	}

	std::cout << "Openning user configuration file \"" << _configfile << "\"" << std::endl;
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
	
			ConfigItem item(type, false);
			assignConfigItem(item, type, elem );
			config[name] = item;
	
		}
	} catch( ... ) {
	}
}

CConfigurationAudioVolume::CConfigurationAudioVolume(std::string const& title, Audio& audio, GetFunc get, SetFunc set):
  CConfiguration(title), m_audio(audio), m_get(get), m_set(set)
{}
void CConfigurationAudioVolume::setNext() { (m_audio.*m_set)(std::min(100u, (m_audio.*m_get)() + 1)); }
void CConfigurationAudioVolume::setPrevious() { (m_audio.*m_set)(std::max(0, int((m_audio.*m_get)()) - 1)); }

std::string CConfigurationAudioVolume::getValue() const {
	(m_audio.*m_set)((m_audio.*m_get)());  // Hack to have the volume set when the control is entered
	return boost::lexical_cast<std::string>((m_audio.*m_get)());
}


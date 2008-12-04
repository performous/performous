#include "configuration.hh"
#include "screen.hh"
#include <boost/lexical_cast.hpp>
#include <algorithm>
#include <libxml++/libxml++.h>


std::map<std::string, boost::any> config; // "name" is the key
std::map<std::string, boost::any> config_desc; // "schema" on the config is the key

void readConfigfile( const std::string &_configfile, const std::string &_schemafile)
{
	xmlpp::NodeSet n;
	std::cout << "Openning configuration file \"" << _schemafile << "\"" << std::endl;

	xmlpp::DomParser domSchema(_schemafile);
	n = domSchema.get_document()->get_root_node()->find("/gconfschemafile/schemalist/schema");
	for (xmlpp::NodeSet::const_iterator it = n.begin(), end = n.end(); it != end; ++it) {
		xmlpp::NodeSet key_nodeset = dynamic_cast<xmlpp::Element&>(**it).find("key/text()");
		if( key_nodeset.size() == 0 ) continue;
		std::string key = dynamic_cast<xmlpp::TextNode&>(**key_nodeset.begin()).get_content();
		std::cout << "Key: \"" << key << "\"" << std::endl;

		xmlpp::NodeSet type_nodeset = dynamic_cast<xmlpp::Element&>(**it).find("type/text()");
		if( type_nodeset.size() == 0 ) continue;
		std::string type = dynamic_cast<xmlpp::TextNode&>(**type_nodeset.begin()).get_content();
		std::cout << "  Type: \"" << type << "\"" << std::endl;

		if( type == std::string("list") ) {
			xmlpp::NodeSet ltype_nodeset = dynamic_cast<xmlpp::Element&>(**it).find("list_type/text()");
			if( ltype_nodeset.size() == 0 ) continue;
			std::string ltype = dynamic_cast<xmlpp::TextNode&>(**ltype_nodeset.begin()).get_content();
			std::cout << "  List type: \"" << ltype << "\"" << std::endl;
		}

		xmlpp::NodeSet default_nodeset = dynamic_cast<xmlpp::Element&>(**it).find("default/text()");
		if( default_nodeset.size() != 0 ) {
			std::string default_value = dynamic_cast<xmlpp::TextNode&>(**default_nodeset.begin()).get_content();
			std::cout << "  Default: \"" << default_value << "\"" << std::endl;
		}

		xmlpp::NodeSet short_desc_nodeset = dynamic_cast<xmlpp::Element&>(**it).find("locale[@name='C']/short/text()");
		if( short_desc_nodeset.size() != 0 ) {
			std::string short_desc = dynamic_cast<xmlpp::TextNode&>(**short_desc_nodeset.begin()).get_content();
			std::cout << "  Short description: \"" << short_desc << "\"" << std::endl;
		}

		xmlpp::NodeSet long_desc_nodeset = dynamic_cast<xmlpp::Element&>(**it).find("locale[@name='C']/long/text()");
		if( long_desc_nodeset.size() != 0 ) {
			std::string long_desc = dynamic_cast<xmlpp::TextNode&>(**long_desc_nodeset.begin()).get_content();
			std::cout << "  Long description: \"" << long_desc << "\"" << std::endl;
		}
	}






	std::cout << "Openning configuration file \"" << _configfile << "\"" << std::endl;
	xmlpp::DomParser domConfig(_configfile);
	n = domConfig.get_document()->get_root_node()->find("/gconf/entry");
	for (xmlpp::NodeSet::const_iterator it = n.begin(), end = n.end(); it != end; ++it) {
		xmlpp::Element& elem = dynamic_cast<xmlpp::Element&>(**it);
		std::string name = elem.get_attribute("name")->get_value();
		std::string schema = elem.get_attribute("schema")->get_value();
		std::string type = elem.get_attribute("type")->get_value();
		if( name.empty() || schema.empty() || type.empty() ) continue;

		if( type == std::string("bool") ) {
			std::string value_string = elem.get_attribute("value")->get_value();
			bool value = false;
			if( value_string == std::string("true") || value_string == std::string("1") )
				value = true;

			std::cout <<  "  Found \"" << name << "\" of type \"" << type << "\" of value " << value << std::endl;
		} else if( type == std::string("int") ) {
			std::string value_string = elem.get_attribute("value")->get_value();
			int value = 0;
			if( !value_string.empty() )
				value = boost::lexical_cast<int>(value_string);

			std::cout <<  "  Found \"" << name << "\" of type \"" << type << "\" of value " << value << std::endl;
		} else if( type == std::string("float") ) {
			std::string value_string = elem.get_attribute("value")->get_value();
			double value = 0;
			if( !value_string.empty() )
				value = boost::lexical_cast<double>(value_string);

			std::cout <<  "  Found \"" <<  name << "\" of type \"" << type << "\" of value " << value << std::endl;
		} else if( type == std::string("string") ) {
			xmlpp::NodeSet n2 = elem.find("stringvalue/text()");
			std::string value("");
			for (xmlpp::NodeSet::const_iterator it2 = n2.begin(), end2 = n2.end(); it2 != end2; ++it2) {
				xmlpp::TextNode& elem2 = dynamic_cast<xmlpp::TextNode&>(**it2);
				value = elem2.get_content();
			}

			std::cout <<  "  Found \"" << name << "\" of type " << type << " of value \"" << value << "\"" << std::endl;
		} else if( type == std::string("list") ) {
			std::string list_type = elem.get_attribute("ltype")->get_value();
			if( list_type.empty() ) continue;

			std::cout <<  "  Found \"" << name << "\" of type \"" << type << " of " << list_type << "\"" << std::endl;

			if( list_type == std::string("string") ) {
				xmlpp::NodeSet n2 = elem.find("li/stringvalue/text()");
				for (xmlpp::NodeSet::const_iterator it2 = n2.begin(), end2 = n2.end(); it2 != end2; ++it2) {
					xmlpp::TextNode& elem2 = dynamic_cast<xmlpp::TextNode&>(**it2);
					std::string value("");
					value = elem2.get_content();
					std::cout <<  "    String: \"" << value << "\"" << std::endl;
				}
			} else {
				std::cout <<  "  Found \"" << name << "\" list of unknown type " << list_type << std::endl;
			}
		} else {
			std::cout <<  "  Found \"" << name << "\" of unknown type " << type << std::endl;
		}
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


#include "configuration.hh"
#include "screen.hh"
#include <boost/lexical_cast.hpp>
#include <algorithm>
#include <libxml++/libxml++.h>


std::map<std::string, boost::any> config;

void readConfigfile( const std::string &_configfile, const std::string &_schemafile)
{
	std::cout << "Openning configuration file \"" << _configfile << "\"" << std::endl;
	xmlpp::DomParser dom(_configfile);
	xmlpp::NodeSet n = dom.get_document()->get_root_node()->find("/gconf/entry");
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


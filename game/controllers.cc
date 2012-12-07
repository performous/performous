#include "controllers.hh"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include "SDL_joystick.h"

#include <libxml++/libxml++.h>
#include "fs.hh"

#include <stdexcept>

using namespace input;

typedef std::map<unsigned, unsigned> ButtonMapping;

/// A controller definition from controllers.xml
struct ControllerDef {
	std::string name;
	std::string description;
	SourceType sourceType;
	DevType devType;
	boost::regex deviceRegex;
	std::pair<unsigned, unsigned> deviceMinMax;
	std::pair<unsigned, unsigned> channelMinMax;
	ButtonMapping mapping;
};

namespace {
	struct XMLError {
		XMLError(xmlpp::Element const& e, std::string const& msg): elem(e), message(msg) {}
		xmlpp::Element const& elem;
		std::string message;
	};
	std::string getAttribute(xmlpp::Element const& elem, std::string const& attr) {
		xmlpp::Attribute const* a = elem.get_attribute(attr);
		if (!a) throw XMLError(elem, "attribute " + attr + " not found");
		return a->get_value();
	}
	template <typename T> bool tryGetAttribute(xmlpp::Element const& elem, std::string const& attr, T& var) {
		xmlpp::Attribute const* a = elem.get_attribute(attr);
		if (!a) return false;
		try {
			var = boost::lexical_cast<T>(a->get_value());
		} catch (std::exception&) {
			throw XMLError(elem, "attribute " + attr + " value invalid: " + a->get_value());
		}
		return true;
	}
	template <typename Numeric> void parseMinMax(xmlpp::Element const& elem, std::pair<Numeric, Numeric>& range) {
		xmlpp::Attribute const* a;
		a = elem.get_attribute("min");
		if (a) range.first = boost::lexical_cast<Numeric>(a->get_value());
		a = elem.get_attribute("max");
		if (a) range.second = boost::lexical_cast<Numeric>(a->get_value());
	}
}




struct Controllers::Impl {
	typedef std::map<std::string, ControllerDef> ControllerDefs;
	ControllerDefs m_controllerDefs;

	typedef std::map<std::string, unsigned> NameToButton;
	NameToButton m_buttons[DEVTYPE_N];

	Impl() {
		#define DEFINE_BUTTON(devtype, name, num) m_buttons[DEVTYPE_##devtype][#name] = devtype##_##name;
		#include "controllers-buttons.ii"
		readControllers(getDefaultConfig(fs::path("/config/controllers.xml")));
		readControllers(getConfigDir() / "controllers.xml");
	}
	
	void readControllers(fs::path const& file) {
		if (!fs::exists(file)) {
			std::clog << "controllers/info: Skipping " << file << " (not found)" << std::endl;
			return;
		}
		std::clog << "controllers/info: Parsing " << file << std::endl;
		xmlpp::DomParser domParser(file.string());
		try {
			parseControllers(domParser, "/controllers/joystick/controller", SOURCETYPE_JOYSTICK);
			parseControllers(domParser, "/controllers/midi/controller", SOURCETYPE_MIDI);
		} catch (XMLError& e) {
			int line = e.elem.get_line();
			std::string name = e.elem.get_name();
			throw std::runtime_error(file.string() + ":" + boost::lexical_cast<std::string>(line) + " element " + name + " " + e.message);
		}
	}

	void parseControllers(xmlpp::DomParser const& dom, std::string const& path, SourceType sourceType) {
		xmlpp::NodeSet n = dom.get_document()->get_root_node()->find(path);
		for (xmlpp::NodeSet::const_iterator nodeit = n.begin(), end = n.end(); nodeit != end; ++nodeit) {
			xmlpp::Element& elem = dynamic_cast<xmlpp::Element&>(**nodeit);
			ControllerDef def;
			def.name = getAttribute(elem, "name");
			def.sourceType = sourceType;
			// Device type
			{
				std::string type = getAttribute(elem, "type");
				if (type == "guitar") def.devType = DEVTYPE_GUITAR;
				else if (type == "drums") def.devType = DEVTYPE_DRUMS;
				else if (type == "keytar") def.devType = DEVTYPE_KEYTAR;
				else if (type == "piano") def.devType = DEVTYPE_PIANO;
				else if (type == "dancepad") def.devType = DEVTYPE_DANCEPAD;
				else {
					std::clog << "controllers/warn: " << type << ": Unknown controller type in controllers.xml (skipped)" << std::endl;
					continue;
				}
			}
			xmlpp::NodeSet ns;
			// Device description
			ns = elem.find("description/text()");
			if (ns.size() == 1) def.description = dynamic_cast<xmlpp::TextNode&>(*ns[0]).get_content();

			// Read filtering rules
			ns = elem.find("device");
			if (ns.size() == 1) {
				xmlpp::Element& elem = dynamic_cast<xmlpp::Element&>(*ns[0]);
				def.deviceRegex = getAttribute(elem, "regex");
				parseMinMax(elem, def.deviceMinMax);
			}
			ns = elem.find("channel");
			parseMinMax(dynamic_cast<xmlpp::Element&>(*ns[0]), def.channelMinMax);

			// Read button mapping
			ns = elem.find("mapping/button");
			for (xmlpp::NodeSet::const_iterator nodeit2 = ns.begin(), end = ns.end(); nodeit2 != end; ++nodeit2) {
				xmlpp::Element& button_elem = dynamic_cast<xmlpp::Element&>(**nodeit2);
				unsigned hw = boost::lexical_cast<unsigned>(getAttribute(button_elem, "hw"));
				std::string map = boost::algorithm::to_upper_copy(getAttribute(button_elem, "map"));
				NameToButton const& n2b = m_buttons[def.devType];
				NameToButton::const_iterator mapit = n2b.find(map);
				if (mapit == n2b.end()) throw XMLError(button_elem, map + ": Invalid value for map attribute");
				def.mapping[hw] = mapit->second;
			}

			// inserting the instrument
			std::clog << "controllers/info: " << (m_controllerDefs.find(def.name) == m_controllerDefs.end() ? "Controller" : "Overriding")
			  << " definition: " << def.name << ": " << def.description << std::endl;
			m_controllerDefs[def.name] = def;
		}
	}
};

Controllers::Controllers(): self(new Controllers::Impl()) {}
Controllers::~Controllers() {}
void Controllers::process(boost::xtime const& now) {}
bool Controllers::getNav(NavEvent& ev) { return false; }
bool Controllers::pressed(SourceId const&, unsigned button) { return false; }

bool Controllers::pushEvent(SDL_Event _e, boost::xtime const& t) {
/*	Event event;
	// Add event time
	event.time = t;
	// Translate to NavButton
	event.nav = getNav(_e);
	switch(_e.type) {
		case SDL_KEYDOWN: return keybutton(event, _e, true);
		case SDL_KEYUP: return keybutton(event, _e, false);
		default: return false;
	}*/
	return true;
}


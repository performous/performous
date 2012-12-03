#include "controllers.hh"

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

#include "SDL_joystick.h"

#include <libxml++/libxml++.h>
#include "fs.hh"

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
	template <typename Numeric> void parseMinMax(xmlpp::Element const& elem, std::pair<Numeric, Numeric>& range) {
		range.first = boost::lexical_cast<Numeric>(getAttribute(elem, "min"));
		range.second = boost::lexical_cast<Numeric>(getAttribute(elem, "max"));
	}
}




struct Controllers::Impl {
	typedef std::map<std::string, ControllerDef> ControllerDefs;
	ControllerDefs m_controllerDefs;

	Impl() {
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
				std::string map = getAttribute(button_elem, "map");
				def.mapping[hw] = boost::lexical_cast<unsigned>(map);
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
bool Controllers::pushEvent(SDL_Event event, boost::xtime const& now) { return false; }
bool Controllers::getNav(NavEvent& ev) { return false; }
bool Controllers::pressed(SourceId const&, unsigned button) { return false; }


#if 0 // FIXME
void init() {
	std::map<unsigned int, Instrument> forced_type;
	ConfigItem::StringList instruments = config["game/instruments"].sl();

	// Populate controller forcing config items
	ConfigItem& ci0 = config["game/instrument0"];
	ConfigItem& ci1 = config["game/instrument1"];
	ConfigItem& ci2 = config["game/instrument2"];
	ConfigItem& ci3 = config["game/instrument3"];
	int i = 0;
	for (ControllerDefs::const_iterator it = m_controllerDefs.begin(); it != m_controllerDefs.end(); ++it, ++i) {
		// Add the enum
		std::string title = it->second.description;
		ci0.addEnum(title);
		ci1.addEnum(title);
		ci2.addEnum(title);
		ci3.addEnum(title);
		// Check for active items
		if (i == ci0.i()-1) instruments.push_back("0:" + it->first);
		if (i == ci1.i()-1) instruments.push_back("1:" + it->first);
		if (i == ci2.i()-1) instruments.push_back("2:" + it->first);
		if (i == ci3.i()-1) instruments.push_back("3:" + it->first);
	}

	// Check all forced instruments
	for (ConfigItem::StringList::const_iterator it = instruments.begin(); it != instruments.end(); ++it) {
		std::istringstream iss(*it);
		unsigned sdl_id;
		char ch;
		std::string type;
		if (!(iss >> sdl_id >> ch >> type) || ch != ':') {
			std::clog << "controllers/error: \"" << *it << "\" invalid syntax, should be SDL_ID:CONTROLLER_TYPE" << std::endl;
			continue;
		} else {
			bool found = false;
			for (Instruments::const_iterator it2 = g_instruments.begin(); it2 != g_instruments.end(); ++it2) {
				if (type == it2->first) {
					forced_type.insert(std::pair<unsigned int,Instrument>(sdl_id, Instrument(it2->second)));
					found = true;
					break;
				}
			}
			if (!found) std::clog << "controllers/error: Controller type \"" << type << "\" unknown" << std::endl;
		}
	}

	unsigned int nbjoysticks = SDL_NumJoysticks();
	for (unsigned int i = 0 ; i < nbjoysticks ; ++i) {
		std::string name = SDL_JoystickName(i);
		std::cout << "SDL joystick: " << name << std::endl;
		SDL_Joystick* joy = SDL_JoystickOpen(i);
		SDL::sdl_devices[i] = joy;
		std::cout << "  Id: " << i;
		std::cout << ",  Axes: " << SDL_JoystickNumAxes(joy);
		std::cout << ", Balls: " << SDL_JoystickNumBalls(joy);
		std::cout << ", Buttons: " << SDL_JoystickNumButtons(joy);
		std::cout << ", Hats: " << SDL_JoystickNumHats(joy) << std::endl;
		if (forced_type.find(i) != forced_type.end() ) {
			std::cout << "  Detected as: " << forced_type.find(i)->second.description << " (forced)" << std::endl;
			detail::devices.insert(std::make_pair(i, detail::InputDevPrivate(forced_type.find(i)->second)));
		} else {
			bool found = false;
			for (ControllerDefs::const_iterator it = m_controllerDefs.begin(); it != m_controllerDefs.end(); ++it) {
				ControllerDef const& def = it->second;
				if (regex_search(name.c_str(), def.deviceRegex)) {
					std::cout << "  Detected as: " << def.description << std::endl;
					detail::devices.insert(std::make_pair(i, detail::InputDevPrivate(def)));
					found = true;
					break;
				}
			}
			if(!found) {
				std::cout << "  Detected as: Unknown (please report the name; use config to force detection)" << std::endl;
				SDL_JoystickClose(joy);
				continue;
			}
		}
	}
}

bool Controllers::pushEvent(SDL_Event _e, boost::xtime t) {
	Event event;
	// Add event time
	event.time = t;
	// Translate to NavButton
	event.nav = getNav(_e);
	switch(_e.type) {
		case SDL_KEYDOWN: return keybutton(event, _e, true);
		case SDL_KEYUP: return keybutton(event, _e, false);
		default: return false;
	}
	return true;
}
#endif

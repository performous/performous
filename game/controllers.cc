#include "controllers.hh"

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

input::Instruments g_instruments;

#include "SDL_joystick.h"

#include <libxml++/libxml++.h>
#include "fs.hh"


using namespace input;

Controllers::Controllers() {}
Controllers::~Controllers() {}
void Controllers::process() {}

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

namespace input::detail {
	class InputDevPrivate {
	  public:
		InputDevPrivate(const Instrument& _instrument) : m_assigned(false), m_instrument(_instrument) {
			for(unsigned int i = 0 ; i < BUTTONS ; i++) {
				m_pressed[i] = false;
			}
		};
		bool tryPoll(Event& _event) {
			if (m_events.empty()) return false;
			_event = m_events.front();
			m_events.pop_front();
			return true;
		};
		void addEvent(Event _event) {
			// only add event if the device is assigned
			if (m_assigned) m_events.push_back(_event);
			// always keep track of button status
			for( unsigned int i = 0 ; i < BUTTONS ; ++i) {
				m_pressed[i] = _event.pressed[i];
			}
		};
		void clearEvents() { m_events.clear(); }
		void assign() { m_assigned = true; }
		void unassign() { m_assigned = false; clearEvents(); }
		bool assigned() const { return m_assigned; }
		bool pressed(int _button) const { return m_pressed[_button]; }
		std::string name() const { return m_instrument.name; }
	  private:
		std::deque<Event> m_events;
		bool m_assigned;
		bool m_pressed[BUTTONS];
		Instrument m_instrument;
	};

	typedef std::map<unsigned int,InputDevPrivate> InputDevs;
	extern InputDevs devices;
}


void readControllers(input::Instruments &instruments, fs::path const& file) {
	if (!fs::exists(file)) {
		std::clog << "controllers/info: Skipping " << file << " (not found)" << std::endl;
		return;
	}
	std::clog << "controllers/info: Parsing " << file << std::endl;
	xmlpp::DomParser domParser(file.string());
	try {
		xmlpp::NodeSet n = domParser.get_document()->get_root_node()->find("/controllers/controller");
		for (xmlpp::NodeSet::const_iterator nodeit = n.begin(), end = n.end(); nodeit != end; ++nodeit) {
			xmlpp::Element& elem = dynamic_cast<xmlpp::Element&>(**nodeit);
			std::string name = getAttribute(elem, "name");
			std::string type = getAttribute(elem, "type");

			xmlpp::NodeSet ns;
			// searching description
			ns = elem.find("description/text()");
			if(ns.size()==0) continue;
			std::string description = dynamic_cast<xmlpp::TextNode&>(*ns[0]).get_content();

			// searching the match
			ns = elem.find("regexp");
			if(ns.size()==0) continue;
			std::string regexp = getAttribute(dynamic_cast<xmlpp::Element&>(*ns[0]), "match");

			// setting the type
			input::DevType devType;
			if(type == "guitar") {
				devType = input::GUITAR;
			} else if(type == "drumkit") {
				devType = input::DRUMS;
			} else if(type == "keyboard") {
				devType = input::KEYBOARD;
			} else if(type == "dancepad") {
				devType = input::DANCEPAD;
			} else {
				continue;
			}

			// setting the mapping
			std::vector<int> mapping(16, -1);
			ns = elem.find("mapping/button");
			static const int SDL_BUTTONS = 16;
			switch(devType) {
				case input::GUITAR:
					for (xmlpp::NodeSet::const_iterator nodeit2 = ns.begin(), end = ns.end(); nodeit2 != end; ++nodeit2) {
						xmlpp::Element& button_elem = dynamic_cast<xmlpp::Element&>(**nodeit2);
						int id = boost::lexical_cast<int>(getAttribute(button_elem, "id"));
						std::string value = getAttribute(button_elem, "value");
						if(id >= SDL_BUTTONS) break;
						if(value == "green") mapping[id] = input::GREEN_FRET_BUTTON;
						else if(value == "red") mapping[id] = input::RED_FRET_BUTTON;
						else if(value == "yellow") mapping[id] = input::YELLOW_FRET_BUTTON;
						else if(value == "blue") mapping[id] = input::BLUE_FRET_BUTTON;
						else if(value == "orange") mapping[id] = input::ORANGE_FRET_BUTTON;
						else if(value == "godmode") mapping[id] = input::GODMODE_BUTTON;
						else if(value == "select") mapping[id] = input::SELECT_BUTTON;
						else if(value == "start") mapping[id] = input::START_BUTTON;
						else continue;
					}
					break;
				case input::DRUMS:
					for (xmlpp::NodeSet::const_iterator nodeit2 = ns.begin(), end = ns.end(); nodeit2 != end; ++nodeit2) {
						xmlpp::Element& button_elem = dynamic_cast<xmlpp::Element&>(**nodeit2);
						int id = boost::lexical_cast<int>(getAttribute(button_elem, "id"));
						std::string value = getAttribute(button_elem, "value");
						if(id >= SDL_BUTTONS) break;
						if(value == "kick") mapping[id] = input::KICK_BUTTON;
						else if(value == "red") mapping[id] = input::RED_TOM_BUTTON;
						else if(value == "yellow") mapping[id] = input::YELLOW_TOM_BUTTON;
						else if(value == "blue") mapping[id] = input::BLUE_TOM_BUTTON;
						else if(value == "green") mapping[id] = input::GREEN_TOM_BUTTON;
						else if(value == "orange") mapping[id] = input::ORANGE_TOM_BUTTON;
						else if(value == "select") mapping[id] = input::SELECT_BUTTON;
						else if(value == "start") mapping[id] = input::START_BUTTON;
						else continue;
					}
					break;
				case input::KEYBOARD:
					for (xmlpp::NodeSet::const_iterator nodeit2 = ns.begin(), end = ns.end(); nodeit2 != end; ++nodeit2) {
						xmlpp::Element& button_elem = dynamic_cast<xmlpp::Element&>(**nodeit2);
						int id = boost::lexical_cast<int>(getAttribute(button_elem, "id"));
						std::string value = getAttribute(button_elem, "value");
						if(id >= SDL_BUTTONS) break;
						if(value == "C") mapping[id] = input::C_BUTTON;
						else if(value == "D") mapping[id] = input::D_BUTTON;
						else if(value == "E") mapping[id] = input::E_BUTTON;
						else if(value == "F") mapping[id] = input::F_BUTTON;
						else if(value == "G") mapping[id] = input::G_BUTTON;
						else if(value == "godmode") mapping[id] = input::GODMODE_BUTTON;
						else continue;
					}
					break;
				case input::DANCEPAD:
					for (xmlpp::NodeSet::const_iterator nodeit2 = ns.begin(), end = ns.end(); nodeit2 != end; ++nodeit2) {
						xmlpp::Element& button_elem = dynamic_cast<xmlpp::Element&>(**nodeit2);
						int id = boost::lexical_cast<int>(getAttribute(button_elem, "id"));
						std::string value = getAttribute(button_elem, "value");
						if(id >= SDL_BUTTONS) break;
						if(value == "left") mapping[id] = input::LEFT_DANCE_BUTTON;
						else if(value == "down") mapping[id] = input::DOWN_DANCE_BUTTON;
						else if(value == "up") mapping[id] = input::UP_DANCE_BUTTON;
						else if(value == "right") mapping[id] = input::RIGHT_DANCE_BUTTON;
						else if(value == "down-left") mapping[id] = input::DOWN_LEFT_DANCE_BUTTON;
						else if(value == "down-right") mapping[id] = input::DOWN_RIGHT_DANCE_BUTTON;
						else if(value == "up-left") mapping[id] = input::UP_LEFT_DANCE_BUTTON;
						else if(value == "up-right") mapping[id] = input::UP_RIGHT_DANCE_BUTTON;
						else if(value == "start") mapping[id] = input::START_BUTTON;
						else if(value == "select") mapping[id] = input::SELECT_BUTTON;
						else continue;
					}
					break;
			}

			// inserting the instrument
			if(instruments.find(name) == instruments.end()) {
				std::clog << "controllers/info:    Adding " << type << ": " << name << std::endl;
			} else {
				std::clog << "controllers/info:    Overriding " << type << ": " << name << std::endl;
				instruments.erase(name);
			}
			input::Instrument instrument(name, devType, mapping);
			instrument.match = regexp;
			instrument.description = description;
			instruments.insert(std::make_pair(name, instrument));
		}
	} catch (XMLError& e) {
		int line = e.elem.get_line();
		std::string name = e.elem.get_name();
		throw std::runtime_error(file.string() + ":" + boost::lexical_cast<std::string>(line) + " element " + name + " " + e.message);
	}
}

void input::SDL::init() {
	readControllers(g_instruments, getDefaultConfig(fs::path("/config/controllers.xml")));
	readControllers(g_instruments, getConfigDir() / "controllers.xml");
	std::map<unsigned int, input::Instrument> forced_type;
	ConfigItem::StringList instruments = config["game/instruments"].sl();

	// Populate controller forcing config items
	ConfigItem& ci0 = config["game/instrument0"];
	ConfigItem& ci1 = config["game/instrument1"];
	ConfigItem& ci2 = config["game/instrument2"];
	ConfigItem& ci3 = config["game/instrument3"];
	int i = 0;
	for (input::Instruments::const_iterator it = g_instruments.begin(); it != g_instruments.end(); ++it, ++i) {
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
			for (input::Instruments::const_iterator it2 = g_instruments.begin(); it2 != g_instruments.end(); ++it2) {
				if (type == it2->first) {
					forced_type.insert(std::pair<unsigned int,input::Instrument>(sdl_id, input::Instrument(it2->second)));
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
		if (SDL_JoystickNumButtons(joy) == 0) {
			std::cout << "  Not suitable for Performous" << std::endl;
			SDL_JoystickClose(joy);
			continue;
		}
		input::SDL::sdl_devices[i] = joy;
		std::cout << "  Id: " << i;
		std::cout << ",  Axes: " << SDL_JoystickNumAxes(joy);
		std::cout << ", Balls: " << SDL_JoystickNumBalls(joy);
		std::cout << ", Buttons: " << SDL_JoystickNumButtons(joy);
		std::cout << ", Hats: " << SDL_JoystickNumHats(joy) << std::endl;
		if( forced_type.find(i) != forced_type.end() ) {
			std::cout << "  Detected as: " << forced_type.find(i)->second.description << " (forced)" << std::endl;
			input::detail::devices.insert(std::make_pair(i, input::detail::InputDevPrivate(forced_type.find(i)->second)));
		} else {
			bool found = false;
			for(input::Instruments::const_iterator it = g_instruments.begin() ; it != g_instruments.end() ; ++it) {
				boost::regex sdl_name(it->second.match);
				if (regex_search(name.c_str(), sdl_name)) {
					std::cout << "  Detected as: " << it->second.description << std::endl;
					input::detail::devices.insert(std::make_pair(i, input::detail::InputDevPrivate(it->second)));
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

bool input::SDL::pushEvent(SDL_Event _e, boost::xtime t) {
	using namespace input::detail;

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


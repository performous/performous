#include "controllers.hh"

#include <boost/smart_ptr/shared_ptr.hpp>
#include <vector>

namespace input {
	struct Joysticks: public Hardware {
		Joysticks() {
			for (int id = 0; id < SDL_NumJoysticks(); ++id) {
				m_joysticks.push_back(JoyPtr(SDL_JoystickOpen(id), SDL_JoystickClose));
			}
		}
		std::string getName(unsigned device) const {
			return SDL_JoystickName(device);
		}
		bool process(Event& event, SDL_Event const& sdlEv) {
			if (sdlEv.type == SDL_JOYBUTTONDOWN || sdlEv.type == SDL_JOYBUTTONUP) {
				event.hw = sdlEv.jbutton.button;
				event.value = (sdlEv.type == SDL_JOYBUTTONDOWN ? 1.0 : 0.0);
			}
			else if (sdlEv.type == SDL_JOYAXISMOTION) {
				event.hw = hwIsAxis.min + sdlEv.jaxis.axis;
				event.value = (sdlEv.jaxis.value + 0.5) / 32767.5;  // Convert to -1.0 .. 1.0 range
				if (std::abs(event.value) < 0.001) event.value = 0.0;  // Some dead zone around zero
			}
			else if (sdlEv.type == SDL_JOYHATMOTION) {
				event.hw = hwIsHat.min + sdlEv.jhat.hat;
				event.value = sdlEv.jhat.value;
			}
			else return false;
			event.source = SourceId(SOURCETYPE_JOYSTICK, sdlEv.jbutton.which);  // All j* structures have .which at the same position as jbutton
			return true;
		}
		typedef boost::shared_ptr<SDL_Joystick> JoyPtr;
		std::vector<JoyPtr> m_joysticks;

	};

	Hardware::ptr constructJoysticks() { return Hardware::ptr(new Joysticks()); }
}


#if 0

// Forcing menu

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

#endif


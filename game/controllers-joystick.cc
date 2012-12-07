#include "controllers.hh"

#include <boost/smart_ptr/shared_ptr.hpp>
#include <vector>

namespace input {
	struct Joysticks: public Hardware {
		Joysticks() {
			for (unsigned id = 0; id < SDL_NumJoysticks(); ++id) {
				m_joysticks.push_back(JoyPtr(SDL_JoystickOpen(id), SDL_JoystickClose));
			}
		}
		void process(boost::xtime const& now) {
			// Init button state
			for (size_t idx = 0; idx < m_joysticks.size(); ++idx) {
				SDL_Joystick* joy = m_joysticks[idx].get();
				unsigned num_buttons = SDL_JoystickNumButtons(joy);
				for( unsigned i = 0; i < num_buttons; ++i) {
					int state = SDL_JoystickGetButton(joy, i);
					Event event;
					event.source = SourceId(SOURCETYPE_JOYSTICK, idx);
					event.hw = i;
					event.value = state;
					event.time = now;
				}
			}
		}
		typedef boost::shared_ptr<SDL_Joystick> JoyPtr;
		std::vector<JoyPtr> m_joysticks;

	};

	Hardware::ptr constructJoysticks() { return Hardware::ptr(new Joysticks()); }
}


#if 0


// NavEvent handling

	} else if (e.type == SDL_JOYBUTTONDOWN) {
		// Joystick buttons
		unsigned int joy_id = e.jbutton.which;
		input::detail::InputDevPrivate devt = input::detail::devices.find(joy_id)->second;
		int b = devt.buttonFromSDL(e.jbutton.button);
		if (b == -1) return input::NONE;
		else if (b == 8) return input::CANCEL;
		else if (b == 9) return input::START;
		// Totally different device types need their own custom mappings
		if (devt.type_match(input::DEVTYPE_DANCEPAD)) {
			// Dance pad can be used for navigation
			if (b == 0) return input::LEFT;
			else if (b == 1) return input::DOWN;
			else if (b == 2) return input::UP;
			else if (b == 3) return input::RIGHT;
			else if (b == 5) return input::MOREUP;
			else if (b == 6) return input::MOREDOWN;
			else return input::NONE;
		} else if (devt.type_match(input::DRUMS)) {
			// Drums can be used for navigation
			if (b == 1) return input::LEFT;
			else if (b == 2) return input::UP;
			else if (b == 3) return input::DOWN;
			else if (b == 4) return input::RIGHT;
		}
	} else if (e.type == SDL_JOYAXISMOTION) {
		// Axis motion
		int axis = e.jaxis.axis;
		int value = e.jaxis.value;
		if (axis == 4 && value > 0) return input::RIGHT;
		else if (axis == 4 && value < 0) return input::LEFT;
		else if (axis == 5 && value > 0) return input::DOWN;
		else if (axis == 5 && value < 0) return input::UP;
	} else if (e.type == SDL_JOYHATMOTION) {
		// Hat motion
		int dir = e.jhat.value;
		// HACK: We probably wan't the guitar strum to scroll songs
		// and main menu items, but they have different orientation.
		// These are switched so it works for now (menu scrolls also on left/right).
		if (input::detail::devices.find(e.jhat.which)->second.type_match(input::GUITAR)) {
			if (dir == SDL_HAT_UP) return input::LEFT;
			else if (dir == SDL_HAT_DOWN) return input::RIGHT;
			else if (dir == SDL_HAT_LEFT) return input::UP;
			else if (dir == SDL_HAT_RIGHT) return input::DOWN;
		} else {
			if (dir == SDL_HAT_UP) return input::UP;
			else if (dir == SDL_HAT_DOWN) return input::DOWN;
			else if (dir == SDL_HAT_LEFT) return input::LEFT;
			else if (dir == SDL_HAT_RIGHT) return input::RIGHT;
		}
	}


// Controller event handling

bool joybutton(input::Event event, SDL_Event const& _e, bool state) {
	using namespace input;
	unsigned int joy_id = _e.jbutton.which;
	if(!devices.find(joy_id)->second.assigned()) return false;
	int button = devices.find(joy_id)->second.buttonFromSDL(_e.jbutton.button);
	if( button == -1 ) return false;
	for( unsigned int i = 0 ; i < BUTTONS ; ++i ) {
		event.pressed[i] = devices.find(joy_id)->second.pressed(i);
	}
	event.type = (state ? input::Event::PRESS : input::Event::RELEASE);
	event.button = button;
	event.pressed[button] = state;
	devices.find(joy_id)->second.addEvent(event);
	return true;
}



	switch(_e.type) {

		case SDL_JOYBUTTONDOWN: return joybutton(event, _e, true);
		case SDL_JOYBUTTONUP: return joybutton(event, _e, false);
		case SDL_JOYAXISMOTION:
		{
			unsigned int joy_id = 0;
			//FIXME: XML axis config is really needed so that these horrible
			//       quirks for RB XBOX360 and GH XPLORER guitars can be removed
			joy_id = _e.jaxis.which;
			InputDevPrivate& dev = devices.find(joy_id)->second;
			if(!dev.assigned()) return false;
			if (dev.name() != "GUITAR_GUITARHERO_XPLORER" && (_e.jaxis.axis == 5 || _e.jaxis.axis == 6 || _e.jaxis.axis == 1)) {
				event.type = input::Event::PICK;
				for( unsigned int i = 0 ; i < BUTTONS ; ++i ) {
					event.pressed[i] = dev.pressed(i);
				}
				// Direction
				event.button = (_e.jaxis.value > 0 ? 0 /* down */: 1 /* up */);
				dev.addEvent(event);
				return true;
			} else if ((dev.name() != "GUITAR_GUITARHERO_XPLORER" && _e.jaxis.axis == 2 )
			  || (dev.name() == "GUITAR_ROCKBAND_XBOX360" && _e.jaxis.axis == 4)
			  || (dev.name() == "GUITAR_ROCKBAND_XBOXADAPTER" && _e.jaxis.axis == 3))
			{
				// Whammy bar (special case for XBox RB guitar
				for( unsigned int i = 0 ; i < BUTTONS ; ++i ) {
					event.pressed[i] = dev.pressed(i);
				}
				event.button = input::WHAMMY_BUTTON;
				event.type = input::Event::PRESS;
				event.pressed[event.button] = (_e.jaxis.value > 0);
				dev.addEvent(event);
				return true;
			} else if ((dev.name() == "GUITAR_ROCKBAND_XBOX360" && _e.jaxis.axis == 3)
			  || (dev.name() == "GUITAR_GUITARHERO_XPLORER" && _e.jaxis.axis == 2)
			  || (dev.name() == "GUITAR_ROCKBAND_XBOXADAPTER" && _e.jaxis.axis ==4))
			{
				// Tilt sensor as an axis on some guitars
				for( unsigned int i = 0 ; i < BUTTONS ; ++i ) {
					event.pressed[i] = dev.pressed(i);
				}
				event.button = input::GODMODE_BUTTON;
				if (_e.jaxis.value < -2) {
					event.type = input::Event::PRESS;
					event.pressed[event.button] = true;
				} else {
					event.type = input::Event::RELEASE;
					event.pressed[event.button] = false;
				}
				dev.addEvent(event);
				return true;
			}
			return false;
		}
		case SDL_JOYHATMOTION:
		{
			unsigned int joy_id = _e.jhat.which;

			if(!devices.find(joy_id)->second.assigned()) return false;
			event.type = input::Event::PICK;
			for( unsigned int i = 0 ; i < BUTTONS ; ++i ) {
				event.pressed[i] = devices.find(joy_id)->second.pressed(i);
			}
			if(_e.jhat.value == SDL_HAT_DOWN ) {
				event.button = 0;
				devices.find(joy_id)->second.addEvent(event);
			} else if(_e.jhat.value == SDL_HAT_UP ) {
				event.button = 1;
				devices.find(joy_id)->second.addEvent(event);
			}
			break;
		}


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


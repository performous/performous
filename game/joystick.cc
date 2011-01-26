#include "joystick.hh"

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

input::Instruments g_instruments;

#include <libxml++/libxml++.h>
#include "fs.hh"

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

#ifdef USE_PORTMIDI

void readConfig(input::MidiDrums::Map& map, fs::path const& file) {
	if (!fs::exists(file)) {
		std::clog << "controllers/info: Skipping " << file << " (not found)" << std::endl;
		return;
	}
	std::clog << "controllers/info: Parsing " << file << std::endl;
	xmlpp::DomParser domParser(file.string());
	try {
		xmlpp::NodeSet n = domParser.get_document()->get_root_node()->find("/mididrums/note");
		for (xmlpp::NodeSet::const_iterator nodeit = n.begin(), end = n.end(); nodeit != end; ++nodeit) {
			xmlpp::Element& elem = dynamic_cast<xmlpp::Element&>(**nodeit);
			int id = boost::lexical_cast<int>(getAttribute(elem, "id"));
			std::string value = getAttribute(elem, "value");
			std::string name = getAttribute(elem, "name");

			// lets erase old mapping for this note
			map.erase(id);

			if(value == "kick") map[id] = input::KICK_BUTTON;
			else if(value == "red") map[id] = input::RED_TOM_BUTTON;
			else if(value == "yellow") map[id] = input::YELLOW_TOM_BUTTON;
			else if(value == "blue") map[id] = input::BLUE_TOM_BUTTON;
			else if(value == "green") map[id] = input::GREEN_TOM_BUTTON;
			else if(value == "orange") map[id] = input::ORANGE_TOM_BUTTON;
		}
	} catch (XMLError& e) {
		int line = e.elem.get_line();
		std::string name = e.elem.get_name();
		throw std::runtime_error(file.string() + ":" + boost::lexical_cast<std::string>(line) + " element " + name + " " + e.message);
	}
}

input::MidiDrums::MidiDrums(): stream(pm::findDevice(true, config["game/midi_input"].s())), devnum(0x8000) {
	readConfig(map, getDefaultConfig(fs::path("/config/mididrums.xml")));
	readConfig(map, getConfigDir() / "mididrums.xml");

	while (detail::devices.find(devnum) != detail::devices.end()) ++devnum;
	detail::devices.insert(std::make_pair<unsigned int, input::detail::InputDevPrivate>(devnum, detail::InputDevPrivate(g_instruments.find("DRUMS_GUITARHERO")->second)));
	event.type = Event::PRESS;
	for (unsigned int i = 0; i < BUTTONS; ++i) event.pressed[i] = false;
}

#include <iomanip>

void input::MidiDrums::process() {
	PmEvent ev;
	while (Pm_Read(stream, &ev, 1) == 1) {
		unsigned char evnt = ev.message & 0xF0;
		unsigned char note = ev.message >> 8;
		unsigned char vel  = ev.message >> 16;
#if 0
		// it is IMHO not a good idea to filter on the channel.
		// code snippet left here for visibility
		unsigned char chan = ev.message & 0x0F;
		if (chan != 0x09) continue; // only accept channel 10 (percussion)
#endif
		if (evnt != 0x90) continue; // 0x90 = any channel note-on
		if (vel  == 0x00) continue; // velocity 0 is often used instead of note-off
		Map::const_iterator it = map.find(note);
		if (it == map.end()) {
			std::cout << "Unassigned MIDI drum event: note " << note << std::endl;
			continue;
		}
		event.button = it->second;
		event.time = now();
		for (unsigned int i = 0; i < BUTTONS; ++i) event.pressed[i] = false;
		event.pressed[it->second] = true;
		detail::devices.find(devnum)->second.addEvent(event);
	}
}
#endif

input::detail::InputDevs input::detail::devices;
input::SDL::SDL_devices input::SDL::sdl_devices;

/// Abstract navigation actions for different input devices, including keyboard
input::NavButton input::getNav(SDL_Event const &e) {
	if (e.type == SDL_KEYDOWN) {
		// Keyboard
		int k = e.key.keysym.sym;
		SDLMod mod = e.key.keysym.mod;
		if (k == SDLK_UP && !(mod & KMOD_CTRL)) return input::UP;
		else if (k == SDLK_DOWN && !(mod & KMOD_CTRL)) return input::DOWN;
		else if (k == SDLK_LEFT) return input::LEFT;
		else if (k == SDLK_RIGHT) return input::RIGHT;
		else if (k == SDLK_RETURN || k == SDLK_KP_ENTER) return input::START;
		else if (k == SDLK_ESCAPE) return input::CANCEL;
		else if (k == SDLK_PAGEUP) return input::MOREUP;
		else if (k == SDLK_PAGEDOWN) return input::MOREDOWN;
		else if (k == SDLK_PAUSE || (k == SDLK_p && mod & KMOD_CTRL)) return input::PAUSE;
		// Volume control is currently handled in main.cc
		//else if (k == SDLK_UP && mod & KMOD_CTRL) return input::VOLUME_UP;
		//else if (k == SDLK_DOWN && mod & KMOD_CTRL) return input::VOLUME_DOWN;
	} else if (e.type == SDL_JOYBUTTONDOWN) {
		// Joystick buttons
		unsigned int joy_id = e.jbutton.which;
		input::detail::InputDevPrivate devt = input::detail::devices.find(joy_id)->second;
		int b = devt.buttonFromSDL(e.jbutton.button);
		if (b == -1) return input::NONE;
		else if (b == 8) return input::CANCEL;
		else if (b == 9) return input::START;
		// Totally different device types need their own custom mappings
		if (devt.type_match(input::DANCEPAD)) {
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
	return input::NONE;
}

void input::SDL::init_devices() {
	for (input::detail::InputDevs::iterator it = input::detail::devices.begin() ; it != input::detail::devices.end() ; ++it) {
		unsigned int id = it->first;
		if(it->second.assigned()) continue;
		if(input::SDL::sdl_devices[id] == NULL) continue; // Keyboard
		unsigned int num_buttons = SDL_JoystickNumButtons(input::SDL::sdl_devices[id]);
		for( unsigned int i = 0 ; i < num_buttons ; ++i ) {
			SDL_Event event;
			int state = SDL_JoystickGetButton(input::SDL::sdl_devices[id], i);
			if( state != 0 ) {
				event.type = SDL_JOYBUTTONDOWN;
				event.jbutton.type = SDL_JOYBUTTONDOWN;
				event.jbutton.state = SDL_PRESSED;
			} else {
				event.type = SDL_JOYBUTTONUP;
				event.jbutton.type = SDL_JOYBUTTONUP;
				event.jbutton.state = SDL_RELEASED;
			}

			event.jbutton.which = id;
			event.jbutton.button = i;
			input::SDL::pushEvent(event);
		}
	}
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
				std::cout << "controllers/info:    Overriding " << type << ": " << name << std::endl;
				instruments.erase(name);
			}
			input::Instrument instrument(name, devType, mapping);
			instrument.match = regexp;
			instrument.description = description;
			instruments.insert(std::make_pair<std::string, input::Instrument>(name, instrument));
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

	ConfigItem::StringList const& instruments = config["game/instruments"].sl();
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
			input::detail::devices.insert(std::make_pair<unsigned int, input::detail::InputDevPrivate>(i, input::detail::InputDevPrivate(forced_type.find(i)->second)));
		} else {
			bool found = false;
			for(input::Instruments::const_iterator it = g_instruments.begin() ; it != g_instruments.end() ; ++it) {
				boost::regex sdl_name(it->second.match);
				if (regex_search(name.c_str(), sdl_name)) {
					std::cout << "  Detected as: " << it->second.description << std::endl;
					input::detail::devices.insert(std::make_pair<unsigned int, input::detail::InputDevPrivate>(i, input::detail::InputDevPrivate(it->second)));
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
	// Here we should send an event to have correct state buttons
	init_devices();
	// Adding keyboard instruments
	std::clog << "controllers/info: Keyboard as guitar controller: " << (config["game/keyboard_guitar"].b() ? "enabled":"disabled") << std::endl;
	std::clog << "controllers/info: Keyboard as drumkit controller: " << (config["game/keyboard_drumkit"].b() ? "enabled":"disabled") << std::endl;
	std::clog << "controllers/info: Keyboard as dance pad controller: " << (config["game/keyboard_dancepad"].b() ? "enabled":"disabled") << std::endl;
	std::clog << "controllers/info: Keyboard as keyboard controller: " << (config["game/keyboard_keyboard"].b() ? "enabled":"disabled") << std::endl;
	input::SDL::sdl_devices[input::detail::KEYBOARD_ID] = NULL;
	input::detail::devices.insert(std::make_pair<unsigned int, input::detail::InputDevPrivate>(input::detail::KEYBOARD_ID, input::detail::InputDevPrivate(g_instruments.find("GUITAR_GUITARHERO")->second)));
	input::SDL::sdl_devices[input::detail::KEYBOARD_ID2] = NULL;
	input::detail::devices.insert(std::make_pair<unsigned int, input::detail::InputDevPrivate>(input::detail::KEYBOARD_ID2, input::detail::InputDevPrivate(g_instruments.find("DRUMS_GUITARHERO")->second)));
	input::SDL::sdl_devices[input::detail::KEYBOARD_ID3] = NULL;
	input::detail::devices.insert(std::make_pair<unsigned int, input::detail::InputDevPrivate>(input::detail::KEYBOARD_ID3, input::detail::InputDevPrivate(g_instruments.find("DANCEPAD_GENERIC")->second)));
	input::SDL::sdl_devices[input::detail::KEYBOARD_ID4] = NULL;
	input::detail::devices.insert(std::make_pair<unsigned int, input::detail::InputDevPrivate>(input::detail::KEYBOARD_ID4, input::detail::InputDevPrivate(g_instruments.find("KEYBOARD_GENERIC")->second)));
}

bool input::SDL::pushEvent(SDL_Event _e) {
	unsigned int joy_id = 0;
	int button;
	using namespace input::detail;

	Event event;
	// Add event time
	event.time = now();
	// Translate to NavButton
	event.nav = getNav(_e);
	static bool pickPressed[2] = {}; // HACK for tracking enter and rshift status
	switch(_e.type) {
		case SDL_KEYDOWN: {
			int button = 0;
			bool is_guitar_event = false;
			bool is_drumkit_event = false;
			bool is_dancepad_event = false;
			bool is_keyboard_event = false;
			event.type = input::Event::PRESS;
			bool guitar = config["game/keyboard_guitar"].b();
			bool drumkit = config["game/keyboard_drumkit"].b();
			bool dancepad = config["game/keyboard_dancepad"].b();
			bool keyboard = config["game/keyboard_keyboard"].b();

			if(!guitar && !drumkit && !dancepad && !keyboard) return false;
			if (_e.key.keysym.mod & (KMOD_CTRL|KMOD_ALT|KMOD_META)) return false;

			switch(_e.key.keysym.sym) {
				case SDLK_RETURN: case SDLK_KP_ENTER:
					if(!guitar) return false;
					if (pickPressed[0]) return true; // repeating
					pickPressed[0] = true;
					event.type = input::Event::PICK;
					button = 0;
					is_guitar_event = true;
					break;
				case SDLK_RSHIFT:
					if(!guitar) return false;
					if (pickPressed[1]) return true; // repeating
					pickPressed[1] = true;
					event.type = input::Event::PICK;
					button = 1;
					is_guitar_event = true;
					break;
				case SDLK_BACKSPACE:
					if(!guitar) return false;
					event.type = input::Event::PRESS;
					button = input::WHAMMY_BUTTON;
					is_guitar_event = true;
					break;
				case SDLK_F12:
					button++;
				case SDLK_F11:
					button++;
				case SDLK_F10:
					button++;
				case SDLK_F9:
					button++;
				case SDLK_F8:
					button++;
				case SDLK_F7:
					if(!keyboard) return false;
					is_keyboard_event = true;
					event.type = input::Event::PRESS;
					break;
				case SDLK_F6: case SDLK_6: case SDLK_n:
					button++;
				case SDLK_F5: case SDLK_5: case SDLK_b:
					button++;
				case SDLK_F4: case SDLK_4: case SDLK_v:
					button++;
				case SDLK_F3: case SDLK_3: case SDLK_c:
					button++;
				case SDLK_F2: case SDLK_2: case SDLK_x:
					button++;
				case SDLK_F1: case SDLK_1: case SDLK_z: case SDLK_w: case SDLK_y:
					if(!guitar) return false;
					is_guitar_event = true;
					event.type = input::Event::PRESS;
					break;
				case SDLK_p:
					button++;
				case SDLK_o:
					button++;
				case SDLK_i:
					button++;
				case SDLK_u:
					button++;
				case SDLK_SPACE:
					if(!drumkit) return false;
					is_drumkit_event = true;
					event.type = input::Event::PRESS;
					event.pressed[button] = true;
					break;
				case SDLK_KP9:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::PRESS;
					button = DOWN_RIGHT_DANCE_BUTTON;
					break;
				case SDLK_KP8: case SDLK_UP:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::PRESS;
					button = UP_DANCE_BUTTON;
					break;
				case SDLK_KP7:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::PRESS;
					button = DOWN_LEFT_DANCE_BUTTON;
					break;
				case SDLK_KP6: case SDLK_RIGHT:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::PRESS;
					button = RIGHT_DANCE_BUTTON;
					break;
				case SDLK_KP5:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::PRESS;
					button = DOWN_DANCE_BUTTON;
					break; // 5 is also down
				case SDLK_KP4: case SDLK_LEFT:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::PRESS;
					button = LEFT_DANCE_BUTTON;
					break;
				case SDLK_KP3:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::PRESS;
					button = UP_RIGHT_DANCE_BUTTON;
					break;
				case SDLK_KP2: case SDLK_DOWN:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::PRESS;
					button = DOWN_DANCE_BUTTON;
					break;
				case SDLK_KP1:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::PRESS;
					button = UP_LEFT_DANCE_BUTTON;
					break;
				default:
					return false;
			}

			if( is_guitar_event ) {
				joy_id = input::detail::KEYBOARD_ID;
			} else if( is_drumkit_event ) {
				joy_id = input::detail::KEYBOARD_ID2;
			} else if( is_dancepad_event ) {
				joy_id = input::detail::KEYBOARD_ID3;
			} else if( is_keyboard_event ) {
				joy_id = input::detail::KEYBOARD_ID4;
			}

			if( is_guitar_event || is_drumkit_event || is_dancepad_event || is_keyboard_event) {
				event.button = button;
				// initialized buttons
				for(unsigned int i = 0 ; i < BUTTONS ; ++i) {
					event.pressed[i] = devices.find(joy_id)->second.pressed(i);
				}
				// if we have a button, set the pushed button to true
				if(event.type == input::Event::PRESS) {
					if(devices.find(joy_id)->second.pressed(event.button)) return true; // repeating
					event.pressed[event.button] = true;
				}
				devices.find(joy_id)->second.addEvent(event);
				return true;
			} else {
				return false;
			}
		}
		case SDL_KEYUP: {
			int button = 0;
			bool is_guitar_event = false;
			bool is_drumkit_event = false;
			bool is_dancepad_event = false;
			bool is_keyboard_event = false;
			event.type = input::Event::PRESS;
			bool guitar = config["game/keyboard_guitar"].b();
			bool drumkit = config["game/keyboard_drumkit"].b();
			bool dancepad = config["game/keyboard_dancepad"].b();
			bool keyboard = config["game/keyboard_keyboard"].b();

			if(!guitar && !drumkit && !dancepad && !keyboard) return false;
			if (_e.key.keysym.mod & (KMOD_CTRL|KMOD_ALT|KMOD_META)) return false;

			switch(_e.key.keysym.sym) {
				case SDLK_RETURN: case SDLK_KP_ENTER:
					if(!guitar) return false;
					pickPressed[0] = false;
					return true;
				case SDLK_RSHIFT:
					if(!guitar) return false;
					pickPressed[1] = false;
					return true;
				case SDLK_BACKSPACE:
					if(!guitar) return false;
					event.type = input::Event::RELEASE;
					button = WHAMMY_BUTTON;
					is_guitar_event = true;
					break;
				case SDLK_F12:
					button++;
				case SDLK_F11:
					button++;
				case SDLK_F10:
					button++;
				case SDLK_F9:
					button++;
				case SDLK_F8:
					button++;
				case SDLK_F7:
					if(!keyboard) return false;
					is_keyboard_event = true;
					event.type = input::Event::RELEASE;
					break;
				case SDLK_F6: case SDLK_6: case SDLK_n:
					button++;
				case SDLK_F5: case SDLK_5: case SDLK_b:
					button++;
				case SDLK_F4: case SDLK_4: case SDLK_v:
					button++;
				case SDLK_F3: case SDLK_3: case SDLK_c:
					button++;
				case SDLK_F2: case SDLK_2: case SDLK_x:
					button++;
				case SDLK_F1: case SDLK_1: case SDLK_z: case SDLK_w: case SDLK_y:
					if(!guitar) return false;
					is_guitar_event = true;
					event.type = input::Event::RELEASE;
					break;
				case SDLK_p:
					button++;
				case SDLK_o:
					button++;
				case SDLK_i:
					button++;
				case SDLK_u:
					button++;
				case SDLK_SPACE:
					if(!drumkit) return false;
					is_drumkit_event = true;
					event.type = input::Event::RELEASE;
					event.pressed[button] = true;
					break;
				case SDLK_KP9:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::RELEASE;
					button = DOWN_RIGHT_DANCE_BUTTON;
					break;
				case SDLK_KP8: case SDLK_UP:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::RELEASE;
					button = UP_DANCE_BUTTON;
					break;
				case SDLK_KP7:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::RELEASE;
					button = DOWN_LEFT_DANCE_BUTTON;
					break;
				case SDLK_KP6: case SDLK_RIGHT:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::RELEASE;
					button = RIGHT_DANCE_BUTTON;
					break;
				case SDLK_KP5:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::RELEASE;
					button = DOWN_DANCE_BUTTON;
					break; // 5 is also down
				case SDLK_KP4: case SDLK_LEFT:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::RELEASE;
					button = LEFT_DANCE_BUTTON;
					break;
				case SDLK_KP3:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::RELEASE;
					button = UP_RIGHT_DANCE_BUTTON;
					break;
				case SDLK_KP2: case SDLK_DOWN:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::RELEASE;
					button = DOWN_DANCE_BUTTON;
					break;
				case SDLK_KP1:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::RELEASE;
					button = UP_LEFT_DANCE_BUTTON;
					break;
				default:
					return false;
			}

			if( is_guitar_event ) {
				joy_id = input::detail::KEYBOARD_ID;
			} else if( is_drumkit_event ) {
				joy_id = input::detail::KEYBOARD_ID2;
			} else if( is_dancepad_event ) {
				joy_id = input::detail::KEYBOARD_ID3;
			} else if( is_keyboard_event ) {
				joy_id = input::detail::KEYBOARD_ID4;
			}

			if( is_guitar_event || is_drumkit_event || is_dancepad_event || is_keyboard_event) {
				event.button = button;
				// initialized buttons
				for(unsigned int i = 0 ; i < BUTTONS ; ++i) {
					event.pressed[i] = devices.find(joy_id)->second.pressed(i);
				}
				// if we have a button, set the pushed button to true
				if(event.type == input::Event::RELEASE) {
					event.pressed[event.button] = false;
				}
				devices.find(joy_id)->second.addEvent(event);
				return true;
			} else {
				return false;
			}
		}
		case SDL_JOYAXISMOTION:
		{
			//FIXME: XML axis config is really needed so that these horrible
			//       quirks for RB XBOX360 and GH XPLORER guitars can be removed
			joy_id = _e.jaxis.which;
			InputDevPrivate& dev = devices.find(joy_id)->second;
			if(!dev.assigned()) return false;
			if (dev.name() != "GUITAR_GUITARHERO_XPLORER" && (_e.jaxis.axis == 5 || _e.jaxis.axis == 6 || _e.jaxis.axis == 1)) {
				event.type = input::Event::PICK;
				// Direction
				if(_e.jaxis.value > 0 ) {
					// down
					event.button = 0;
					dev.addEvent(event);
				} else if(_e.jaxis.value < 0 ) {
					// up
					event.button = 1;
					dev.addEvent(event);
				}
				break;
			} else if ((dev.name() != "GUITAR_GUITARHERO_XPLORER" && _e.jaxis.axis == 2 )
			  || (dev.name() == "GUITAR_ROCKBAND_XBOX360" && _e.jaxis.axis == 4)) {
				// Whammy bar (special case for XBox RB guitar
				for( unsigned int i = 0 ; i < BUTTONS ; ++i ) {
					event.pressed[i] = dev.pressed(i);
				}
				event.button = input::WHAMMY_BUTTON;
				if (_e.jaxis.value > 0) {
					event.type = input::Event::PRESS;
					event.pressed[event.button] = true;
				} else {
					event.type = input::Event::RELEASE;
					event.pressed[event.button] = false;
				}
				dev.addEvent(event);
				break;
			} else if ((dev.name() == "GUITAR_ROCKBAND_XBOX360" && _e.jaxis.axis == 3)
			  || (dev.name() == "GUITAR_GUITARHERO_XPLORER" && _e.jaxis.axis == 2)) {
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
				break;
			} else {
				return false;
			}
			// we should never be there
			break;
		}
		case SDL_JOYHATMOTION:
			joy_id = _e.jhat.which;

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
		case SDL_JOYBUTTONDOWN:
			joy_id = _e.jbutton.which;
			if(!devices.find(joy_id)->second.assigned()) return false;
			button = devices.find(joy_id)->second.buttonFromSDL(_e.jbutton.button);
			if( button == -1 ) return false;
			for( unsigned int i = 0 ; i < BUTTONS ; ++i ) {
				event.pressed[i] = devices.find(joy_id)->second.pressed(i);
			}
			event.type = input::Event::PRESS;
			event.button = button;
			event.pressed[button] = true;
			devices.find(joy_id)->second.addEvent(event);
			break;
		case SDL_JOYBUTTONUP:
			joy_id = _e.jbutton.which;
			if(!devices.find(joy_id)->second.assigned()) return false;
			button = devices.find(joy_id)->second.buttonFromSDL(_e.jbutton.button);
			if( button == -1 ) return false;
			for( unsigned int i = 0 ; i < BUTTONS ; ++i ) {
				event.pressed[i] = devices.find(joy_id)->second.pressed(i);
			}
			event.type = input::Event::RELEASE;
			event.button = button;
			event.pressed[button] = false;
			devices.find(joy_id)->second.addEvent(event);
			break;
		case SDL_JOYBALLMOTION:
		default:
			if (event.nav != input::NONE) {
				event.button = 0;
				event.type = input::Event::PRESS;
				devices.find(joy_id)->second.addEvent(event);
			}
			return false;
	}
	return true;
}


#include "joystick.hh"

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

input::Instruments g_instruments;

#ifdef USE_PORTMIDI
input::MidiDrums::MidiDrums(): stream(pm::findDevice(true, config["system/midi_input"].s())), devnum(0x8000) {
	while (detail::devices.find(devnum) != detail::devices.end()) ++devnum;
	detail::devices.insert(std::make_pair<unsigned int, input::detail::InputDevPrivate>(devnum, detail::InputDevPrivate(g_instruments.find("DRUMS_GUITARHERO")->second)));
	event.type = Event::PRESS;
	for (unsigned int i = 0; i < BUTTONS; ++i) event.pressed[i] = false;
#if 1
	// these are considered "safe" notes that should work with 95% of all modules
	// notes below 35 and above 81 are non standardized

	// *) these mappings have been verified with an Alesis DM5 Pro Kit and RockBand 2 songs
	// the remaining mappings have been guessed...

	static const int DRUM0_ORANGE = 0; // kick drum
	static const int DRUM1_RED    = 1; // snare drum
	static const int DRUM2_YELLOW = 2; // hi-hat/ hi-mid tom
	static const int DRUM3_BLUE   = 3; // low tom/ ride cymbal
	static const int DRUM4_GREEN  = 4; // low floor tom/ crash cymbal

	map[35] = DRUM0_ORANGE;  // 35 - Acoustic Bass Drum
	map[36] = DRUM0_ORANGE;  // 36 - Bass Drum 1 *)
	map[37] = DRUM1_RED;     // 37 - Side Stick
	map[38] = DRUM1_RED;     // 38 - Acoustic Snare *)
	// 39 - Hand Clap
	map[40] = DRUM1_RED;     // 40 - Electric Snare
	map[41] = DRUM4_GREEN;   // 41 - Low Floor Tom *)
	map[42] = DRUM2_YELLOW;  // 42 - Closed Hi-Hat
	// 43 - High Floor Tom
	// map[44] = DRUM2_YELLOW;  // 44 - Pedal Hi-Hat *) - ignore this for playability!
	map[45] = DRUM3_BLUE;    // 45 - Low Tom *)
	map[46] = DRUM2_YELLOW;  // 46 - Open Hi-Hat *)
	// 47 - Low-Mid Tom
	map[48] = DRUM2_YELLOW;  // 48 - Hi-Mid Tom *)
	map[49] = DRUM4_GREEN;   // 49 - Crash Cymbal 1 *)
	// 50 - High Tom
	map[51] = DRUM3_BLUE;    // 51 - Ride Cymbal 1 *)
	// 52 - Chinese Cymbal
	// 53 - Ride Bell
	// 54 - Tambourine
	// 55 - Splash Cymbal
	// 56 - Cowbell
	map[57] = DRUM4_GREEN;   // 57 - Crash Cymbal 2
	// 58 - Vibraslap
	map[59] = DRUM3_BLUE;    // 59 - Ride Cymbal 2
	// 60 - Hi Bongo
	// 61 - Low Bongo
	// 62 - Mute Hi Conga
	// 63 - Open Hi Conga
	// 64 - Low Conga
	// 65 - High Timbale
	// 66 - Low Timbale
	// 67 - High Agogo
	// 68 - Low Agogo
	// 69 - Cabasa
	// 70 - Maracas
	// 71 - Short Whistle
	// 72 - Long Whistle
	// 73 - Short Guiro
	// 74 - Long Guiro
	// 75 - Claves
	// 76 - Hi Wood Block
	// 77 - Low Wood Block
	// 78 - Mute Cuica
	// 79 - Open Cuica
	// 80 - Mute Triangle
	// 81 - Open Triangle
#else
	// this is the original mapping from Performous 0.5.1
	map[35] = map[36] = 0;  // Bass drum 1/2
	map[38] = map[40] = 1;  // Snare 1/2
	map[42] = map[46] = 2;  // Hi-hat closed/open
	map[52] = map[57] = 2;  // Crash2 1/2
	map[41] = map[43] = 3;  // Tom low 1/2
	map[45] = map[47] = 3;  // Tom mid 1/2
	map[48] = map[50] = 3;  // Tom high 1/2
	map[49] = map[51] = 4;  // Cymbal crash/ride
#endif
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

#include "fs.hh"
#include <libxml++/libxml++.h>

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

	std::string regexp_match_force("^([0-9]+):(\\(");
	for(input::Instruments::iterator it = g_instruments.begin() ; it != g_instruments.end() ; ++it) {
		if(it == g_instruments.begin())
			regexp_match_force += it->first;
		else
			regexp_match_force += "|" + it->first;
	}
	regexp_match_force += "\\))$";
	boost::regex match_force(regexp_match_force);
	boost::match_results<const char*> what;

	ConfigItem::StringList const& instruments = config["game/instruments"].sl();
	for (ConfigItem::StringList::const_iterator it = instruments.begin(); it != instruments.end(); ++it) {
		if (!regex_search(it->c_str(), what, match_force)) {
			std::clog << "controllers/error:  " << *it << "\" is not a valid instrument forced value" << std::endl;
			continue;
		} else {
			unsigned int sdl_id = boost::lexical_cast<unsigned int>(what[1]);
			std::string instrument_type(what[2]);

			for(input::Instruments::const_iterator it2 = g_instruments.begin() ; it2 != g_instruments.end() ; ++it2) {
				if(instrument_type == it2->first) {
					forced_type.insert(std::pair<unsigned int,input::Instrument>(sdl_id, input::Instrument(it2->second)));
					break;
				}
			}
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
	input::SDL::sdl_devices[input::detail::KEYBOARD_ID] = NULL;
	input::detail::devices.insert(std::make_pair<unsigned int, input::detail::InputDevPrivate>(input::detail::KEYBOARD_ID, input::detail::InputDevPrivate(g_instruments.find("GUITAR_GUITARHERO")->second)));
	input::SDL::sdl_devices[input::detail::KEYBOARD_ID2] = NULL;
	input::detail::devices.insert(std::make_pair<unsigned int, input::detail::InputDevPrivate>(input::detail::KEYBOARD_ID2, input::detail::InputDevPrivate(g_instruments.find("DRUMS_GUITARHERO")->second)));
	input::SDL::sdl_devices[input::detail::KEYBOARD_ID3] = NULL;
	input::detail::devices.insert(std::make_pair<unsigned int, input::detail::InputDevPrivate>(input::detail::KEYBOARD_ID3, input::detail::InputDevPrivate(g_instruments.find("DANCEPAD_GENERIC")->second)));
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
			event.type = input::Event::PRESS;
			bool guitar = config["game/keyboard_guitar"].b();
			bool drumkit = config["game/keyboard_drumkit"].b();
			bool dancepad = config["game/keyboard_dancepad"].b();

			if(!guitar && !drumkit && !dancepad) return false;
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
				case SDLK_F6: case SDLK_6:
					button++;
				case SDLK_F5: case SDLK_5:
					button++;
				case SDLK_F4: case SDLK_4:
					button++;
				case SDLK_F3: case SDLK_3:
					button++;
				case SDLK_F2: case SDLK_2:
					button++;
				case SDLK_F1: case SDLK_1:
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
			}

			if( is_guitar_event || is_drumkit_event || is_dancepad_event ) {
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
			event.type = input::Event::PRESS;
			bool guitar = config["game/keyboard_guitar"].b();
			bool drumkit = config["game/keyboard_drumkit"].b();
			bool dancepad = config["game/keyboard_dancepad"].b();

			if(!guitar && !drumkit && !dancepad) return false;
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
				case SDLK_F6: case SDLK_6:
					button++;
				case SDLK_F5: case SDLK_5:
					button++;
				case SDLK_F4: case SDLK_4:
					button++;
				case SDLK_F3: case SDLK_3:
					button++;
				case SDLK_F2: case SDLK_2:
					button++;
				case SDLK_F1: case SDLK_1:
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
			}

			if( is_guitar_event || is_drumkit_event || is_dancepad_event ) {
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
			joy_id = _e.jaxis.which;
			if(!devices.find(joy_id)->second.assigned()) return false;
			if (_e.jaxis.axis == 5 || _e.jaxis.axis == 6 || _e.jaxis.axis == 1) {
				event.type = input::Event::PICK;
			} else if (_e.jaxis.axis == 2 || (devices.find(joy_id)->second.name() == "GUITAR_ROCKBAND_XB360" && _e.jaxis.axis == 4)) {
				// nothing to do here
			} else if (devices.find(joy_id)->second.name() == "DRUMS_ROCKBAND_XB360" && _e.jaxis.axis == 3) { // <= WTF HERE !!!
				// nothing to do here
			} else {
				return false;
			}
			for( unsigned int i = 0 ; i < BUTTONS ; ++i ) {
				event.pressed[i] = devices.find(joy_id)->second.pressed(i);
			}
			// XBox RB guitar's Tilt sensor
			if (devices.find(joy_id)->second.name() == "DRUMS_ROCKBAND_XB360" && _e.jaxis.axis == 3) {
				event.button = input::GODMODE_BUTTON;
				if (_e.jaxis.value < -2) {
					event.type = input::Event::PRESS;
					event.pressed[event.button] = true;
				} else {
					event.type = input::Event::RELEASE;
					event.pressed[event.button] = false;
				}
				devices.find(joy_id)->second.addEvent(event);
				break;
			} else if (_e.jaxis.axis == 2 || (devices.find(joy_id)->second.name() == "GUITAR_ROCKBAND_XB360" && _e.jaxis.axis == 4)) {
				event.button = input::WHAMMY_BUTTON;
				if (_e.jaxis.value > 0) {
					event.type = input::Event::PRESS;
					event.pressed[event.button] = true;
				} else {
					event.type = input::Event::RELEASE;
					event.pressed[event.button] = false;
				}
				devices.find(joy_id)->second.addEvent(event);
				break;
			}
			// Direction
			if(_e.jaxis.value > 0 ) { // down
				event.button = 0;
				devices.find(joy_id)->second.addEvent(event);
			} else if(_e.jaxis.value < 0 ) { // up
				event.button = 1;
				devices.find(joy_id)->second.addEvent(event);
			}
			break;
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


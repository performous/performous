#include "joystick.hh"
#include <iostream>

#include <boost/lexical_cast.hpp>

#ifdef USE_PORTMIDI
input::MidiDrums::MidiDrums(int devId): stream(devId), devnum(0x8000 + devId) {
	Private::devices[devnum] = Private::InputDevPrivate(Private::DRUMS_MIDI);
	event.type = Event::PRESS;
	for (unsigned int i = 0; i < BUTTONS; ++i) event.pressed[i] = false;
	map[35] = map[36] = 0;  // Bass drum 1/2
	map[38] = map[40] = 1;  // Snare 1/2
	map[42] = map[46] = 2;  // Hi-hat closed/open
	map[41] = map[43] = 3;  // Tom low 1/2
	map[45] = map[47] = 3;  // Tom mid 1/2
	map[48] = map[50] = 3;  // Tom high 1/2
	map[49] = map[51] = 4;  // Cymbal crash/ride
}

void input::MidiDrums::process() {
	PmEvent ev;
	while (Pm_Read(stream, &ev, 1) == 1) {
		if ((ev.message & 0xFF) != 0x99) continue;
		unsigned char ch = ev.message >> 8;
		//unsigned char vel = ev.message >> 16;
		Map::const_iterator it = map.find(ch);
		if (it == map.end()) {
			std::cout << "Unassigned MIDI drum event: channel " << ch << std::endl;
			continue;
		}
		event.button = it->second;
		event.time = now();
		Private::devices[devnum].addEvent(event);
	}
}
#endif

static const unsigned SDL_BUTTONS = 10;

int input::buttonFromSDL(input::Private::Type _type, unsigned int _sdl_button) {
	static const int inputmap[5][SDL_BUTTONS] = {
		//G  R  Y  B  O  S    // for guitars (S=starpower)
		{ 2, 0, 1, 3, 4, 5, -1, -1, 8, 9 }, // Guitar Hero guitar
		{ 3, 0, 1, 2, 4, 5, -1, -1, 8, 9 }, // Rock Band guitar
		//K  R  Y  B  G  O    // for drums
		{ 3, 4, 1, 2, 0, 4, -1, -1, 8, 9 }, // Guitar Hero drums
		{ 3, 4, 1, 2, 0,-1, -1, -1, 8, 9 }, // Rock Band drums
		// Left  Down  Up  Right  DownL  DownR  UpL    UpR    Start  Select
		{  0,    1,    2,  3,     6,     7,     4,     5,     9,     8 } // generic dance pad
	};
	if( _sdl_button >= SDL_BUTTONS ) return -1;
	switch(_type) {
		case input::Private::GUITAR_GH:
			return inputmap[0][_sdl_button];
		case input::Private::GUITAR_RB:
			return inputmap[1][_sdl_button];
		case input::Private::DRUMS_GH:
			return inputmap[2][_sdl_button];
		case input::Private::DRUMS_RB:
			return inputmap[3][_sdl_button];
		case input::Private::DANCEPAD_GENERIC:
			return inputmap[4][_sdl_button];
		default:
			return -1;
	}
}

input::Private::InputDevs input::Private::devices;
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
		else if (k == SDLK_RETURN) return input::START;
		else if (k == SDLK_ESCAPE) return input::CANCEL;
		else if (k == SDLK_PAGEUP) return input::MOREUP;
		else if (k == SDLK_PAGEDOWN) return input::MOREDOWN;
		else if (k == SDLK_PAUSE || (k == SDLK_p && mod & KMOD_CTRL)) return input::PAUSE;
		else if (k == SDLK_UP && mod & KMOD_CTRL) return input::CTRL_UP;
		else if (k == SDLK_DOWN && mod & KMOD_CTRL) return input::CTRL_DOWN;
	} else if (e.type == SDL_JOYBUTTONDOWN) {
		// Joystick buttons
		unsigned int joy_id = e.jbutton.which;
		input::Private::InputDevPrivate devt = input::Private::devices[joy_id];
		int b = buttonFromSDL(devt.type(), e.jbutton.button);
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
			if (b == 0) return input::START;
			else if (b == 1) return input::LEFT;
			else if (b == 2) return input::UP;
			else if (b == 3) return input::DOWN;
			else if (b == 4) return input::RIGHT;
		}
	// Are these needed?
	/* } else if (e.type == SDL_JOYAXISMOTION) {
		// Axis motion
		int axis = e.jaxis.axis;
		int value = e.jaxis.value;
		if (axis == 4 && value > 0) return input::RIGHT;
		else if (axis == 4 && value < 0) return input::LEFT;
		else if (axis == 5 && value > 0) return input::DOWN;
		else if (axis == 5 && value < 0) return input::UP;
	*/
	} else if (e.type == SDL_JOYHATMOTION) {
		// Hat motion
		int dir = e.jhat.value;
		// HACK: We probably wan't the guitar strum to scroll songs
		// and main menu items, but they have different orientation.
		// These are switched so it works for now (menu scrolls also on left/right).
		if (input::Private::devices[e.jhat.which].type_match(input::GUITAR)) {
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
	for (input::Private::InputDevs::iterator it = input::Private::devices.begin() ; it != input::Private::devices.end() ; ++it) {
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

#include <boost/spirit/include/classic_core.hpp>

void input::SDL::init() {
	unsigned int sdl_id;
	std::string instrument_type;
	std::map<unsigned int, input::Private::Type> forced_type;

	using namespace boost::spirit::classic;
	rule<> type = str_p("GUITAR_GUITARHERO") | "GUITAR_ROCKBAND" | "DRUMS_GUITARHERO" | "DRUMS_ROCKBAND";
	rule<> entry = uint_p[assign_a(sdl_id)] >> ":" >> (type)[assign_a(instrument_type)];

	ConfigItem::StringList const& instruments = config["game/instruments"].sl();
	for (ConfigItem::StringList::const_iterator it = instruments.begin(); it != instruments.end(); ++it) {
		if (!parse(it->c_str(), entry).full) {
			std::cerr << "Error \"" << *it << "\" is not a valid instrument forced value" << std::endl;
			continue;
		} else {
			if (instrument_type == "GUITAR_GUITARHERO") {
				forced_type[sdl_id] = input::Private::GUITAR_GH;
			} else if (instrument_type == "DRUMS_GUITARHERO") {
				forced_type[sdl_id] = input::Private::DRUMS_GH;
			} else if (instrument_type == "GUITAR_ROCKBAND") {
				forced_type[sdl_id] = input::Private::GUITAR_RB;
			} else if (instrument_type == "DRUMS_ROCKBAND") {
				forced_type[sdl_id] = input::Private::DRUMS_GH;
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
			switch(forced_type[i]) {
				case input::Private::GUITAR_GH:
					std::cout << "  Detected as: Guitar Hero Guitar (forced)" << std::endl;
					break;
				case input::Private::DRUMS_GH:
					std::cout << "  Detected as: Guitar Hero Drums (forced)" << std::endl;
					break;
				case input::Private::GUITAR_RB:
					std::cout << "  Detected as: RockBand Guitar (forced)" << std::endl;
					break;
				case input::Private::DRUMS_RB:
					std::cout << "  Detected as: RockBand Drums (forced)" << std::endl;
					break;
				case input::Private::DRUMS_MIDI:
					std::cout << "  Detected as: MIDI Drums (forced)" << std::endl;
					break;
				case input::Private::DANCEPAD_GENERIC:
					std::cout << "  Detected as: Generic dance pad (forced)" << std::endl;
					break;
			}
			input::Private::devices[i] = input::Private::InputDevPrivate(forced_type[i]);
		} else if( name.find("Guitar Hero3") != std::string::npos ) {
			std::cout << "  Detected as: Guitar Hero Guitar" << std::endl;
			input::Private::devices[i] = input::Private::InputDevPrivate(input::Private::GUITAR_GH);
		} else if( name.find("Guitar Hero4") != std::string::npos ) {
			// here we can have both drumkit or guitar .... let say the drumkit
			std::cout << "  Detected as: Guitar Hero Drums (guessed)" << std::endl;
			input::Private::devices[i] = input::Private::InputDevPrivate(input::Private::DRUMS_GH);
		} else if( name.find("Harmonix Guitar") != std::string::npos ) {
			std::cout << "  Detected as: RockBand Guitar" << std::endl;
			input::Private::devices[i] = input::Private::InputDevPrivate(input::Private::GUITAR_RB);
		} else if( name.find("Harmonix Drum Kit") != std::string::npos ) {
			std::cout << "  Detected as: RockBand Drums" << std::endl;
			input::Private::devices[i] = input::Private::InputDevPrivate(input::Private::DRUMS_RB);
		} else if( name.find("Harmonix Drum kit") != std::string::npos ) {
			std::cout << "  Detected as: RockBand Drums" << std::endl;
			input::Private::devices[i] = input::Private::InputDevPrivate(input::Private::DRUMS_RB);
		} else if( name.find("RedOctane USB Pad") != std::string::npos ) {
			std::cout << "  Detected as: Generic Dance Pad" << std::endl;
			input::Private::devices[i] = input::Private::InputDevPrivate(input::Private::DANCEPAD_GENERIC);
		} else if( name.find("Positive Gaming Impact USB pad") != std::string::npos ) {
			std::cout << "  Detected as: Generic Dance Pad" << std::endl;
			input::Private::devices[i] = input::Private::InputDevPrivate(input::Private::DANCEPAD_GENERIC);
		} else if( name.find("Joypad to USB converter") != std::string::npos ) {
			std::cout << "  Detected as: Generic Dance Pad (guessed)" << std::endl;
			input::Private::devices[i] = input::Private::InputDevPrivate(input::Private::DANCEPAD_GENERIC);
		} else {
			std::cout << "  Detected as: Unknwown (please report the name, assuming Guitar Hero Drums)" << std::endl;
			input::Private::devices[i] = input::Private::InputDevPrivate(input::Private::DRUMS_GH);
		}
	}
	// Here we should send an event to have correct state buttons
	init_devices();
	// Adding keyboard instruments
	std::cout << "Keyboard as guitar controller: " << (config["game/keyboard_guitar"].b() ? "enabled":"disabled") << std::endl;
	std::cout << "Keyboard as drumkit controller: " << (config["game/keyboard_drumkit"].b() ? "enabled":"disabled") << std::endl;
	std::cout << "Keyboard as dance pad controller: " << (config["game/keyboard_dancepad"].b() ? "enabled":"disabled") << std::endl;
	input::SDL::sdl_devices[input::Private::KEYBOARD_ID] = NULL;
	input::Private::devices[input::Private::KEYBOARD_ID] = input::Private::InputDevPrivate(input::Private::GUITAR_GH);
	input::SDL::sdl_devices[input::Private::KEYBOARD_ID2] = NULL;
	input::Private::devices[input::Private::KEYBOARD_ID2] = input::Private::InputDevPrivate(input::Private::DRUMS_GH);
	input::SDL::sdl_devices[input::Private::KEYBOARD_ID3] = NULL;
	input::Private::devices[input::Private::KEYBOARD_ID3] = input::Private::InputDevPrivate(input::Private::DANCEPAD_GENERIC);
}

bool input::SDL::pushEvent(SDL_Event _e) {
	unsigned int joy_id = 0;
	int button;
	using namespace input::Private;

	Event event;
	// Add event time
	event.time = now();
	static bool pickPressed[2] = {}; // HACK for tracking enter and rshift status
	switch(_e.type) {
		case SDL_KEYDOWN: {
			if(!config["game/keyboard_guitar"].b() && !config["game/keyboard_drumkit"].b() && !config["game/keyboard_dancepad"].b())
			  return false;
			if (_e.key.keysym.mod & (KMOD_CTRL|KMOD_SHIFT|KMOD_ALT|KMOD_META)) return false;
			int button = 0;
			event.type = input::Event::PRESS;
			if(config["game/keyboard_guitar"].b() && devices[input::Private::KEYBOARD_ID].assigned()) {
				joy_id = input::Private::KEYBOARD_ID;
				switch(_e.key.keysym.sym) {
					case SDLK_RETURN:
						if (pickPressed[0]) return true; // repeating
						pickPressed[0] = true;
						event.type = input::Event::PICK;
						event.button = 1;
						for( unsigned int i = 0 ; i < BUTTONS ; ++i ) {
							event.pressed[i] = devices[joy_id].pressed(i);
						}
						devices[joy_id].addEvent(event);
						return true;
					case SDLK_RSHIFT:
						if (pickPressed[1]) return true; // repeating
						pickPressed[1] = true;
						event.type = input::Event::PICK;
						event.button = 0;
						for( unsigned int i = 0 ; i < BUTTONS ; ++i ) {
							event.pressed[i] = devices[joy_id].pressed(i);
						}
						devices[joy_id].addEvent(event);
						return true;
					case SDLK_BACKSPACE:
						event.type = input::Event::WHAMMY;
						event.button = 1;
						for( unsigned int i = 0 ; i < BUTTONS ; ++i ) {
							event.pressed[i] = devices[joy_id].pressed(i);
						}
						devices[joy_id].addEvent(event);
						return true;
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
						event.type = input::Event::PRESS;
						break;
					default:
						return false;
				}
			} else if(config["game/keyboard_drumkit"].b() && devices[input::Private::KEYBOARD_ID2].assigned()) {
				joy_id = input::Private::KEYBOARD_ID2;
				switch(_e.key.keysym.sym) {
					case SDLK_u:
						button++;
					case SDLK_i:
						button++;
					case SDLK_o:
						button++;
					case SDLK_p:
						event.type = input::Event::PRESS;
						break;
					default:
						return false;
				}
			} else if(config["game/keyboard_dancepad"].b() && devices[input::Private::KEYBOARD_ID3].assigned()) {
				joy_id = input::Private::KEYBOARD_ID3;
				switch(_e.key.keysym.sym) {
					case SDLK_KP9: button = 5; break;
					case SDLK_KP8: case SDLK_UP: button = 2; break;
					case SDLK_KP7: button = 4; break;
					case SDLK_KP6: case SDLK_RIGHT: button = 3; break;
					case SDLK_KP5: button = 1; break; // 5 is also down
					case SDLK_KP4: case SDLK_LEFT: button = 0; break;
					case SDLK_KP3: button = 7; break;
					case SDLK_KP2: case SDLK_DOWN: button = 1; break;
					case SDLK_KP1: button = 6; break;
					default: return false;
				}
			} else {
				return false;
			}
			if(devices[joy_id].pressed(button)) return true; // repeating
			event.button = button;
			for( unsigned int i = 0 ; i < BUTTONS ; ++i ) {
				event.pressed[i] = devices[joy_id].pressed(i);
			}
			event.pressed[button] = true;
			devices[joy_id].addEvent(event);
			break;
		}
		case SDL_KEYUP: {
			if(!config["game/keyboard_guitar"].b() && !config["game/keyboard_dancepad"].b())
			  return false;
			if (_e.key.keysym.mod & (KMOD_CTRL|KMOD_SHIFT|KMOD_ALT|KMOD_META)) return false;
			int button = 0;
			event.type = input::Event::RELEASE;
			if(config["game/keyboard_guitar"].b() && devices[input::Private::KEYBOARD_ID].assigned()) {
				joy_id = input::Private::KEYBOARD_ID;
				switch(_e.key.keysym.sym) {
					case SDLK_RETURN: pickPressed[0] = false; return true;
					case SDLK_RSHIFT: pickPressed[1] = false; return true;
					case SDLK_BACKSPACE:
						event.type = input::Event::WHAMMY;
						event.button = 0;
						for( unsigned int i = 0 ; i < BUTTONS ; ++i ) {
							event.pressed[i] = devices[joy_id].pressed(i);
						}
						devices[joy_id].addEvent(event);
						return true;
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
						event.type = input::Event::RELEASE;
						break;
					default:
						return false;
				}
			} else if(config["game/keyboard_drumkit"].b() && devices[input::Private::KEYBOARD_ID2].assigned()) {
				joy_id = input::Private::KEYBOARD_ID2;
				switch(_e.key.keysym.sym) {
					case SDLK_u:
						button++;
					case SDLK_i:
						button++;
					case SDLK_o:
						button++;
					case SDLK_p:
						event.type = input::Event::RELEASE;
						break;
					default:
						return false;
				}
			} else if(config["game/keyboard_dancepad"].b() && devices[input::Private::KEYBOARD_ID3].assigned()) {
				joy_id = input::Private::KEYBOARD_ID3;
				switch(_e.key.keysym.sym) {
					case SDLK_KP9: button = 5; break;
					case SDLK_KP8: case SDLK_UP: button = 2; break;
					case SDLK_KP7: button = 4; break;
					case SDLK_KP6: case SDLK_RIGHT: button = 3; break;
					case SDLK_KP5: button = 1; break; // 5 is also down
					case SDLK_KP4: case SDLK_LEFT: button = 0; break;
					case SDLK_KP3: button = 7; break;
					case SDLK_KP2: case SDLK_DOWN: button = 1; break;
					case SDLK_KP1: button = 6; break;
					default: return false;
				}
			} else {
				return false;
			}
			event.button = button;
			for( unsigned int i = 0 ; i < BUTTONS ; ++i ) {
				event.pressed[i] = devices[joy_id].pressed(i);
			}
			event.pressed[button] = false;
			devices[joy_id].addEvent(event);
			break;
		}
		case SDL_JOYAXISMOTION:
			joy_id = _e.jaxis.which;
			if(!devices[joy_id].assigned()) return false;
			if (_e.jaxis.axis == 5 || _e.jaxis.axis == 6) {
				event.type = input::Event::PICK;
			} else if (_e.jaxis.axis == 2) {
				event.type = input::Event::WHAMMY;
			} else {
				return false;
			}
			for( unsigned int i = 0 ; i < BUTTONS ; ++i ) {
				event.pressed[i] = devices[joy_id].pressed(i);
			}
			if(_e.jaxis.value > 0 ) { // down
				event.button = 0;
				devices[joy_id].addEvent(event);
			} else if(_e.jaxis.value < 0 ) { // up
				event.button = 1;
				devices[joy_id].addEvent(event);
			}
			break;
		case SDL_JOYHATMOTION:
			joy_id = _e.jhat.which;

			if(!devices[joy_id].assigned()) return false;
			event.type = input::Event::PICK;
			for( unsigned int i = 0 ; i < BUTTONS ; ++i ) {
				event.pressed[i] = devices[joy_id].pressed(i);
			}
			if(_e.jhat.value == SDL_HAT_DOWN ) {
				event.button = 0;
				devices[joy_id].addEvent(event);
			} else if(_e.jhat.value == SDL_HAT_UP ) {
				event.button = 1;
				devices[joy_id].addEvent(event);
			}
			break;
		case SDL_JOYBUTTONDOWN:
			joy_id = _e.jbutton.which;
			if(!devices[joy_id].assigned()) return false;
			button = buttonFromSDL(devices[joy_id].type(),_e.jbutton.button);
			if( button == -1 ) return false;
			for( unsigned int i = 0 ; i < BUTTONS ; ++i ) {
				event.pressed[i] = devices[joy_id].pressed(i);
			}
			event.type = input::Event::PRESS;
			event.button = button;
			event.pressed[button] = true;
			devices[joy_id].addEvent(event);
			break;
		case SDL_JOYBUTTONUP:
			joy_id = _e.jbutton.which;
			if(!devices[joy_id].assigned()) return false;
			button = buttonFromSDL(devices[joy_id].type(),_e.jbutton.button);
			if( button == -1 ) return false;
			for( unsigned int i = 0 ; i < BUTTONS ; ++i ) {
				event.pressed[i] = devices[joy_id].pressed(i);
			}
			event.type = input::Event::RELEASE;
			event.button = button;
			event.pressed[button] = false;
			devices[joy_id].addEvent(event);
			break;
		case SDL_JOYBALLMOTION:
		default:
			return false;
	}
	return true;
}


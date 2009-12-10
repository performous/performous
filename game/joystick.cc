#include "joystick.hh"
#include <iostream>

#include <boost/lexical_cast.hpp>

#ifdef HAVE_PORTMIDI
input::MidiDrums::MidiDrums(int devId): stream(devId) {
	Private::devices[0x8000] = Private::InputDevPrivate(Private::DRUMS_MIDI);
	event.type = Event::PRESS;
	for (unsigned int i = 0; i < BUTTONS; ++i) event.pressed[i] = false;
}

void input::MidiDrums::process() {
	PmEvent ev;
	while (Pm_Read(stream, &ev, 1) == 1) {
		if ((ev.message & 0xFF) != 0x99) continue;
		unsigned char ch = ev.message >> 8;
		//unsigned char vel = ev.message >> 16;
		event.button = ch % 5;
		event.time = now();
		Private::devices[0x8000].addEvent(event);
	}
}
#endif

static const unsigned SDL_BUTTONS = 10;

int buttonFromSDL(input::Private::Type _type, unsigned int _sdl_button) {
	static const int inputmap[5][SDL_BUTTONS] = {
		//G  R  Y  B  O       // for guitars
		{ 2, 0, 1, 3, 4, -1 }, // Guitar Hero guitar
		{ 3, 0, 1, 2, 4, -1 }, // Rock Band guitar
		//K  R  Y  B  G  O    // for drums
		{ 3, 4, 1, 2, 0, 4 }, // Guitar Hero drums
		{ 3, 4, 1, 2, 0, -1 },  // Rock Band drums
		// Left  Down  Up  Right  DownL  DownR  UpL  UpR  Enter  Select
		{  0,    2,    1,  3 } /*   4,     5,     6,   7,   8,     9 } */ // generic dance pad
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
					std::cout << "  Detected as: Generic dance pad" << std::endl;
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
		} else {
			std::cout << "  Detected as: Unknwown (please report the name, assuming Guitar Hero Drums)" << std::endl;
			input::Private::devices[i] = input::Private::InputDevPrivate(input::Private::DRUMS_GH);
		}
	}
	// Here we should send an event to have correct state buttons
	init_devices();
	// Adding keyboard instrument
	std::cout << "Keyboard as guitar controller: " << (config["game/keyboard_guitar"].b() ? "enabled":"disabled") << std::endl;
	input::SDL::sdl_devices[input::Private::KEYBOARD_ID] = NULL;
	input::Private::devices[input::Private::KEYBOARD_ID] = input::Private::InputDevPrivate(input::Private::GUITAR_GH);
}

bool input::SDL::pushEvent(SDL_Event _e) {
	int joy_id;
	int button;
	using namespace input::Private;

	Event event;
	// Add event time
	event.time = now();
	static bool pickPressed[2] = {}; // HACK for tracking enter and rshift status
	switch(_e.type) {
		case SDL_KEYDOWN: {
			if( !config["game/keyboard_guitar"].b() ) return false;
			int button = 0;
			joy_id = input::Private::KEYBOARD_ID;
			if(!devices[joy_id].assigned()) return false;
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
			if( !config["game/keyboard_guitar"].b() ) return false;
			int button = 0;
			joy_id = input::Private::KEYBOARD_ID;
			if(!devices[joy_id].assigned()) return false;
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


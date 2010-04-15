#include "joystick.hh"
#include <iostream>

#include <boost/lexical_cast.hpp>

#ifdef USE_PORTMIDI
input::MidiDrums::MidiDrums(): stream(pm::findDevice(true, config["system/midi_input"].s())), devnum(0x8000) {
	while (detail::devices.find(devnum) != detail::devices.end()) ++devnum;
	detail::devices[devnum] = detail::InputDevPrivate(detail::DRUMS_MIDI);
	event.type = Event::PRESS;
	for (unsigned int i = 0; i < BUTTONS; ++i) event.pressed[i] = false;
	map[35] = map[36] = 0;  // Bass drum 1/2
	map[38] = map[40] = 1;  // Snare 1/2
	map[42] = map[46] = 2;  // Hi-hat closed/open
	map[52] = map[57] = 2;  // Crash2 1/2
	map[41] = map[43] = 3;  // Tom low 1/2
	map[45] = map[47] = 3;  // Tom mid 1/2
	map[48] = map[50] = 3;  // Tom high 1/2
	map[49] = map[51] = 4;  // Cymbal crash/ride
}

#include <iomanip>

void input::MidiDrums::process() {
	PmEvent ev;
	while (Pm_Read(stream, &ev, 1) == 1) {
		if ((ev.message & 0xFF) != 0x99) continue; // 0x99 = channel 10 (percussion) note ON
		unsigned char note = ev.message >> 8;
		//unsigned char vel = ev.message >> 16;
		Map::const_iterator it = map.find(note);
		if (it == map.end()) {
			std::cout << "Unassigned MIDI drum event: note " << note << std::endl;
			continue;
		}
		event.button = it->second;
		event.time = now();
		detail::devices[devnum].addEvent(event);
	}
}
#endif

static const unsigned SDL_BUTTONS = 16;

int input::buttonFromSDL(input::detail::Type _type, unsigned int _sdl_button) {
	static const int inputmap[11][SDL_BUTTONS] = {
		//G  R  Y  B  O  S    // for guitars (S=starpower)
		{ 2, 0, 1, 3, 4, 5, -1, -1,  8,  9, -1, -1, -1, -1, -1, -1 }, // Guitar Hero guitar
		{ 0, 1, 3, 2, 4,-1,  8,  9, -1, -1, -1, -1, -1, -1, -1, -1 }, // Guitar Hero X-plorer guitar
		{ 3, 0, 1, 2, 4, 5, -1, -1,  8,  9, -1, -1, -1, -1, -1, -1 }, // Rock Band guitar PS3
		{ 0, 1, 3, 2, 4,-1,  8,  9, -1, -1, -1, -1, -1, -1, -1, -1 }, // Rock Band guitar XBOX360
		{ 2, 1, 3, 4,-1, 0, -1, -1,  8,  9, -1, -1, -1, -1, -1, -1 }, // Hama Wireless Guitar Controller for PS2 with converter
		//K  R  Y  B  G  O    // for drums
		{ 3, 4, 1, 2, 0, 4, -1, -1,  8,  9, -1, -1, -1, -1, -1, -1 }, // Guitar Hero drums
		{ 3, 4, 1, 2, 0,-1, -1, -1,  8,  9, -1, -1, -1, -1, -1, -1 }, // Rock Band drums PS3
		{ 4, 1, 3, 2, 0,-1,  8,  9, -1, -1, -1, -1, -1, -1, -1, -1 }, // Rock Band drums XBOX360
		// Left  Down  Up  Right  DownL  DownR  UpL    UpR    Start  Select
		{  0,  1,  2,  3,  6,  7,  4,  5,  9,  8, -1, -1, -1, -1, -1, -1 }, // generic dance pad
		{  9,  8, -1, -1, -1, -1, -1, -1,  0,  3,  1,  2, -1, -1, -1, -1 }, // TigerGame dance pad
		{  4,  7,  6,  5, -1, -1, -1, -1,  9,  8, -1, -1,  2,  3,  1,  0 }  // dance pad with ems ps2/pc adapter
	};
	if( _sdl_button >= SDL_BUTTONS ) return -1;
	using namespace detail;
	switch(_type) {
	case GUITAR_GH: return inputmap[0][_sdl_button];
	case GUITAR_GH_XPLORER: return inputmap[1][_sdl_button];
	case GUITAR_RB_PS3: return inputmap[2][_sdl_button];
	case GUITAR_RB_XB360: return inputmap[3][_sdl_button];
	case GUITAR_HAMA_PS2: return inputmap[4][_sdl_button];
	case DRUMS_GH: return inputmap[5][_sdl_button];
	case DRUMS_RB_PS3: return inputmap[6][_sdl_button];
	case DRUMS_RB_XB360: return inputmap[7][_sdl_button];
	case DRUMS_MIDI: throw std::logic_error("MIDI drums do not use SDL buttons");
	case DANCEPAD_GENERIC: return inputmap[8][_sdl_button];
	case DANCEPAD_TIGERGAME: return inputmap[9][_sdl_button];
	case DANCEPAD_EMS2: return inputmap[10][_sdl_button];
	}
	throw std::logic_error("Unknown instrument type in buttonFromSDL");
}

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
		input::detail::InputDevPrivate devt = input::detail::devices[joy_id];
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
		if (input::detail::devices[e.jhat.which].type_match(input::GUITAR)) {
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

#include <boost/spirit/include/classic_core.hpp>

void input::SDL::init() {
	unsigned int sdl_id;
	std::string instrument_type;
	std::map<unsigned int, input::detail::Type> forced_type;

	using namespace boost::spirit::classic;
	rule<> type = str_p("GUITAR_GUITARHERO_XPLORER") | "GUITAR_HAMA_PS2" | "GUITAR_ROCKBAND_PS3"
	  | "GUITAR_ROCKBAND_XB360" | "GUITAR_GUITARHERO" | "DRUMS_GUITARHERO" | "DRUMS_ROCKBAND_PS3"
	  | "DRUMS_ROCKBAND_XB360" | "DRUMS_MIDI" | "DANCEPAD_EMS2" | "DANCEPAD_GENERIC" | "DANCEPAD_TIGERGAME";
	rule<> entry = uint_p[assign_a(sdl_id)] >> ":" >> (type)[assign_a(instrument_type)];

	ConfigItem::StringList const& instruments = config["game/instruments"].sl();
	for (ConfigItem::StringList::const_iterator it = instruments.begin(); it != instruments.end(); ++it) {
		if (!parse(it->c_str(), entry).full) {
			std::cerr << "Error \"" << *it << "\" is not a valid instrument forced value" << std::endl;
			continue;
		} else {
			if (instrument_type == "GUITAR_GUITARHERO") {
				forced_type[sdl_id] = input::detail::GUITAR_GH;
			} else if (instrument_type == "GUITAR_GUITARHERO_XPLORER") {
				forced_type[sdl_id] = input::detail::GUITAR_GH_XPLORER;
			} else if (instrument_type == "GUITAR_HAMA_PS2") {
				forced_type[sdl_id] = input::detail::GUITAR_HAMA_PS2;
			} else if (instrument_type == "DRUMS_GUITARHERO") {
				forced_type[sdl_id] = input::detail::DRUMS_GH;
			} else if (instrument_type == "GUITAR_ROCKBAND_PS3") {
				forced_type[sdl_id] = input::detail::GUITAR_RB_PS3;
			} else if (instrument_type == "GUITAR_ROCKBAND_XB360") {
				forced_type[sdl_id] = input::detail::GUITAR_RB_XB360;
			} else if (instrument_type == "DRUMS_ROCKBAND_PS3") {
				forced_type[sdl_id] = input::detail::DRUMS_RB_PS3;
			} else if (instrument_type == "DRUMS_ROCKBAND_XB360") {
				forced_type[sdl_id] = input::detail::DRUMS_RB_XB360;
			} else if (instrument_type == "DRUMS_MIDI") {
				forced_type[sdl_id] = input::detail::DRUMS_MIDI;
			} else if (instrument_type == "DANCEPAD_GENERIC") {
				forced_type[sdl_id] = input::detail::DANCEPAD_GENERIC;
			} else if (instrument_type == "DANCEPAD_TIGERGAME") {
				forced_type[sdl_id] = input::detail::DANCEPAD_TIGERGAME;
			} else if (instrument_type == "DANCEPAD_EMS2") {
				forced_type[sdl_id] = input::detail::DANCEPAD_EMS2;
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
				case input::detail::GUITAR_GH:
					std::cout << "  Detected as: Guitar Hero Guitar (forced)" << std::endl;
					break;
				case input::detail::GUITAR_GH_XPLORER:
					std::cout << "  Detected as: Guitar Hero Guitar X-plorer (forced)" << std::endl;
					break;
				case input::detail::GUITAR_HAMA_PS2:
					std::cout << "  Detected as: Hama Guitar for PS2 with converter (forced)" << std::endl;
					break;
				case input::detail::DRUMS_GH:
					std::cout << "  Detected as: Guitar Hero Drums (forced)" << std::endl;
					break;
				case input::detail::GUITAR_RB_PS3:
					std::cout << "  Detected as: RockBand Guitar PS3 (forced)" << std::endl;
					break;
				case input::detail::GUITAR_RB_XB360:
					std::cout << "  Detected as: RockBand Guitar Xbox360 (forced)" << std::endl;
					break;
				case input::detail::DRUMS_RB_PS3:
					std::cout << "  Detected as: RockBand Drums PS3 (forced)" << std::endl;
					break;
				case input::detail::DRUMS_RB_XB360:
					std::cout << "  Detected as: RockBand Drums Xbox360 (forced)" << std::endl;
					break;
				case input::detail::DRUMS_MIDI:
					std::cout << "  Detected as: MIDI Drums (forced)" << std::endl;
					break;
				case input::detail::DANCEPAD_TIGERGAME:
					std::cout << "  Detected as: TigerGame dance pad (forced)" << std::endl;
					break;
				case input::detail::DANCEPAD_GENERIC:
					std::cout << "  Detected as: Generic dance pad (forced)" << std::endl;
					break;
				case input::detail::DANCEPAD_EMS2:
					std::cout << "  Detected as: EMS2 dance pad controller converter (forced)" << std::endl;
					break;
			}
			input::detail::devices[i] = input::detail::InputDevPrivate(forced_type[i]);
		} else if( name.find("Guitar Hero3") != std::string::npos ) {
			std::cout << "  Detected as: Guitar Hero Guitar" << std::endl;
			input::detail::devices[i] = input::detail::InputDevPrivate(input::detail::GUITAR_GH);
		} else if( name.find("Guitar Hero X-plorer") != std::string::npos ) {
			std::cout << "  Detected as: Guitar Hero Guitar X-plorer" << std::endl;
			input::detail::devices[i] = input::detail::InputDevPrivate(input::detail::GUITAR_GH_XPLORER);
		} else if( name.find("PS  Converter") != std::string::npos ) {
			std::cout << "  Detected as: Hama Guitar with converter (guessed)" << std::endl;
			input::detail::devices[i] = input::detail::InputDevPrivate(input::detail::GUITAR_HAMA_PS2);
		} else if( name.find("Guitar Hero4") != std::string::npos ) {
			// here we can have both drumkit or guitar .... let say the drumkit
			std::cout << "  Detected as: Guitar Hero Drums (guessed)" << std::endl;
			input::detail::devices[i] = input::detail::InputDevPrivate(input::detail::DRUMS_GH);
		} else if( name.find("RedOctane MIDI Drum GuitarHero") != std::string::npos ) {
			// This is GH Metallica guitar
			std::cout << "  Detected as: Guitar Hero Guitar" << std::endl;
			input::detail::devices[i] = input::detail::InputDevPrivate(input::detail::GUITAR_GH);
		} else if( name.find("Harmonix Guitar") != std::string::npos ) {
			if (name.find("Xbox") != std::string::npos) {
				std::cout << "  Detected as: RockBand Guitar Xbox360" << std::endl;
				input::detail::devices[i] = input::detail::InputDevPrivate(input::detail::GUITAR_RB_XB360);
			} else {
				std::cout << "  Detected as: RockBand Guitar PS3" << std::endl;
				input::detail::devices[i] = input::detail::InputDevPrivate(input::detail::GUITAR_RB_PS3);
			}
		} else if( name.find("Harmonix Drum Kit") != std::string::npos || name.find("Harmonix Drum kit") != std::string::npos) {
			if (name.find("Xbox") != std::string::npos) {
				std::cout << "  Detected as: RockBand Drums Xbox360" << std::endl;
				input::detail::devices[i] = input::detail::InputDevPrivate(input::detail::DRUMS_RB_XB360);
			} else {
				std::cout << "  Detected as: RockBand Drums PS3" << std::endl;
				input::detail::devices[i] = input::detail::InputDevPrivate(input::detail::DRUMS_RB_PS3);
			}
		} else if( name.find("Mad Catz Portable Drum") != std::string::npos) {
			std::cout << "  Detected as: RockBand Drums Xbox360" << std::endl;
			input::detail::devices[i] = input::detail::InputDevPrivate(input::detail::DRUMS_RB_XB360);
		} else if( name.find("TigerGame") != std::string::npos ) {
			std::cout << "  Detected as: TigerGame Dance Pad" << std::endl;
			input::detail::devices[i] = input::detail::InputDevPrivate(input::detail::DANCEPAD_TIGERGAME);
		} else if( name.find("Redoctane XBOX DDR") != std::string::npos ) {
			std::cout << "  Detected as: Generic Dance Pad" << std::endl;
			input::detail::devices[i] = input::detail::InputDevPrivate(input::detail::DANCEPAD_GENERIC);
		} else if( name.find("Dance mat") != std::string::npos ) {
			std::cout << "  Detected as: Generic Dance Pad" << std::endl;
			input::detail::devices[i] = input::detail::InputDevPrivate(input::detail::DANCEPAD_GENERIC);
		} else if( name.find("RedOctane USB Pad") != std::string::npos ) {
			std::cout << "  Detected as: Generic Dance Pad" << std::endl;
			input::detail::devices[i] = input::detail::InputDevPrivate(input::detail::DANCEPAD_GENERIC);
		} else if( name.find("Positive Gaming Impact USB pad") != std::string::npos ) {
			std::cout << "  Detected as: Generic Dance Pad" << std::endl;
			input::detail::devices[i] = input::detail::InputDevPrivate(input::detail::DANCEPAD_GENERIC);
		} else if( name.find("Joypad to USB converter") != std::string::npos ) {
			std::cout << "  Detected as: Generic Dance Pad (guessed)" << std::endl;
			input::detail::devices[i] = input::detail::InputDevPrivate(input::detail::DANCEPAD_GENERIC);
		} else if( name.find("0b43:0003") != std::string::npos ) {
			std::cout << "  Detected as: EMS2 Dance Pad controller converter (guessed)" << std::endl;
			input::detail::devices[i] = input::detail::InputDevPrivate(input::detail::DANCEPAD_EMS2);
		} else {
			std::cout << "  Detected as: Unknown (please report the name; use config to force detection)" << std::endl;
			SDL_JoystickClose(joy);
			continue;
		}
	}
	// Here we should send an event to have correct state buttons
	init_devices();
	// Adding keyboard instruments
	std::cout << "Keyboard as guitar controller: " << (config["game/keyboard_guitar"].b() ? "enabled":"disabled") << std::endl;
	std::cout << "Keyboard as drumkit controller: " << (config["game/keyboard_drumkit"].b() ? "enabled":"disabled") << std::endl;
	std::cout << "Keyboard as dance pad controller: " << (config["game/keyboard_dancepad"].b() ? "enabled":"disabled") << std::endl;
	input::SDL::sdl_devices[input::detail::KEYBOARD_ID] = NULL;
	input::detail::devices[input::detail::KEYBOARD_ID] = input::detail::InputDevPrivate(input::detail::GUITAR_GH);
	input::SDL::sdl_devices[input::detail::KEYBOARD_ID2] = NULL;
	input::detail::devices[input::detail::KEYBOARD_ID2] = input::detail::InputDevPrivate(input::detail::DRUMS_GH);
	input::SDL::sdl_devices[input::detail::KEYBOARD_ID3] = NULL;
	input::detail::devices[input::detail::KEYBOARD_ID3] = input::detail::InputDevPrivate(input::detail::DANCEPAD_GENERIC);
}

bool input::SDL::pushEvent(SDL_Event _e) {
	unsigned int joy_id = 0;
	int button;
	using namespace input::detail;

	Event event;
	// Add event time
	event.time = now();
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
					button = 1;
					is_guitar_event = true;
					break;
				case SDLK_RSHIFT:
					if(!guitar) return false;
					if (pickPressed[1]) return true; // repeating
					pickPressed[1] = true;
					event.type = input::Event::PICK;
					button = 0;
					is_guitar_event = true;
					break;
				case SDLK_BACKSPACE:
					if(!guitar) return false;
					event.type = input::Event::WHAMMY;
					button = 1;
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
					if(devices[joy_id].pressed(button)) return true; // repeating
					is_drumkit_event = true;
					event.type = input::Event::PRESS;
					event.pressed[button] = true;
					break;
				case SDLK_KP9:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::PRESS;
					button = 5;
					break;
				case SDLK_KP8: case SDLK_UP:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::PRESS;
					button = 2;
					break;
				case SDLK_KP7:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::PRESS;
					button = 4;
					break;
				case SDLK_KP6: case SDLK_RIGHT:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::PRESS;
					button = 3;
					break;
				case SDLK_KP5:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::PRESS;
					button = 1;
					break; // 5 is also down
				case SDLK_KP4: case SDLK_LEFT:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::PRESS;
					button = 0;
					break;
				case SDLK_KP3:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::PRESS;
					button = 7;
					break;
				case SDLK_KP2: case SDLK_DOWN:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::PRESS;
					button = 1;
					break;
				case SDLK_KP1:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::PRESS;
					button = 6;
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
					event.pressed[i] = devices[joy_id].pressed(i);
				}
				// if we have a button, set the pushed button to true
				if(event.type == input::Event::PRESS) {
					if(devices[joy_id].pressed(event.button)) return true; // repeating
					event.pressed[event.button] = true;
				}
				devices[joy_id].addEvent(event);
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
					event.type = input::Event::WHAMMY;
					button = 0;
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
					if(devices[joy_id].pressed(button)) return true; // repeating
					is_drumkit_event = true;
					event.type = input::Event::RELEASE;
					event.pressed[button] = true;
					break;
				case SDLK_KP9:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::RELEASE;
					button = 5;
					break;
				case SDLK_KP8: case SDLK_UP:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::RELEASE;
					button = 2;
					break;
				case SDLK_KP7:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::RELEASE;
					button = 4;
					break;
				case SDLK_KP6: case SDLK_RIGHT:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::RELEASE;
					button = 3;
					break;
				case SDLK_KP5:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::RELEASE;
					button = 1;
					break; // 5 is also down
				case SDLK_KP4: case SDLK_LEFT:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::RELEASE;
					button = 0;
					break;
				case SDLK_KP3:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::RELEASE;
					button = 7;
					break;
				case SDLK_KP2: case SDLK_DOWN:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::RELEASE;
					button = 1;
					break;
				case SDLK_KP1:
					if(!dancepad) return false;
					is_dancepad_event = true;
					event.type = input::Event::RELEASE;
					button = 6;
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
					event.pressed[i] = devices[joy_id].pressed(i);
				}
				// if we have a button, set the pushed button to true
				if(event.type == input::Event::RELEASE) {
					event.pressed[event.button] = false;
				}
				devices[joy_id].addEvent(event);
				return true;
			} else {
				return false;
			}
		}
		case SDL_JOYAXISMOTION:
			joy_id = _e.jaxis.which;
			if(!devices[joy_id].assigned()) return false;
			if (_e.jaxis.axis == 5 || _e.jaxis.axis == 6 || _e.jaxis.axis == 1) {
				event.type = input::Event::PICK;
			} else if (_e.jaxis.axis == 2 || (devices[joy_id].type() == input::detail::GUITAR_RB_XB360
			  && _e.jaxis.axis == 4)) {
				event.type = input::Event::WHAMMY;
			} else {
				return false;
			}
			for( unsigned int i = 0 ; i < BUTTONS ; ++i ) {
				event.pressed[i] = devices[joy_id].pressed(i);
			}
			// XBox RB guitar's Tilt sensor
			if (devices[joy_id].type() == input::detail::GUITAR_RB_XB360 && _e.jaxis.axis == 3) {
				event.button = input::STARPOWER_BUTTON;
				if (_e.jaxis.value < -2) {
					event.type = input::Event::PRESS;
					event.pressed[event.button] = true;
				} else {
					event.type = input::Event::RELEASE;
					event.pressed[event.button] = false;
				}
				devices[joy_id].addEvent(event);
				break;
			}
			// Direction
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


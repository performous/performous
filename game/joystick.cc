#include "joystick.hh"
#include <iostream>

#include <boost/lexical_cast.hpp>

/*
enum TypeSDL {UNKNOWN, GUITAR_RB, DRUM_RB, GUITAR_GH, DRUM_GH, TYPESDL_N};
static const unsigned SDL_BUTTONS = 6;

int buttonFromSDL(TypeSDL type, unsigned sdlButton) {
	static const int inputmap[TYPESDL_N][SDL_BUTTONS] = {
		{ 2, 0, 1, 3, 4, 0 }, // Guitar Hero guitar
		{ 3, 4, 1, 2, 0, 4 }, // Guitar Hero drums
		{ 3, 0, 1, 2, 4, 0 }, // Rock Band guitar
		{ 3, 4, 1, 2, 0, 0 }  // Rock Band drums
	};
	if (sdlButton > SDL_BUTTONS) return -1;
	return inputmap[type][sdlButton];
}
*/


/*

As reference:

void GuitarGraph::inputProcess() {
	for (Joysticks::iterator it = joysticks.begin(); it != joysticks.end(); ++it) {
		for (JoystickEvent ev; it->second.tryPollEvent(ev); ) {
			// RockBand pick event
			if (ev.type == JoystickEvent::HAT_MOTION && ev.hat_direction != JoystickEvent::CENTERED) picked = true;
			// GuitarHero pick event
			if (ev.type == JoystickEvent::AXIS_MOTION && ev.axis_id == 5 && ev.axis_value != 0) picked = true;
			if (ev.type != JoystickEvent::BUTTON_DOWN && ev.type != JoystickEvent::BUTTON_UP) continue;
			unsigned b = ev.button_id;
			if (b >= 6) continue;
			static const int inputmap[][6] = {
				{ 2, 0, 1, 3, 4, 0 }, // Guitar Hero guitar
				{ 3, 4, 1, 2, 0, 4 }, // Guitar Hero drums
				{ 3, 0, 1, 2, 4, 0 }, // Rock Band guitar
				{ 3, 4, 1, 2, 0, 0 }  // Rock Band drums
			};
			int instrument = 2 * (it->second.getType() == Joystick::ROCKBAND) + m_drums;
			int button = inputmap[instrument][b];
			fretPressed[button] = (ev.type == JoystickEvent::BUTTON_DOWN);
			if (fretPressed[button]) fretHit[button] = true;
		}
	}
}

*/

input::Private::InputDevs input::Private::devices;
input::SDL_devices input::sdl_devices;

void input::init_devices() {
	for (input::Private::InputDevs::iterator it = input::Private::devices.begin() ; it != input::Private::devices.end() ; ++it) {
		unsigned int id = it->first;
		if( it->second.assigned() ) continue;
		unsigned int num_buttons = SDL_JoystickNumButtons(input::sdl_devices[id]);
		for( unsigned int i = 0 ; i < num_buttons ; ++i ) {
			SDL_Event event;
			event.type = SDL_JOYBUTTONDOWN;
			event.jbutton.type = SDL_JOYBUTTONDOWN;
			event.jbutton.which = id;
			event.jbutton.button = i;
			event.jbutton.state = SDL_PRESSED;
			input::pushEvent(event);
		}
	}
}

void input::init() {
	unsigned int nbjoysticks = SDL_NumJoysticks();
	std::cout << "Detecting " << nbjoysticks << " joysticks..." << std::endl;

	for (unsigned int i = 0 ; i < nbjoysticks ; ++i) {
		input::sdl_devices[i] = SDL_JoystickOpen(i);
		std::string name = SDL_JoystickName(i);
		std::cout << "Id: " << i << std::endl;
		std::cout << "  Name: " << name << std::endl;
		if( name.find("Guitar Hero3") != std::string::npos ) {
			input::Private::devices[i] = input::Private::InputDevPrivate(input::Private::GUITAR_GH);
			std::cout << "  Detected as: Guitar Hero Guitar" << std::endl;
		} else if( name.find("Guitar Hero4") != std::string::npos ) {
			// here we can have both drumkit or guitar .... let say the drumkit
			input::Private::devices[i] = input::Private::InputDevPrivate(input::Private::DRUM_GH);
			std::cout << "  Detected as: Guitar Hero Drums (guessed)" << std::endl;
		} else if( name.find("Harmonix Guitar") != std::string::npos ) {
			std::cout << "  Detected as: RockBand Guitar" << std::endl;
			input::Private::devices[i] = input::Private::InputDevPrivate(input::Private::GUITAR_RB);
		} else if( name.find("Harmonix Drum Kit") != std::string::npos ) {
			std::cout << "  Detected as: RockBand Drums" << std::endl;
			input::Private::devices[i] = input::Private::InputDevPrivate(input::Private::DRUM_RB);
		} else {
			std::cout << "  Detected as: Unknwown (please report the name)" << std::endl;
			input::Private::devices[i] = input::Private::InputDevPrivate();
		}
	}
	// Here we should send an event to have correct state buttons
	init_devices();
}

bool input::pushEvent(SDL_Event _e) {
	int joy_id;
	using namespace input::Private;
	switch(_e.type) {
		case SDL_JOYAXISMOTION:
			joy_id = _e.jaxis.which;
			// do stuffs to devices[joy_id] events (PICK)
			break;
		case SDL_JOYHATMOTION:
			joy_id = _e.jhat.which;
			// do stuffs to devices[joy_id] events (PICK)
			break;
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			joy_id = _e.jbutton.which;
			// do stuffs to devices[joy_id] states (BUTTONS)
			// do stuffs to devices[joy_id] events (BUTTONS)
			break;
		case SDL_JOYBALLMOTION:
		default:
			return false;
	}
	return true;
}

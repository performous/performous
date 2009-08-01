#include "joystick.hh"
#include <iostream>

#include <boost/lexical_cast.hpp>

enum TypeSDL {UNKNOWN, GUITAR_RB, DRUM_RB, GUITAR_GH, DRUM_GH, TYPESDL_N};
static const int SDL_BUTTONS = 6;

int buttonFromSDL(TypeSDL type, unsigned sdlButton) {
	static const int inputmap[TYPESDL_N][SDL_BUTTONS] = {
		{ 2, 0, 1, 3, 4, 0 }, // Guitar Hero guitar
		{ 3, 4, 1, 2, 0, 4 }, // Guitar Hero drums
		{ 3, 0, 1, 2, 4, 0 }, // Rock Band guitar
		{ 3, 4, 1, 2, 0, 0 }  // Rock Band drums
	};
	if (sdlButton > SDL_BUTTONS) return -1;
	return inputmap[instrument][sdlButton];
}

Joysticks joysticks;

Joystick::Joystick(Joystick::Type _type): m_type(_type) { }

Joystick::~Joystick() {
};

Joystick::Type Joystick::getType() const {
	return m_type;
}

void Joystick::addEvent(SDL_JoyAxisEvent event) {
	JoystickEvent joy_event(JoystickEvent::AXIS_MOTION);

	joy_event.axis_id = event.axis;
	joy_event.axis_value = event.value;

	m_events.push_back(joy_event);
}

void Joystick::addEvent(SDL_JoyBallEvent event) {
	JoystickEvent joy_event(JoystickEvent::BALL_MOTION);

	joy_event.ball_id = event.ball;
	joy_event.ball_dx = event.xrel;
	joy_event.ball_dy = event.yrel;

	m_events.push_back(joy_event);
}

void Joystick::addEvent(SDL_JoyHatEvent event) {
	JoystickEvent joy_event(JoystickEvent::HAT_MOTION);

	joy_event.hat_id = event.hat;
	switch(event.value) {
		case SDL_HAT_LEFTUP:
			joy_event.hat_direction = JoystickEvent::LEFT_UP;
			break;
		case SDL_HAT_LEFT:
			joy_event.hat_direction = JoystickEvent::LEFT;
			break;
		case SDL_HAT_LEFTDOWN:
			joy_event.hat_direction = JoystickEvent::LEFT_DOWN;
			break;
		case SDL_HAT_UP:
			joy_event.hat_direction = JoystickEvent::UP;
			break;
		case SDL_HAT_CENTERED:
			joy_event.hat_direction = JoystickEvent::CENTERED;
			break;
		case SDL_HAT_DOWN:
			joy_event.hat_direction = JoystickEvent::DOWN;
			break;
		case SDL_HAT_RIGHTUP:
			joy_event.hat_direction = JoystickEvent::RIGHT_UP;
			break;
		case SDL_HAT_RIGHT:
			joy_event.hat_direction = JoystickEvent::RIGHT;
			break;
		case SDL_HAT_RIGHTDOWN:
			joy_event.hat_direction = JoystickEvent::RIGHT_DOWN;
			break;
		default:
			joy_event.hat_direction = JoystickEvent::CENTERED;
			break;
	}

	m_events.push_back(joy_event);
}

void Joystick::addEvent(SDL_JoyButtonEvent event) {
	JoystickEvent joy_event((event.type == SDL_JOYBUTTONDOWN) ? JoystickEvent::BUTTON_DOWN : JoystickEvent::BUTTON_UP);

	joy_event.button_id = event.button;
	joy_event.button_state = event.state;

	m_events.push_back(joy_event);

}
bool Joystick::tryPollEvent(JoystickEvent& _event) {
	if( m_events.empty() ) return false;

	_event = m_events.front();
	m_events.pop_front();
	return true;
}

void Joystick::clearEvents() {
	m_events.clear();
}

/**
 * New input management, superseed all joysticks stuffs
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
	// new stuffs
	unsigned int nbjoysticks = SDL_NumJoysticks();
	std::cout << "Detecting " << nbjoysticks << " joysticks..." << std::endl;

	for (unsigned int i = 0 ; i < nbjoysticks ; ++i) {
		input::sdl_devices[i] = SDL_JoystickOpen(i);
		std::string name = SDL_JoystickName(i);
		if( name.find("Guitar Hero3") != std::string::npos ) {
			input::Private::devices[i] = input::Private::InputDevPrivate(input::Private::GUITAR_GH);
		} else if( name.find("Guitar Hero4") != std::string::npos ) {
			// here we can have both drumkit or guitar .... let say the drumkit
			input::Private::devices[i] = input::Private::InputDevPrivate(input::Private::DRUM_GH);
		} else if( name.find("Harmonix Guitar") != std::string::npos ) {
			input::Private::devices[i] = input::Private::InputDevPrivate(input::Private::GUITAR_RB);
		} else if( name.find("Harmonix Drum Kit") != std::string::npos ) {
			input::Private::devices[i] = input::Private::InputDevPrivate(input::Private::DRUM_RB);
		} else {
			input::Private::devices[i] = input::Private::InputDevPrivate();
		}
		std::cout << "Id: " << i << std::endl;
		std::cout << "  Name: " << name << std::endl;
	}
	// Here we should send an event to have correct state buttons
	init_devices();
	// compatibility with old joystick stuffs
	for (input::Private::InputDevs::iterator it = input::Private::devices.begin() ; it != input::Private::devices.end() ; ++it) {
		if( it->second.type() == Private::GUITAR_GH || it->second.type() == Private::DRUM_GH ) {
			joysticks[it->first] = Joystick(Joystick::GUITARHERO);
		} else if( it->second.type() == Private::GUITAR_GH || it->second.type() == Private::DRUM_GH ) {
			joysticks[it->first] = Joystick(Joystick::ROCKBAND);
		} else {
			joysticks[it->first] = Joystick();
		}
	}
}
void input::clear() {
	// new stuffs
	// nothing to do here
	// compatibility with old joystick stuffs
	for(Joysticks::iterator it = joysticks.begin() ; it != joysticks.end() ; ++it ) {
		it->second.clearEvents();
	}
}
bool input::pushEvent(SDL_Event _e) {
	// new stuffs
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
	// compatibility with old joystick stuffs
	switch(_e.type) {
		case SDL_JOYAXISMOTION:
			joysticks[_e.jaxis.which].addEvent(_e.jaxis);
			break;
		case SDL_JOYBALLMOTION:
			joysticks[_e.jball.which].addEvent(_e.jball);
			break;
		case SDL_JOYHATMOTION:
			joysticks[_e.jhat.which].addEvent(_e.jhat);
			break;
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			joysticks[_e.jbutton.which].addEvent(_e.jbutton);
			break;
		default:
			return false;
	}
	return true;
}

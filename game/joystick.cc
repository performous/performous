#include "joystick.hh"
#include <iostream>

#include <boost/lexical_cast.hpp>

Joysticks joysticks;

Joystick::Joystick(unsigned int _id): m_id(_id) {
	if( getName().find("Guitar Hero") != std::string::npos ) {
		m_type = Joystick::GUITARHERO;
	} else if( getName().find("Harmonix") != std::string::npos ) {
		m_type = Joystick::ROCKBAND;
	} else {
		m_type = Joystick::UNKNOWN;
	}
};
Joystick::~Joystick() {
	// Do it another way :/
	//if(SDL_JoystickOpened(m_id)) SDL_JoystickClose(m_joystick);
};
std::string Joystick::getName() const {
	return std::string(SDL_JoystickName(m_id));
};
Joystick::Type Joystick::getType() const {
	return m_type;
}
bool Joystick::buttonState(unsigned char _button_id) const {
	return (SDL_JoystickGetButton(m_joystick, _button_id) != 0);
};
JoystickEvent::HatDirection Joystick::hat(unsigned char _hat_id) const {
	unsigned char direction = SDL_JoystickGetHat(m_joystick, _hat_id);
	switch(direction) {
		case SDL_HAT_LEFTUP:
			return JoystickEvent::LEFT_UP;
		case SDL_HAT_LEFT:
			return JoystickEvent::LEFT;
		case SDL_HAT_LEFTDOWN:
			return JoystickEvent::LEFT_DOWN;
		case SDL_HAT_UP:
			return JoystickEvent::UP;
		case SDL_HAT_CENTERED:
			return JoystickEvent::CENTERED;
		case SDL_HAT_DOWN:
			return JoystickEvent::DOWN;
		case SDL_HAT_RIGHTUP:
			return JoystickEvent::RIGHT_UP;
		case SDL_HAT_RIGHT:
			return JoystickEvent::RIGHT;
		case SDL_HAT_RIGHTDOWN:
			return JoystickEvent::RIGHT_DOWN;
	}
	// we should not go here
	return JoystickEvent::CENTERED;
};
short Joystick::axis(unsigned char _axis_id) const {
	return SDL_JoystickGetAxis(m_joystick, _axis_id);
};
std::pair<int, int> Joystick::ball(int _ball_id) const {
	int dx, dy;
	SDL_JoystickGetBall(m_joystick, _ball_id, &dx,&dy);
	return std::pair<int,int>(dx,dy);
};

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

void input::init() {
	// new stuffs
	unsigned int nbjoysticks = SDL_NumJoysticks();
	std::cout << "Detecting " << nbjoysticks << " joysticks..." << std::endl;

	for (unsigned int i = 0 ; i < nbjoysticks ; i++) {
		SDL_JoystickOpen(i);
		input::Private::devices[i] = input::Private::InputDevPrivate();
		std::cout << "Id: " << i << std::endl;
		std::cout << "  Name: " << SDL_JoystickName(i) << std::endl;
	}
	// compatibility with old joystick stuffs
	for (input::Private::InputDevs::iterator it = input::Private::devices.begin() ; it != input::Private::devices.end() ; ++it) {
		joysticks[it->first] = Joystick(it->first);
	}
}
void input::clear() {
	// new stuffs
	for (input::Private::InputDevs::iterator it = input::Private::devices.begin() ; it != input::Private::devices.end() ; ++it) {
		it->second.clearEvents();
	}
	// compatibility with old joystick stuffs
	for(Joysticks::iterator it = joysticks.begin() ; it != joysticks.end() ; ++it ) {
		it->second.clearEvents();
	}
}
bool input::pushEvent(SDL_Event _e) {
	// new stuffs
	// TODO
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

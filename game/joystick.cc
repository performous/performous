#include "joystick.hh"
#include <iostream>

#include <boost/lexical_cast.hpp>

Joysticks joysticks;

Joystick::Joystick(unsigned int _id): m_id(_id) {
	m_joystick = SDL_JoystickOpen(m_id);
};
Joystick::~Joystick() {
	// Do it another way :/
	//if(SDL_JoystickOpened(m_id)) SDL_JoystickClose(m_joystick);
};
std::string Joystick::getName() const {
	return std::string(SDL_JoystickName(m_id));
};
std::string Joystick::getDescription() const {
	std::string desc;
	desc += "axes: ";
	desc += boost::lexical_cast<std::string>(SDL_JoystickNumAxes(m_joystick));
	desc += ", buttons: ";
	desc += boost::lexical_cast<std::string>(SDL_JoystickNumButtons(m_joystick));
	desc += ", balls: ";
	desc += boost::lexical_cast<std::string>(SDL_JoystickNumBalls(m_joystick));
	desc += ", hats: ";
	desc += boost::lexical_cast<std::string>(SDL_JoystickNumHats(m_joystick));
	return desc;
};
bool Joystick::buttonState(unsigned char _button_id) const {
	return (SDL_JoystickGetButton(m_joystick, _button_id) != 0);
};
unsigned char Joystick::hat(unsigned char _hat_id) const {
	return SDL_JoystickGetHat(m_joystick, _hat_id);
};
short Joystick::axe(unsigned char _axe_id) const {
	return SDL_JoystickGetAxis(m_joystick, _axe_id);
};
std::pair<int, int> Joystick::ball(int _ball_id) const {
	int dx, dy;
	SDL_JoystickGetBall(m_joystick, _ball_id, &dx,&dy);
	return std::pair<int,int>(dx,dy);
};

void joysticks_init() {
	unsigned int nbjoysticks = SDL_NumJoysticks();
	std::cout << "Detecting " << nbjoysticks << " joysticks..." << std::endl;

	for (unsigned int i = 0 ; i < nbjoysticks ; i++) {
		joysticks[i] = Joystick(i);
		std::cout << "Id: " << i << std::endl;
		std::cout << "  Name: " << joysticks[i].getName() << std::endl;
		std::cout << "  Description: " << joysticks[i].getDescription() << std::endl;
	}
}

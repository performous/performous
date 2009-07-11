#include "joystick.hh"
#include "fs.hh"
#include <iostream>
#include <cstdlib>

class Joystick {
  public:
  	Joystick() {};
	Joystick(unsigned int _id): m_id(_id) {
		m_joystick = SDL_JoystickOpen(m_id);
	};
	~Joystick() {/*if(SDL_JoystickOpened(m_id)) SDL_JoystickClose(m_joystick);*/}; // should be called before SDL finished
	std::string getName() const {return std::string(SDL_JoystickName(m_id));};
	std::string getDescription() const {
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
	bool buttonState(unsigned char _button_id) { return (SDL_JoystickGetButton(m_joystick, _button_id) != 0);};
	unsigned char hat(unsigned char _hat_id) { return SDL_JoystickGetHat(m_joystick, _hat_id);};
	short axe(unsigned char _axe_id) { return SDL_JoystickGetAxis(m_joystick, _axe_id);};
	std::pair<int, int> ball(int _ball_id) {int dx, dy;SDL_JoystickGetBall(m_joystick, _ball_id, &dx,&dy);return std::pair<int,int>(dx,dy);};
  private:
	SDL_Joystick * m_joystick;
	unsigned int m_id;
};

std::map<unsigned int,Joystick> joysticks;

#define PS3_DRUM_CONTROLLER_BLUE   0
#define PS3_DRUM_CONTROLLER_GREEN  1
#define PS3_DRUM_CONTROLLER_RED    2
#define PS3_DRUM_CONTROLLER_YELLOW 3
#define PS3_DRUM_CONTROLLER_XXX    4
#define PS3_DRUM_CONTROLLER_ORANGE 5

void check_joystick_event(SDL_Event event, Audio &audio) {
	switch( event.type ) {
		case SDL_JOYAXISMOTION:
			break;
		case SDL_JOYHATMOTION:
			break;
		case SDL_JOYBALLMOTION:
			// relatives things, we do not want to manage this for the moment
			break;
		case SDL_JOYBUTTONDOWN:
			switch( event.jbutton.button ) {
				case PS3_DRUM_CONTROLLER_RED: // Snare drum
					audio.playSample(getDataPath("sounds/drum_snare.ogg"));
					break;
				case PS3_DRUM_CONTROLLER_BLUE: // Tom 1
					audio.playSample(getDataPath("sounds/drum_tom1.ogg"));
					break;
				case PS3_DRUM_CONTROLLER_GREEN: // Tom 2
					audio.playSample(getDataPath("sounds/drum_tom2.ogg"));
					break;
				case PS3_DRUM_CONTROLLER_YELLOW: // Hi hat
					audio.playSample(getDataPath("sounds/drum_hi-hat.ogg"));
					break;
				case PS3_DRUM_CONTROLLER_ORANGE: // crash cymbal
					audio.playSample(getDataPath("sounds/drum_cymbal.ogg"));
					break;
				case PS3_DRUM_CONTROLLER_XXX: // Drum bass
					audio.playSample(getDataPath("sounds/drum_bass.ogg"));
					break;
			}
			break;
		case SDL_JOYBUTTONUP:
			break;
	}
}
void probe() {
	unsigned int nbjoysticks = SDL_NumJoysticks();
	printf("Number of joysticks: %u\n\n", nbjoysticks);

	for (unsigned int i = 0 ; i < nbjoysticks ; i++) {
		joysticks[i] = Joystick(i);
		std::cout << "Id: " << i << std::endl;
		std::cout << "Name: " << joysticks[i].getName() << std::endl;
		std::cout << "Description: " << joysticks[i].getDescription() << std::endl;
	}
}

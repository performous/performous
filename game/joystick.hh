#pragma once

#include "SDL_joystick.h"
#include "audio.hh"

#define PS3_DRUM_CONTROLLER_BLUE   0
#define PS3_DRUM_CONTROLLER_GREEN  1
#define PS3_DRUM_CONTROLLER_RED    2
#define PS3_DRUM_CONTROLLER_YELLOW 3
#define PS3_DRUM_CONTROLLER_XXX    4
#define PS3_DRUM_CONTROLLER_ORANGE 5

void joysticks_init();

/// joystick class used for dealing with ps3 drumsets
class Joystick {
  public:
	Joystick() {};
	/// create joystick object with given id
	Joystick(unsigned int _id);
	~Joystick();
	/// get name of joystick
	std::string getName() const;
	/// get longer description of joystick
	std::string getDescription() const;
	/// returns true if the button with _button_id is pressed
	bool buttonState(unsigned char _button_id)  const;
	/// hi-hat
	unsigned char hat(unsigned char _hat_id) const;
	short axe(unsigned char _axe_id) const;
	std::pair<int, int> ball(int _ball_id) const;
  private:
	SDL_Joystick * m_joystick;
	unsigned int m_id;
};

typedef std::map<unsigned int,Joystick> Joysticks;
extern Joysticks joysticks;

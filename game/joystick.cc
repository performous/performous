#include "joystick.hh"
#include "fs.hh"
#include <iostream>
#include <cstdlib>

class Joystick {
  public:
  	Joystick() {};
	Joystick(unsigned int _id): m_id(_id) {
		m_joystick = SDL_JoystickOpen(m_id);
		for( int i = 0 ; i < SDL_JoystickNumButtons(m_joystick) ; i++ ) {
			m_buttons_states[i] = false;
		}
		for( int i = 0 ; i < SDL_JoystickNumAxes(m_joystick) ; i++ ) {
			m_axes_values[i] = 0;
		}
		for( int i = 0 ; i < SDL_JoystickNumHats(m_joystick) ; i++ ) {
			m_hats_values[i] = 0;
		}
	};
	~Joystick() {/*if(SDL_JoystickOpened(m_id)) SDL_JoystickClose(m_joystick);*/}; // should be called before SDL finished
	std::string getName() const {return std::string(SDL_JoystickName(m_id));};
	std::string getDescription() const {
		std::string desc;
		desc += "axes: ";
		desc += boost::lexical_cast<std::string>(m_axes_values.size());
		desc += ", buttons: ";
		desc += boost::lexical_cast<std::string>(m_buttons_states.size());
		desc += ", hats: ";
		desc += boost::lexical_cast<std::string>(m_hats_values.size());
		return desc;
	};
	void buttonPressed( int _button_id) { m_buttons_states[_button_id] = true; };
	void buttonReleased( int _button_id) { m_buttons_states[_button_id] = false; };
	bool buttonState(int _button_id) { return m_buttons_states[_button_id];};
	void hat( int _hat_id, int _value) { m_hats_values[_hat_id] = _value;};
	void axe( int _axe_id, int _value) { m_axes_values[_axe_id] = _value;};

	int hat( int _hat_id) { return m_hats_values[_hat_id];};
	int axe( int _axe_id) { return m_axes_values[_axe_id];};
  private:
	SDL_Joystick * m_joystick;
	unsigned int m_id;
	std::map<int, bool> m_buttons_states;
	std::map<int, int> m_axes_values;
	std::map<int, int> m_hats_values;
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
			joysticks[(int)event.jaxis.which].axe((int)event.jaxis.axis,(int)event.jaxis.value);
			break;
		case SDL_JOYHATMOTION:
			joysticks[(int)event.jhat.which].axe((int)event.jhat.hat,(int)event.jhat.value);
			break;
		case SDL_JOYBALLMOTION:
			// relatives things, we do not want to manage this for the moment
			break;
		case SDL_JOYBUTTONDOWN:
			joysticks[(int)event.jbutton.which].buttonPressed((int)event.jbutton.button);
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
			joysticks[(int)event.jbutton.which].buttonReleased((int)event.jbutton.button);
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

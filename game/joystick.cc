#include "joystick.hh"
#include "fs.hh"
#include <iostream>
#include <cstdlib>

#define PS3_DRUM_CONTROLLER_BLUE   0
#define PS3_DRUM_CONTROLLER_GREEN  1
#define PS3_DRUM_CONTROLLER_RED    2
#define PS3_DRUM_CONTROLLER_YELLOW 3
#define PS3_DRUM_CONTROLLER_XXX    4
#define PS3_DRUM_CONTROLLER_ORANGE 5

>>>>>>> dd75dc2... Removed PERFORMOUS_CONFIG_SCHEMA and add getDataPath:game/joystick.cc
void check_joystick_event(SDL_Event event, Audio &audio) {
	switch( event.type ) {
		case SDL_JOYAXISMOTION:
			std::cout << "Received an axis motion on "  << (int)event.jaxis.which << std::endl;
			break;
		case SDL_JOYHATMOTION:
			std::cout << "Received an hat motion on "  << (int)event.jhat.which << std::endl;
			break;
		case SDL_JOYBALLMOTION:
			std::cout << "Received an ball motion on "  << (int)event.jball.which << std::endl;
			break;
		case SDL_JOYBUTTONDOWN:
			std::cout << "Received a button down on "  << (int)event.jbutton.which << " on " << (int)event.jbutton.button << std::endl;
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
			std::cout << "Received a button up on " << (int)event.jbutton.which << std::endl;
			break;
	}
}
void probe() {
	int nbjoysticks = SDL_NumJoysticks();
	printf("Number of joysticks: %d\n\n", nbjoysticks);

	for (int i = 0 ; i < nbjoysticks ; i++) {
		SDL_Joystick * joy = SDL_JoystickOpen(i);
		printf("Joystick %d %s\n", i, SDL_JoystickName(i));
		printf("Axes: %d\n", SDL_JoystickNumAxes(joy));
		printf("Buttons: %d\n", SDL_JoystickNumButtons(joy));
		printf("Trackballs: %d\n", SDL_JoystickNumBalls(joy));
		printf("Hats: %d\n\n", SDL_JoystickNumHats(joy));
	}
}

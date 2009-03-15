#include "joystick.hh"
#include <iostream>
#include <cstdlib>

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
				case 2: // Snare drum
					audio.playSample("/tmp/drum_snare.ogg");
					break;
				case 0: // Tom 1
					audio.playSample("/tmp/drum_tom1.ogg");
					break;
				case 1: // Tom 2
					audio.playSample("/tmp/drum_tom2.ogg");
					break;
				case 3: // Hi hat
					audio.playSample("/tmp/drum_hi-hat.ogg");
					break;
				case 5: // crash cymbal
					audio.playSample("/tmp/drum_cymbal.ogg");
					break;
				case 4: // Drum bass
					audio.playSample("/tmp/drum_bass.ogg");
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

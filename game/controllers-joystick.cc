#include "controllers.hh"

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
			input::SDL::pushEvent(event, boost::xtime());
		}
	}
}




	} else if (e.type == SDL_JOYBUTTONDOWN) {
		// Joystick buttons
		unsigned int joy_id = e.jbutton.which;
		input::detail::InputDevPrivate devt = input::detail::devices.find(joy_id)->second;
		int b = devt.buttonFromSDL(e.jbutton.button);
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
			if (b == 1) return input::LEFT;
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
		if (input::detail::devices.find(e.jhat.which)->second.type_match(input::GUITAR)) {
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


bool joybutton(input::Event event, SDL_Event const& _e, bool state) {
	using namespace input;
	using namespace input::detail;
	unsigned int joy_id = _e.jbutton.which;
	if(!devices.find(joy_id)->second.assigned()) return false;
	int button = devices.find(joy_id)->second.buttonFromSDL(_e.jbutton.button);
	if( button == -1 ) return false;
	for( unsigned int i = 0 ; i < BUTTONS ; ++i ) {
		event.pressed[i] = devices.find(joy_id)->second.pressed(i);
	}
	event.type = (state ? input::Event::PRESS : input::Event::RELEASE);
	event.button = button;
	event.pressed[button] = state;
	devices.find(joy_id)->second.addEvent(event);
	return true;
}



	switch(_e.type) {

		case SDL_JOYBUTTONDOWN: return joybutton(event, _e, true);
		case SDL_JOYBUTTONUP: return joybutton(event, _e, false);
		case SDL_JOYAXISMOTION:
		{
			unsigned int joy_id = 0;
			//FIXME: XML axis config is really needed so that these horrible
			//       quirks for RB XBOX360 and GH XPLORER guitars can be removed
			joy_id = _e.jaxis.which;
			InputDevPrivate& dev = devices.find(joy_id)->second;
			if(!dev.assigned()) return false;
			if (dev.name() != "GUITAR_GUITARHERO_XPLORER" && (_e.jaxis.axis == 5 || _e.jaxis.axis == 6 || _e.jaxis.axis == 1)) {
				event.type = input::Event::PICK;
				for( unsigned int i = 0 ; i < BUTTONS ; ++i ) {
					event.pressed[i] = dev.pressed(i);
				}
				// Direction
				event.button = (_e.jaxis.value > 0 ? 0 /* down */: 1 /* up */);
				dev.addEvent(event);
				return true;
			} else if ((dev.name() != "GUITAR_GUITARHERO_XPLORER" && _e.jaxis.axis == 2 )
			  || (dev.name() == "GUITAR_ROCKBAND_XBOX360" && _e.jaxis.axis == 4)
			  || (dev.name() == "GUITAR_ROCKBAND_XBOXADAPTER" && _e.jaxis.axis == 3))
			{
				// Whammy bar (special case for XBox RB guitar
				for( unsigned int i = 0 ; i < BUTTONS ; ++i ) {
					event.pressed[i] = dev.pressed(i);
				}
				event.button = input::WHAMMY_BUTTON;
				event.type = input::Event::PRESS;
				event.pressed[event.button] = (_e.jaxis.value > 0);
				dev.addEvent(event);
				return true;
			} else if ((dev.name() == "GUITAR_ROCKBAND_XBOX360" && _e.jaxis.axis == 3)
			  || (dev.name() == "GUITAR_GUITARHERO_XPLORER" && _e.jaxis.axis == 2)
			  || (dev.name() == "GUITAR_ROCKBAND_XBOXADAPTER" && _e.jaxis.axis ==4))
			{
				// Tilt sensor as an axis on some guitars
				for( unsigned int i = 0 ; i < BUTTONS ; ++i ) {
					event.pressed[i] = dev.pressed(i);
				}
				event.button = input::GODMODE_BUTTON;
				if (_e.jaxis.value < -2) {
					event.type = input::Event::PRESS;
					event.pressed[event.button] = true;
				} else {
					event.type = input::Event::RELEASE;
					event.pressed[event.button] = false;
				}
				dev.addEvent(event);
				return true;
			}
			return false;
		}
		case SDL_JOYHATMOTION:
		{
			unsigned int joy_id = _e.jhat.which;

			if(!devices.find(joy_id)->second.assigned()) return false;
			event.type = input::Event::PICK;
			for( unsigned int i = 0 ; i < BUTTONS ; ++i ) {
				event.pressed[i] = devices.find(joy_id)->second.pressed(i);
			}
			if(_e.jhat.value == SDL_HAT_DOWN ) {
				event.button = 0;
				devices.find(joy_id)->second.addEvent(event);
			} else if(_e.jhat.value == SDL_HAT_UP ) {
				event.button = 1;
				devices.find(joy_id)->second.addEvent(event);
			}
			break;
		}


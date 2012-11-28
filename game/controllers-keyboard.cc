
	if (e.type == SDL_KEYDOWN) {
		// Keyboard
		int k = e.key.keysym.sym;
		SDLMod mod = e.key.keysym.mod;
		if (k == SDLK_UP && !(mod & KMOD_CTRL)) return input::UP;
		else if (k == SDLK_DOWN && !(mod & KMOD_CTRL)) return input::DOWN;
		else if (k == SDLK_LEFT) return input::LEFT;
		else if (k == SDLK_RIGHT) return input::RIGHT;
		else if (k == SDLK_RETURN || k == SDLK_KP_ENTER) return input::START;
		else if (k == SDLK_ESCAPE) return input::CANCEL;
		else if (k == SDLK_PAGEUP) return input::MOREUP;
		else if (k == SDLK_PAGEDOWN) return input::MOREDOWN;
		else if (k == SDLK_PAUSE || (k == SDLK_p && mod & KMOD_CTRL)) return input::PAUSE;
		// Volume control is currently handled in main.cc
		//else if (k == SDLK_UP && mod & KMOD_CTRL) return input::VOLUME_UP;
		//else if (k == SDLK_DOWN && mod & KMOD_CTRL) return input::VOLUME_DOWN;



bool keybutton(input::Event event, SDL_Event const& _e, bool state) {
	using namespace input;
	using namespace input::detail;
	unsigned int joy_id = 0;
	int button = 0;
	event.type = (state ? input::Event::PRESS : input::Event::RELEASE);
	bool guitar = config["game/keyboard_guitar"].b();
	bool drumkit = config["game/keyboard_drumkit"].b();
	bool dancepad = config["game/keyboard_dancepad"].b();
	bool keyboard = config["game/keyboard_keyboard"].b();


	switch(_e.key.keysym.sym) {
		// Guitar picking on keyboard
		case SDLK_RSHIFT: button++; // Button number allows using secondary pick for menu navigation
		case SDLK_RETURN: case SDLK_KP_ENTER:
			if (!guitar) return false;
			if (!state) return true;
			event.type = input::Event::PICK;
			joy_id = input::detail::KEYBOARD_GUITAR;
			break;

		// Guitar buttons on keyboard
		case SDLK_BACKSPACE: button++;  // Whammy
		case SDLK_RCTRL: button++;  // God mode
		case SDLK_F5: case SDLK_5: case SDLK_b: button++;
		case SDLK_F4: case SDLK_4: case SDLK_v: button++;
		case SDLK_F3: case SDLK_3: case SDLK_c: button++;
		case SDLK_F2: case SDLK_2: case SDLK_x: button++;
		case SDLK_F1: case SDLK_1: case SDLK_z: case SDLK_w: case SDLK_y:
			if (!guitar) return false;
			joy_id = KEYBOARD_GUITAR;
			break;

		// Keyboard/keytar on keyboard
		case SDLK_F12: button++;
		case SDLK_F11: button++;
		case SDLK_F10: button++;
		case SDLK_F9: button++;
		case SDLK_F8: button++;
		case SDLK_F7:
			if (!keyboard) return false;
			joy_id = KEYBOARD_KEYBOARD;
			break;

		// Drums on keyboard
		case SDLK_p: button++;
		case SDLK_o: button++;
		case SDLK_i: button++;
		case SDLK_u: button++;
		case SDLK_SPACE:
			if(!drumkit) return false;
			joy_id = KEYBOARD_DRUMS;
			event.pressed[button] = state;
			break;

		// Dance on keypad
		case SDLK_KP9: button++;
		case SDLK_KP7: button++;
		case SDLK_KP3: button++;
		case SDLK_KP1: button++;
		case SDLK_KP6: case SDLK_RIGHT: button++;
		case SDLK_KP8: case SDLK_UP: button++;
		case SDLK_KP2: case SDLK_DOWN: case SDLK_KP5: button++;
		case SDLK_KP4: case SDLK_LEFT:
			if(!dancepad) return false;
			joy_id = KEYBOARD_DANCEPAD;
			break;

		default: return false;
	}

	// We need to push an event
	event.button = button;
	// Read old button status from device
	for(unsigned int i = 0 ; i < BUTTONS ; ++i) {
		event.pressed[i] = devices.find(joy_id)->second.pressed(i);
	}
	// if we have a holdable button, update the pressed status
	if (event.type == input::Event::PRESS || event.type == input::Event::RELEASE) {
		event.pressed[event.button] = state;
	}
	devices.find(joy_id)->second.addEvent(event);
	return true;
}

	switch(_e.type) {
		case SDL_KEYDOWN: return keybutton(event, _e, true);
		case SDL_KEYUP: return keybutton(event, _e, false);
		default: return false;
	}
	return true;


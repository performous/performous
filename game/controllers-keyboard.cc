#include "controllers.hh"

namespace input {
	class Keyboard: public Hardware {
	public:
		bool process(Event& event, SDL_Event const& sdlEv) {
			if (sdlEv.type != SDL_KEYDOWN && sdlEv.type != SDL_KEYUP) return false;
			event.source = SourceId(SOURCETYPE_KEYBOARD);
			event.hw = sdlEv.key.keysym.sym;
			event.value = (sdlEv.type == SDL_KEYDOWN ? 1.0 : 0.0);
			mapping(event);
			event.navButton = navigation(event, sdlEv);
			return event.devType != DEVTYPE_NONE || event.navButton != NONE;
		}
		void mapping(Event& event) {
			unsigned button = 0;
			switch (event.hw) {
				// Guitar on keyboard
				case SDLK_RSHIFT: button++;
				case SDLK_RETURN: case SDLK_KP_ENTER: button++;
				case SDLK_BACKSPACE: button++;  // Whammy
				case SDLK_RCTRL: button++;  // God mode
				case SDLK_F5: case SDLK_5: case SDLK_b: button++;
				case SDLK_F4: case SDLK_4: case SDLK_v: button++;
				case SDLK_F3: case SDLK_3: case SDLK_c: button++;
				case SDLK_F2: case SDLK_2: case SDLK_x: button++;
				case SDLK_F1: case SDLK_1: case SDLK_z: case SDLK_w: case SDLK_y:  // Support also French and German layouts
					if (!config["game/keyboard_guitar"].b()) return;
					event.devType = DEVTYPE_GUITAR;
					break;

				// Keytar on keyboard
				case SDLK_F12: button++;
				case SDLK_F11: button++;
				case SDLK_F10: button++;
				case SDLK_F9: button++;
				case SDLK_F8: button++;
				case SDLK_F7:
					if (!config["game/keyboard_keyboard"].b()) return;  // FIXME: Rename config option to keytar
					event.devType = DEVTYPE_KEYTAR;
					break;

				// Drums on keyboard
				case SDLK_p: button++;
				case SDLK_o: button++;
				case SDLK_i: button++;
				case SDLK_u: button++;
				case SDLK_SPACE:
					if (!config["game/keyboard_drumkit"].b()) return;
					event.devType = DEVTYPE_DRUMS;
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
					if (!config["game/keyboard_dancepad"].b()) return;
					event.devType = DEVTYPE_DANCEPAD;
					break;

				default: return;
			}
			event.id = Button(button);
			event.source.channel = event.devType;  // Each type gets its own unique SourceId
		}
		NavButton navigation(Event& event, SDL_Event const& sdlEv) {
			unsigned k = event.hw;
			SDLMod mod = sdlEv.key.keysym.mod;
			if (k == SDLK_UP) return mod & KMOD_CTRL ? input::VOLUME_UP : input::UP;
			if (k == SDLK_DOWN) return mod & KMOD_CTRL ? input::VOLUME_DOWN : input::DOWN;
			if (k == SDLK_LEFT) return input::LEFT;
			if (k == SDLK_RIGHT) return input::RIGHT;
			if (k == SDLK_RETURN || k == SDLK_KP_ENTER) return input::START;
			if (k == SDLK_ESCAPE) return input::CANCEL;
			if (k == SDLK_PAGEUP) return input::MOREUP;
			if (k == SDLK_PAGEDOWN) return input::MOREDOWN;
			if (k == SDLK_PAUSE || (k == SDLK_p && mod & KMOD_CTRL)) return input::PAUSE;
			return input::NONE;
		}
	};

	Hardware::ptr constructKeyboard() { return Hardware::ptr(new Keyboard()); }
	
}


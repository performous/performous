#include "controllers.hh"

namespace {
	bool g_enableInstruments = false;
}

namespace input {
	class Keyboard: public Hardware {
	public:
		bool process(Event& event, SDL_Event const& sdlEv) {
			if (sdlEv.type != SDL_KEYDOWN && sdlEv.type != SDL_KEYUP) return false;
			event.source = SourceId(SOURCETYPE_KEYBOARD, sdlEv.key.which);  // Device number .which is always zero with SDL 1.2 :(
			event.hw = sdlEv.key.keysym.sym;
			event.value = (sdlEv.type == SDL_KEYDOWN ? 1.0 : 0.0);
			// Get the modifier keys that we actually use as modifiers
			unsigned mod = sdlEv.key.keysym.mod & (KMOD_LCTRL | KMOD_LALT);
			// Map to keyboard instruments (sets event.button if matching)
			if (g_enableInstruments && !mod) mapping(event);
			// Map to menu navigation
			if (event.button == GENERIC_UNASSIGNED) event.button = navigation(event.hw, mod);
			return event.button != GENERIC_UNASSIGNED;
		}
		void mapping(Event& event) {
			unsigned button = 0;
			switch (event.hw) {
				// Guitar on keyboard
				case SDLK_BACKSPACE: button = GUITAR_WHAMMY; goto guitar_process;
				case SDLK_RCTRL: button = GUITAR_GODMODE; goto guitar_process;
				case SDLK_RSHIFT: button = GUITAR_PICK_UP; goto guitar_process;
				case SDLK_RETURN: case SDLK_KP_ENTER: button = GUITAR_PICK_DOWN; goto guitar_process;
				case SDLK_F5: case SDLK_5: case SDLK_b: button++;
				case SDLK_F4: case SDLK_4: case SDLK_v: button++;
				case SDLK_F3: case SDLK_3: case SDLK_c: button++;
				case SDLK_F2: case SDLK_2: case SDLK_x: button++;
				case SDLK_F1: case SDLK_1: case SDLK_z: case SDLK_w: case SDLK_y:  // Support also French and German layouts
				guitar_process:
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
					if (!config["game/keyboard_keytar"].b()) return;
					event.devType = DEVTYPE_KEYTAR;
					break;

				// Drums on keyboard
				case SDLK_8: button = DRUMS_YELLOW_CYMBAL; goto drum_process;
				case SDLK_9: button = DRUMS_GREEN_CYMBAL; goto drum_process;
				case SDLK_0: button = DRUMS_BLUE_CYMBAL; goto drum_process;
				case SDLK_u: button = DRUMS_RED; goto drum_process;
				case SDLK_i: button = DRUMS_YELLOW_TOM; goto drum_process;
				case SDLK_o: button = DRUMS_BLUE_TOM; goto drum_process;
				case SDLK_p: button = DRUMS_GREEN_TOM; goto drum_process;
				case SDLK_SPACE:
					drum_process:
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
			event.button = ButtonId(button);
			event.source.channel = event.devType;  // Each type gets its own unique SourceId channel
		}
		Button navigation(unsigned k, unsigned mod) {
			if (!mod) {
				if (k == SDLK_UP) return GENERIC_UP;
				if (k == SDLK_DOWN) return GENERIC_DOWN;
				if (k == SDLK_LEFT) return GENERIC_LEFT;
				if (k == SDLK_RIGHT) return GENERIC_RIGHT;
				if (k == SDLK_RETURN || k == SDLK_KP_ENTER) return GENERIC_START;
				if (k == SDLK_ESCAPE) return GENERIC_CANCEL;
				if (k == SDLK_PAGEUP) return GENERIC_MOREUP;
				if (k == SDLK_PAGEDOWN) return GENERIC_MOREDOWN;
				if (k == SDLK_PAUSE) return GENERIC_PAUSE;
			}
			else if (mod == KMOD_LCTRL) {
				if (k == SDLK_UP) return GENERIC_VOLUME_UP;
				if (k == SDLK_DOWN) return GENERIC_VOLUME_DOWN;
				if (k == SDLK_p) return GENERIC_PAUSE;
			}
			return GENERIC_UNASSIGNED;
		}
	};

	void Hardware::enableKeyboardInstruments(bool state) { g_enableInstruments = state; }
	Hardware::ptr constructKeyboard() { return Hardware::ptr(new Keyboard()); }
	
}


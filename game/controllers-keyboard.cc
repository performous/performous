#include "controllers.hh"

#include <set>

namespace {
	bool g_enableInstruments = false;
}

namespace input {
	class Keyboard: public Hardware {
		std::set<unsigned> m_pressed;
		bool m_guitar, m_keytar, m_drumkit, m_dancepad;
		/// Set the value of an instrument bool from a config variable and return a log message snippet if value was changed.
		static std::string setMode(bool& var, std::string const& name) {
			bool value = config["game/keyboard_" + name].b();
			if (var == value) return std::string();
			var = value;
			return " " + name + (value ? " enabled." : " disabled.");
		}
	public:
		Keyboard(): m_guitar(), m_keytar(), m_drumkit(), m_dancepad() {}
		bool process(Event& event, SDL_Event const& sdlEv) override {
			if (sdlEv.type != SDL_KEYDOWN && sdlEv.type != SDL_KEYUP) return false;
			// Switch modes only when no keys are pressed (avoids buttons getting stuck on mode change)
			if (m_pressed.empty()) {
				std::string msg;
				if (g_enableInstruments) {
					msg += setMode(m_guitar, "guitar");
					msg += setMode(m_keytar, "keytar");
					msg += setMode(m_drumkit, "drumkit");
					msg += setMode(m_dancepad, "dancepad");
				} else if (m_guitar || m_keytar || m_drumkit || m_dancepad) {
					msg = " all instruments disabled.";
					m_guitar = m_keytar = m_drumkit = m_dancepad = false;
				}
				if (!msg.empty()) std::clog << "controller-keyboard/info: Mode change:" + msg << std::endl;
			}
			// Keep track of pressed keys
			{
				unsigned pressedId = sdlEv.key.keysym.sym << 16 | sdlEv.key.keysym.sym;
				if (sdlEv.type == SDL_KEYDOWN) m_pressed.insert(pressedId);
				else m_pressed.erase(pressedId);
			}
			// Convert SDL event into controller Event
			event.source = SourceId(SOURCETYPE_KEYBOARD, 0);  // FIXME! make the device ID zero because in SDL2 it ain't zero!!
			event.hw = sdlEv.key.keysym.sym;
			event.value = (sdlEv.type == SDL_KEYDOWN ? 1.0 : 0.0);
			// Get the modifier keys that we actually use as modifiers
			unsigned mod = sdlEv.key.keysym.mod & (KMOD_LCTRL | KMOD_LALT);
			// Map to keyboard instruments (sets event.button if matching)
			if (!mod) mapping(event);
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
					if (!m_guitar) return;
					event.devType = DEVTYPE_GUITAR;
					break;

				// Keytar on keyboard
				case SDLK_F12: button++;
				case SDLK_F11: button++;
				case SDLK_F10: button++;
				case SDLK_F9: button++;
				case SDLK_F8: button++;
				case SDLK_F7:
					if (!m_keytar) return;
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
					if (!m_drumkit) return;
					event.devType = DEVTYPE_DRUMS;
					break;

				// Dance on keypad
				case SDLK_KP_9: button++;
				case SDLK_KP_7: button++;
				case SDLK_KP_3: button++;
				case SDLK_KP_1: button++;
				case SDLK_KP_6: case SDLK_RIGHT: button++;
				case SDLK_KP_8: case SDLK_UP: button++;
				case SDLK_KP_2: case SDLK_DOWN: case SDLK_KP_5: button++;
				case SDLK_KP_4: case SDLK_LEFT:
					if (!m_dancepad) return;
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


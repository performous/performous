#include "controllers.hh"
#include "platform.hh"

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
			event.source = SourceId(SOURCETYPE_KEYBOARD, 0);  // FIXME! cmake the device ID zero because in SDL2 it ain't zero!!
			event.hw = sdlEv.key.keysym.scancode;
			event.value = (sdlEv.type == SDL_KEYDOWN ? 1.0 : 0.0);
			// Get the modifier keys that we actually use as modifiers
			unsigned mod = sdlEv.key.keysym.mod & (Platform::shortcutModifier(false) | KMOD_LALT);
			// Map to keyboard instruments (sets event.button if matching)
			if (!mod) mapping(event);
			// Map to menu navigation
			if (event.button == GENERIC_UNASSIGNED) event.button = navigation(sdlEv.key.keysym.scancode, mod);
			return event.button != GENERIC_UNASSIGNED;
		}
		void mapping(Event& event) {
			unsigned button = 0;
			switch (event.hw) {
				// Guitar on keyboard
				case SDL_SCANCODE_BACKSPACE: button = GUITAR_WHAMMY; goto guitar_process;
				case SDL_SCANCODE_RCTRL: case SDL_SCANCODE_RALT: button = GUITAR_GODMODE; goto guitar_process;
				case SDL_SCANCODE_RSHIFT: button = GUITAR_PICK_UP; goto guitar_process;
				case SDL_SCANCODE_RETURN: case SDL_SCANCODE_KP_ENTER: button = GUITAR_PICK_DOWN; goto guitar_process;
				case SDL_SCANCODE_F5: case SDL_SCANCODE_5: case SDL_SCANCODE_B: button++;
				case SDL_SCANCODE_F4: case SDL_SCANCODE_4: case SDL_SCANCODE_V: button++;
				case SDL_SCANCODE_F3: case SDL_SCANCODE_3: case SDL_SCANCODE_C: button++;
				case SDL_SCANCODE_F2: case SDL_SCANCODE_2: case SDL_SCANCODE_X: button++;
				case SDL_SCANCODE_F1: case SDL_SCANCODE_1: case SDL_SCANCODE_Z:
				guitar_process:
					if (!m_guitar) return;
					event.devType = DEVTYPE_GUITAR;
					break;

				// Keytar on keyboard
				case SDL_SCANCODE_F12: button++;
				case SDL_SCANCODE_F11: button++;
				case SDL_SCANCODE_F10: button++;
				case SDL_SCANCODE_F9: button++;
				case SDL_SCANCODE_F8: button++;
				case SDL_SCANCODE_F7:
					if (!m_keytar) return;
					event.devType = DEVTYPE_KEYTAR;
					break;

				// Drums on keyboard
				case SDL_SCANCODE_8: button = DRUMS_YELLOW_CYMBAL; goto drum_process;
				case SDL_SCANCODE_9: button = DRUMS_BLUE_CYMBAL; goto drum_process;
				case SDL_SCANCODE_0: button = DRUMS_GREEN_CYMBAL; goto drum_process;
				case SDL_SCANCODE_U: button = DRUMS_RED; goto drum_process;
				case SDL_SCANCODE_I: button = DRUMS_YELLOW_TOM; goto drum_process;
				case SDL_SCANCODE_O: button = DRUMS_BLUE_TOM; goto drum_process;
				case SDL_SCANCODE_P: button = DRUMS_GREEN_TOM; goto drum_process;
				case SDL_SCANCODE_SPACE:
					drum_process:
					if (!m_drumkit) return;
					event.devType = DEVTYPE_DRUMS;
					break;

				// Dance on keypad
				case SDL_SCANCODE_KP_9: button++;
				case SDL_SCANCODE_KP_7: button++;
				case SDL_SCANCODE_KP_3: button++;
				case SDL_SCANCODE_KP_1: button++;
				case SDL_SCANCODE_KP_6: case SDL_SCANCODE_RIGHT: button++;
				case SDL_SCANCODE_KP_8: case SDL_SCANCODE_UP: button++;
				case SDL_SCANCODE_KP_2: case SDL_SCANCODE_DOWN: case SDL_SCANCODE_KP_5: button++;
				case SDL_SCANCODE_KP_4: case SDL_SCANCODE_LEFT:
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
				if (k == SDL_SCANCODE_UP) return GENERIC_UP;
				if (k == SDL_SCANCODE_DOWN) return GENERIC_DOWN;
				if (k == SDL_SCANCODE_LEFT) return GENERIC_LEFT;
				if (k == SDL_SCANCODE_RIGHT) return GENERIC_RIGHT;
				if (k == SDL_SCANCODE_RETURN || k == SDL_SCANCODE_KP_ENTER) return GENERIC_START;
				if (k == SDL_SCANCODE_ESCAPE) return GENERIC_CANCEL;
				if (k == SDL_SCANCODE_PAGEUP) return GENERIC_MOREUP;
				if (k == SDL_SCANCODE_PAGEDOWN) return GENERIC_MOREDOWN;
				if (k == SDL_SCANCODE_PAUSE) return GENERIC_PAUSE;
			}
			else if (mod == Platform::shortcutModifier(false)) {
				if (k == SDL_SCANCODE_UP) return GENERIC_VOLUME_UP;
				if (k == SDL_SCANCODE_DOWN) return GENERIC_VOLUME_DOWN;
				if (k == SDL_SCANCODE_P) return GENERIC_PAUSE;
			}
			return GENERIC_UNASSIGNED;
		}
	};

	void Hardware::enableKeyboardInstruments(bool state) { g_enableInstruments = state; }
	Hardware::ptr constructKeyboard() { return Hardware::ptr(new Keyboard()); }
	
}


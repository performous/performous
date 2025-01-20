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
		unsigned m_mod;
		/// Set the value of an instrument bool from a config variable and return a log message snippet if value was changed.
		static std::string setMode(bool& var, std::string const& name) {
			bool value = config["game/keyboard_" + name].b();
			if (var == value) return std::string();
			var = value;
			return " " + name + (value ? " enabled." : " disabled.");
		}
	public:
		Keyboard(): m_guitar(), m_keytar(), m_drumkit(), m_dancepad(), m_mod() {}
		bool process(Event& event, SDL_Event const& sdlEv) override {
			if (sdlEv.type != SDL_KEYDOWN && sdlEv.type != SDL_KEYUP) return false;
			// Switch modes and update m_mod only when no buttons are pressed
			// (avoids buttons getting stuck on mode change)
			if (m_pressed.empty()) {
				// The mods that we consider modifiers here: only Ctrl/Cmd and Alt
				m_mod = sdlEv.key.keysym.mod & (Platform::shortcutModifier() | KMOD_LALT);
				// Enable/disable keyboard instruments based on current config
				std::string msg;
				if (g_enableInstruments) {
					msg.append(setMode(m_guitar, "guitar"));
					msg.append(setMode(m_keytar, "keytar"));
					msg.append(setMode(m_drumkit, "drumkit"));
					msg.append(setMode(m_dancepad, "dancepad"));
				} else if (m_guitar || m_keytar || m_drumkit || m_dancepad) {
					msg = " all instruments disabled.";
					m_guitar = m_keytar = m_drumkit = m_dancepad = false;
				}
				if (!msg.empty()) SpdLogger::info(LogSystem::CONTROLLERS, "Keyboard mode change: {}.", msg);
			}
			// Convert SDL event into controller Event
			event.source = SourceId(SourceType::KEYBOARD, 0);
			event.hw = sdlEv.key.keysym.scancode;
			event.value = (sdlEv.type == SDL_KEYDOWN ? 1.0 : 0.0);
			// Map to keyboard instruments (sets event.button if matching)
			mapping(event);
			// Map to menu navigation
			if (event.button == ButtonId::GENERIC_UNASSIGNED) event.button = navigation(event.hw);
			if (event.button == ButtonId::GENERIC_UNASSIGNED) return false;
			// Keep track of pressed buttons
			if (event.value != 0.0) m_pressed.insert(to_underlying(event.button.id));
			else m_pressed.erase(to_underlying(event.button.id));
			return true;
		}
		void mapping(Event& event) {
			if (m_mod) return;
			unsigned button = 0;
			switch (event.hw) {
				// Guitar on keyboard
				case SDL_SCANCODE_BACKSPACE: button = to_underlying(ButtonId::GUITAR_WHAMMY); goto guitar_process;
				case SDL_SCANCODE_BACKSLASH: case SDL_SCANCODE_SLASH: button = to_underlying(ButtonId::GUITAR_GODMODE); goto guitar_process;
				case SDL_SCANCODE_RSHIFT: button = to_underlying(ButtonId::GUITAR_PICK_UP); goto guitar_process;
				case SDL_SCANCODE_RETURN: case SDL_SCANCODE_KP_ENTER: button = to_underlying(ButtonId::GUITAR_PICK_DOWN); goto guitar_process;
				case SDL_SCANCODE_F5: case SDL_SCANCODE_5: case SDL_SCANCODE_B: button++; [[fallthrough]];
				case SDL_SCANCODE_F4: case SDL_SCANCODE_4: case SDL_SCANCODE_V: button++; [[fallthrough]];
				case SDL_SCANCODE_F3: case SDL_SCANCODE_3: case SDL_SCANCODE_C: button++; [[fallthrough]];
				case SDL_SCANCODE_F2: case SDL_SCANCODE_2: case SDL_SCANCODE_X: button++; [[fallthrough]];
				case SDL_SCANCODE_F1: case SDL_SCANCODE_1: case SDL_SCANCODE_Z:
				guitar_process:
					if (!m_guitar) return;
					event.devType = DevType::GUITAR;
					break;

				// Keytar on keyboard
				case SDL_SCANCODE_F12: button++; [[fallthrough]];
				case SDL_SCANCODE_F11: button++; [[fallthrough]];
				case SDL_SCANCODE_F10: button++; [[fallthrough]];
				case SDL_SCANCODE_F9: button++; [[fallthrough]];
				case SDL_SCANCODE_F8: button++; [[fallthrough]];
				case SDL_SCANCODE_F7:
					if (!m_keytar) return;
					event.devType = DevType::KEYTAR;
					break;

				// Drums on keyboard
				case SDL_SCANCODE_8: button = to_underlying(ButtonId::DRUMS_YELLOW_CYMBAL); goto drum_process;
				case SDL_SCANCODE_9: button = to_underlying(ButtonId::DRUMS_BLUE_CYMBAL); goto drum_process;
				case SDL_SCANCODE_0: button = to_underlying(ButtonId::DRUMS_GREEN_CYMBAL); goto drum_process;
				case SDL_SCANCODE_U: button = to_underlying(ButtonId::DRUMS_RED); goto drum_process;
				case SDL_SCANCODE_I: button = to_underlying(ButtonId::DRUMS_YELLOW_TOM); goto drum_process;
				case SDL_SCANCODE_O: button = to_underlying(ButtonId::DRUMS_BLUE_TOM); goto drum_process;
				case SDL_SCANCODE_P: button = to_underlying(ButtonId::DRUMS_GREEN_TOM); goto drum_process;
				case SDL_SCANCODE_SPACE:
					drum_process:
					if (!m_drumkit) return;
					event.devType = DevType::DRUMS;
					break;

				// Dance on keypad
				case SDL_SCANCODE_KP_9: button++; [[fallthrough]];
				case SDL_SCANCODE_KP_7: button++; [[fallthrough]];
				case SDL_SCANCODE_KP_3: button++; [[fallthrough]];
				case SDL_SCANCODE_KP_1: button++; [[fallthrough]];
				case SDL_SCANCODE_KP_6: case SDL_SCANCODE_RIGHT: button++; [[fallthrough]];
				case SDL_SCANCODE_KP_8: case SDL_SCANCODE_UP: button++; [[fallthrough]];
				case SDL_SCANCODE_KP_2: case SDL_SCANCODE_DOWN: case SDL_SCANCODE_KP_5: button++; [[fallthrough]];
				case SDL_SCANCODE_KP_4: case SDL_SCANCODE_LEFT:
					if (!m_dancepad) return;
					event.devType = DevType::DANCEPAD;
					break;

				default: return;
			}
			event.button = ButtonId(button);
			event.source.channel = to_underlying(event.devType); // Each type gets its own unique SourceId channel
		}
		Button navigation(unsigned k) {
			if (!m_mod) {
				if (k == SDL_SCANCODE_UP) return ButtonId::GENERIC_UP;
				if (k == SDL_SCANCODE_DOWN) return ButtonId::GENERIC_DOWN;
				if (k == SDL_SCANCODE_LEFT) return ButtonId::GENERIC_LEFT;
				if (k == SDL_SCANCODE_RIGHT) return ButtonId::GENERIC_RIGHT;
				if (k == SDL_SCANCODE_RETURN || k == SDL_SCANCODE_KP_ENTER) return ButtonId::GENERIC_START;
				if (k == SDL_SCANCODE_ESCAPE) return ButtonId::GENERIC_CANCEL;
				if (k == SDL_SCANCODE_PAGEUP) return ButtonId::GENERIC_MOREUP;
				if (k == SDL_SCANCODE_PAGEDOWN) return ButtonId::GENERIC_MOREDOWN;
				if (k == SDL_SCANCODE_PAUSE) return ButtonId::GENERIC_PAUSE;
			}
			else if (m_mod == Platform::shortcutModifier()) {
				if (k == SDL_SCANCODE_UP) return ButtonId::GENERIC_VOLUME_UP;
				if (k == SDL_SCANCODE_DOWN) return ButtonId::GENERIC_VOLUME_DOWN;
				if (k == SDL_SCANCODE_P) return ButtonId::GENERIC_PAUSE;
			}
			return ButtonId::GENERIC_UNASSIGNED;
		}
	};

	void Hardware::enableKeyboardInstruments(bool state) { g_enableInstruments = state; }
	Hardware::ptr constructKeyboard() { return Hardware::ptr(new Keyboard()); }

}

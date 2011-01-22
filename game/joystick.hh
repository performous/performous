#pragma once

#include <climits>
#include <deque>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <boost/noncopyable.hpp>

#include "SDL_events.h"
#include "SDL_joystick.h"

#include "xtime.hh"
#include "configuration.hh"

#ifdef USE_PORTMIDI
#include "portmidi.hh"
#endif

namespace input {
	enum DevType { GUITAR, DRUMS, KEYBOARD, DANCEPAD };
	enum NavButton { NONE, UP, DOWN, LEFT, RIGHT, START, SELECT, CANCEL, PAUSE, MOREUP, MOREDOWN, VOLUME_UP, VOLUME_DOWN };

	static const std::size_t BUTTONS = 10;
	// Guitar buttons
	static const int GREEN_FRET_BUTTON = 0;
	static const int RED_FRET_BUTTON = 1;
	static const int YELLOW_FRET_BUTTON = 2;
	static const int BLUE_FRET_BUTTON = 3;
	static const int ORANGE_FRET_BUTTON = 4;
	static const int GODMODE_BUTTON = 5;
	static const int WHAMMY_BUTTON = 6;
	// Drums buttons
	static const int KICK_BUTTON = 0;
	static const int RED_TOM_BUTTON = 1;
	static const int YELLOW_TOM_BUTTON = 2;
	static const int BLUE_TOM_BUTTON = 3;
	static const int GREEN_TOM_BUTTON = 4;
	static const int ORANGE_TOM_BUTTON = 5;
	// Keyboard buttons
	static const int C_BUTTON = 0;
	static const int D_BUTTON = 1;
	static const int E_BUTTON = 2;
	static const int F_BUTTON = 3;
	static const int G_BUTTON = 4;
	//static const int GODMODE_BUTTON = 5;
	//static const int WHAMMY_BUTTON = 6;
	// Dance buttons
	static const int LEFT_DANCE_BUTTON = 0;
	static const int DOWN_DANCE_BUTTON = 1;
	static const int UP_DANCE_BUTTON = 2;
	static const int RIGHT_DANCE_BUTTON = 3;
	static const int DOWN_LEFT_DANCE_BUTTON = 4;
	static const int DOWN_RIGHT_DANCE_BUTTON = 5;
	static const int UP_LEFT_DANCE_BUTTON = 6;
	static const int UP_RIGHT_DANCE_BUTTON = 7;
	// Global buttons
	static const int SELECT_BUTTON = 8;
	static const int START_BUTTON = 9;

	struct Event {
		enum Type { PRESS, RELEASE, PICK };
		Type type;
		int button; // Translated button number for press/release events. 0 for pick down, 1 for pick up (NOTE: these are NOT pick press/release events but rather different directions)
		bool pressed[BUTTONS]; // All events tell the button state right after the event happened
		NavButton nav; // Event translated to NavButton
		// More stuff later, when it is actually used
		boost::xtime time;
	};

	struct Instrument {
		Instrument(std::string _name, input::DevType _type, std::vector<int> _mapping) : name(_name), type(_type), mapping(_mapping) {};
		std::string name;
		input::DevType type;
		std::string description;
		std::string match;
		std::vector<int> mapping;
	};
	typedef std::map<std::string, Instrument> Instruments;

	namespace detail {
		static unsigned int KEYBOARD_ID = UINT_MAX;
		static unsigned int KEYBOARD_ID2 = KEYBOARD_ID-1;
		static unsigned int KEYBOARD_ID3 = KEYBOARD_ID-2;
		static unsigned int KEYBOARD_ID4 = KEYBOARD_ID-3; // Four ids needed for keyboard guitar/drumkit/dancepad/keyboard

		class InputDevPrivate {
		  public:
			InputDevPrivate(const Instrument& _instrument) : m_assigned(false), m_instrument(_instrument) {
				for(unsigned int i = 0 ; i < BUTTONS ; i++) {
					m_pressed[i] = false;
				}
			};
			bool tryPoll(Event& _event) {
				if (m_events.empty()) return false;
				_event = m_events.front();
				m_events.pop_front();
				return true;
			};
			void addEvent(Event _event) {
				// only add event if the device is assigned
				if (m_assigned) m_events.push_back(_event);
				/*
				if (_event.type == Event::PICK)
					std::cout << "PICK event " << _event.button << std::endl;
				if (_event.type == Event::PRESS)
					std::cout << "PRESS event " << _event.button << std::endl;
				if (_event.type == Event::RELEASE)
					std::cout << "RELEASE event " << _event.button << std::endl;
				*/
				// always keep track of button status
				for( unsigned int i = 0 ; i < BUTTONS ; ++i) {
					m_pressed[i] = _event.pressed[i];
				}
			};
			void clearEvents() {m_events.clear();};
			void assign() {m_assigned = true;};
			void unassign() {m_assigned = false; clearEvents();};
			bool assigned() {return m_assigned;};
			bool pressed(int _button) {return m_pressed[_button];};
			std::string name() {return m_instrument.name;};
			bool type_match(DevType _type) {
				return _type == m_instrument.type;
			};
			int buttonFromSDL(unsigned int sdl_button) {
				try {
					return m_instrument.mapping.at(sdl_button);
				} catch(...) {
					return -1;
				}
			}
		  private:
			std::deque<Event> m_events;
			bool m_assigned;
			bool m_pressed[BUTTONS];
			Instrument m_instrument;
		};

		typedef std::map<unsigned int,InputDevPrivate> InputDevs;
		extern InputDevs devices;
	}

	NavButton getNav(SDL_Event const &e);

	struct NoDevError: std::runtime_error {
		NoDevError(): runtime_error("No instrument of the requested type was available") {}
	};

	class InputDev: boost::noncopyable {
	  public:
		// First gives a correct instrument type
		// Then gives an unknown instrument type
		// Finally throw an exception if only wrong (or none) instrument are available
		InputDev(DevType _type): m_dev_type(_type) {
			for (detail::InputDevs::iterator it = detail::devices.begin() ; it != detail::devices.end() ; ++it) {
				if (it->first == detail::KEYBOARD_ID && !config["game/keyboard_guitar"].b()) continue;
				if (it->first == detail::KEYBOARD_ID2 && !config["game/keyboard_drumkit"].b()) continue;
				if (it->first == detail::KEYBOARD_ID3 && !config["game/keyboard_dancepad"].b()) continue;
				if (it->first == detail::KEYBOARD_ID4 && !config["game/keyboard_keyboard"].b()) continue;
				if (!it->second.assigned() && it->second.type_match(_type)) {
					m_device_id = it->first;
					it->second.assign();
					return;
				}
			}
			throw NoDevError();
		};
		~InputDev() {
			detail::devices.find(m_device_id)->second.unassign();
		};
		bool tryPoll(Event& _e) { return detail::devices.find(m_device_id)->second.tryPoll(_e); };
		void addEvent(Event _e) { detail::devices.find(m_device_id)->second.addEvent(_e); };
		bool pressed(int _button) { return detail::devices.find(m_device_id)->second.pressed(_button); }; // Current state
		bool isKeyboard() const { return (m_device_id == detail::KEYBOARD_ID || m_device_id == detail::KEYBOARD_ID2 || m_device_id == detail::KEYBOARD_ID3 || m_device_id == detail::KEYBOARD_ID4); };
		DevType getDevType() const { return m_dev_type; }
	  private:
		unsigned int m_device_id; // should be some kind of reference
		DevType m_dev_type;
	};

	namespace SDL {
		typedef std::map<unsigned int,SDL_Joystick*> SDL_devices;
		extern SDL_devices sdl_devices;
		void init_devices();
		// Initialize all event stuffs
		void init();
		// Returns true if event is taken, feed an InputDev by transforming SDL_Event into Event
		bool pushEvent(SDL_Event);
	}

#ifdef USE_PORTMIDI
	// Warning: MidiDrums should be instanciated after Window (that load joysticks)
	class MidiDrums {
	public:
		static bool enabled() { return true; }
		MidiDrums();
		void process();
	private:
		pm::Input stream;
		unsigned int devnum;
		Event event;
		typedef std::map<unsigned, unsigned> Map;
		Map map;
	};
#else
	class MidiDrums {
	public:
		static bool enabled() { return false; }
		MidiDrums() {};
		void process() {};
	private:
	};
#endif

}


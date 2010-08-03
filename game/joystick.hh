#pragma once

#include <climits>
#include <deque>
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
	enum DevType { GUITAR, DRUMS, DANCEPAD };
	enum NavButton { NONE, UP, DOWN, LEFT, RIGHT, START, SELECT, CANCEL, PAUSE, MOREUP, MOREDOWN, VOLUME_UP, VOLUME_DOWN };

	static const std::size_t BUTTONS = 10;
	static const int STARPOWER_BUTTON = 5;

	struct Event {
		enum Type { PRESS, RELEASE, PICK, WHAMMY };
		Type type;
		int button; // Translated button number for press/release events. 0 for pick down, 1 for pick up (NOTE: these are NOT pick press/release events but rather different directions)
		bool pressed[BUTTONS]; // All events tell the button state right after the event happened
		// More stuff later, when it is actually used
		boost::xtime time;
	};

	namespace detail {
		enum Type { GUITAR_RB_PS3, DRUMS_RB_PS3, GUITAR_RB_XB360, DRUMS_RB_XB360,
		  GUITAR_GH, GUITAR_GH_XPLORER, GUITAR_HAMA_PS2, DRUMS_GH, DRUMS_MIDI, DANCEPAD_TIGERGAME, DANCEPAD_GENERIC, DANCEPAD_EMS2, DANCEPAD_2TECH };
		static unsigned int KEYBOARD_ID = UINT_MAX;
		static unsigned int KEYBOARD_ID2 = KEYBOARD_ID-1;
		static unsigned int KEYBOARD_ID3 = KEYBOARD_ID-2; // Three ids needed for keyboard guitar/drumkit/dancepad

		class InputDevPrivate {
		  public:
			InputDevPrivate(Type _type) : m_assigned(false), m_type(_type) {
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
			Type type() {return m_type;};
			bool type_match(DevType _type) {
				switch (m_type) {
				case GUITAR_GH:
				case GUITAR_GH_XPLORER:
				case GUITAR_HAMA_PS2:
				case GUITAR_RB_PS3:
				case GUITAR_RB_XB360:
					return _type == GUITAR;
				case DRUMS_GH:
				case DRUMS_MIDI:
				case DRUMS_RB_PS3:
				case DRUMS_RB_XB360:
					return _type == DRUMS;
				case DANCEPAD_GENERIC:
				case DANCEPAD_EMS2:
				case DANCEPAD_TIGERGAME:
				case DANCEPAD_2TECH:
					return _type == DANCEPAD;
				}
				throw std::logic_error("Unhandled DevType");
			};
		  private:
			std::deque<Event> m_events;
			bool m_assigned;
			bool m_pressed[BUTTONS];
			Type m_type;
		};

		typedef std::map<unsigned int,InputDevPrivate> InputDevs;
		extern InputDevs devices;
	}

	int buttonFromSDL(detail::Type _type, unsigned int _sdl_button);
	NavButton getNav(SDL_Event const &e);

	struct NoDevError: std::runtime_error {
		NoDevError(): runtime_error("No instrument of the requested type was available") {}
	};

	class InputDev: boost::noncopyable {
	  public:
		// First gives a correct instrument type
		// Then gives an unknown instrument type
		// Finally throw an exception if only wrong (or none) instrument are available
		InputDev(DevType _type) {
			for (detail::InputDevs::iterator it = detail::devices.begin() ; it != detail::devices.end() ; ++it) {
				if (it->first == detail::KEYBOARD_ID && !config["game/keyboard_guitar"].b()) continue;
				if (it->first == detail::KEYBOARD_ID2 && !config["game/keyboard_drumkit"].b()) continue;
				if (it->first == detail::KEYBOARD_ID3 && !config["game/keyboard_dancepad"].b()) continue;
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
		bool isKeyboard() const { return (m_device_id == detail::KEYBOARD_ID || m_device_id == detail::KEYBOARD_ID2 || m_device_id == detail::KEYBOARD_ID3); };
	  private:
		unsigned int m_device_id; // should be some kind of reference
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


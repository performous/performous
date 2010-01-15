#pragma once

#include <climits>
#include <deque>
#include <iostream>
#include <stdexcept>

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

	namespace Private {
		enum Type { GUITAR_RB_PS3, DRUMS_RB_PS3, GUITAR_RB_XB360, DRUMS_RB_XB360,
		  GUITAR_GH, GUITAR_GH_XPLORER, DRUMS_GH, DRUMS_MIDI, DANCEPAD_TIGERGAME, DANCEPAD_GENERIC };
		static unsigned int KEYBOARD_ID = UINT_MAX;
		static unsigned int KEYBOARD_ID2 = KEYBOARD_ID-1;
		static unsigned int KEYBOARD_ID3 = KEYBOARD_ID-2; // Three ids needed for keyboard guitar/drumkit/dancepad

		class InputDevPrivate {
		  public:
			InputDevPrivate() : m_assigned(false), m_type(input::Private::DRUMS_GH) {
				for(unsigned int i = 0 ; i < BUTTONS ; i++) {
					m_pressed[i] = false;
				}
			};
			InputDevPrivate(input::Private::Type _type) : m_assigned(false), m_type(_type) {
				for(unsigned int i = 0 ; i < BUTTONS ; i++) {
					m_pressed[i] = false;
				}
			};
			bool tryPoll(Event& _event) {
				if( m_events.empty() ) return false;
				_event = m_events.front();
				m_events.pop_front();
				return true;
			};
			void addEvent(Event _event) {
				// only add event if the device is assigned
				if( m_assigned ) m_events.push_back(_event);
				/*
				if( _event.type == input::Event::PICK )
					std::cout << "PICK event " << _event.button << std::endl;
				if( _event.type == input::Event::PRESS )
					std::cout << "PRESS event " << _event.button << std::endl;
				if( _event.type == input::Event::RELEASE )
					std::cout << "RELEASE event " << _event.button << std::endl;
				*/
				// always keep track of button status
				for( unsigned int i = 0 ; i < BUTTONS ; ++i ) {
					m_pressed[i] = _event.pressed[i];
				}
			};
			void clearEvents() {m_events.clear();};
			void assign() {m_assigned = true;};
			void unassign() {m_assigned = false; clearEvents();};
			bool assigned() {return m_assigned;};
			bool pressed(int _button) {return m_pressed[_button];};
			input::Private::Type type() {return m_type;};
			bool type_match( input::DevType _type) {
				if( _type == input::GUITAR &&
				  (m_type == input::Private::GUITAR_GH || m_type == input::Private::GUITAR_GH_XPLORER
				  || m_type == input::Private::GUITAR_RB_PS3 || m_type == input::Private::GUITAR_RB_XB360) ) {
					return true;
				}
				else if( _type == input::DRUMS &&
				  (m_type == input::Private::DRUMS_GH || m_type == input::Private::DRUMS_MIDI
				  || m_type == input::Private::DRUMS_RB_PS3 || m_type == input::Private::DRUMS_RB_XB360) ) {
					return true;
				}
				else if( _type == input::DANCEPAD &&
				  (m_type == input::Private::DANCEPAD_GENERIC || m_type == input::Private::DANCEPAD_TIGERGAME) ) {
					return true;
				}
				else {
					return false;
				}
			};
		  private:
			std::deque<Event> m_events;
			bool m_assigned;
			bool m_pressed[BUTTONS];
			input::Private::Type m_type;
		};

		typedef std::map<unsigned int,InputDevPrivate> InputDevs;
		extern InputDevs devices;
	}

	int buttonFromSDL(input::Private::Type _type, unsigned int _sdl_button);
	NavButton getNav(SDL_Event const &e);

	class InputDev {
	  public:
		// First gives a correct instrument type
		// Then gives an unknown instrument type
		// Finally throw an exception if only wrong (or none) instrument are available
		InputDev(input::DevType _type) {
			using namespace input::Private;
			if( _type == input::DRUMS )
				std::cout << "Request acquiring DRUM" << std::endl;
			if( _type == input::GUITAR )
				std::cout << "Request acquiring GUITAR" << std::endl;
			if( _type == input::DANCEPAD )
				std::cout << "Request acquiring DANCEPAD" << std::endl;
			if( devices.size() == 0 ) throw std::runtime_error("No InputDev available");
			for(InputDevs::iterator it = devices.begin() ; it != devices.end() ; ++it) {
				if( it->first == input::Private::KEYBOARD_ID && !config["game/keyboard_guitar"].b() )
					continue;
				if( it->first == input::Private::KEYBOARD_ID2 && !config["game/keyboard_drumkit"].b() )
					continue;
				if( it->first == input::Private::KEYBOARD_ID3 && !config["game/keyboard_dancepad"].b() )
					continue;
				if( !it->second.assigned() && it->second.type_match(_type) ) {
					std::cout << "Found @" << it->first << std::endl;
					m_device_id = it->first;
					it->second.assign();
					return;
				}
			}
			std::cout << "No InputDev was found!" << std::endl;
			throw std::runtime_error("No matching instrument available");
		};
		~InputDev() {
			using namespace input::Private;
			// we assume find will success
			devices.find(m_device_id)->second.unassign();
		};
		bool tryPoll(Event& _e) {return input::Private::devices.find(m_device_id)->second.tryPoll(_e);};
		void addEvent(Event _e) {input::Private::devices.find(m_device_id)->second.addEvent(_e);};
		bool pressed(int _button) {return input::Private::devices.find(m_device_id)->second.pressed(_button);}; // Current state
		bool isKeyboard() const {return (m_device_id == input::Private::KEYBOARD_ID || m_device_id == input::Private::KEYBOARD_ID2 || m_device_id == input::Private::KEYBOARD_ID3);};
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
		MidiDrums(int devId);
		void process();
	  private:
		pm::Input stream;
		unsigned int devnum;
		Event event;
		typedef std::map<unsigned, unsigned> Map;
		Map map;
	};
#endif

};


#pragma once

#include "SDL_events.h"
#include "SDL_joystick.h"
#include "audio.hh"

namespace input {
	enum DevType { GUITAR, DRUMS };

	static const std::size_t BUTTONS = 6;

	typedef std::map<unsigned int,SDL_Joystick*> SDL_devices;
	extern SDL_devices sdl_devices;
	void init_devices();

	struct Event {
		enum Type { PRESS, RELEASE, PICK };
		Type type;
		int button; // Translated button number for press/release events. 0 for pick down, 1 for pick up (NOTE: these are NOT pick press/release events but rather different directions)
		bool pressed[BUTTONS]; // All events tell the button state right after the event happened
		// More stuff later, when it is actually used
	};
	
	namespace Private {
		enum Type {UNKNOWN, GUITAR_RB, DRUM_RB, GUITAR_GH, DRUM_GH};
		class InputDevPrivate {
		  public:
			InputDevPrivate(input::Private::Type _type = input::Private::UNKNOWN) : m_assigned(false), m_type(_type) {};
			bool tryPoll(Event& _event) {
				if( m_events.empty() ) return false;
				_event = m_events.front();
				m_events.pop_front();
				return true;
			};
			void addEvent(Event _event) {
				// only add event if the device is assigned
				if( m_assigned ) m_events.push_back(_event);
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
					(m_type == input::Private::GUITAR_GH || m_type == input::Private::GUITAR_RB) ) {
					return true;
				} else if( _type == input::DRUMS &&
					(m_type == input::Private::DRUM_GH || m_type == input::Private::DRUM_RB) ) {
					return true;
				} else {
					return false;
				}
			};
			void setType(input::DevType _type) {
				if( _type == input::GUITAR ) {
					m_type = input::Private::GUITAR_RB;
				} else if( _type == input::DRUMS ) {
					m_type = input::Private::DRUM_RB;
				} else {
					m_type = input::Private::UNKNOWN;
				}
				init_devices();
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

	class InputDev {
	  public:
		// First gives a correct instrument type
		// Then gives an unknown instrument type
		// Finally throw an exception if only wrong (or none) instrument are available
		InputDev(input::DevType _type) {
			using namespace input::Private;
			if( _type == input::DRUMS )
				std::cout << "Request aquiring DRUM" << std::endl;
			if( _type == input::GUITAR )
				std::cout << "Request aquiring GUITAR" << std::endl;
			if( devices.size() == 0 ) throw std::runtime_error("No InputDev available");
			for(InputDevs::iterator it = devices.begin() ; it != devices.end() ; ++it) {
				if( !it->second.assigned() && it->second.type_match(_type) ) {
					std::cout << "Found @" << it->first << std::endl;
					m_device_id = it->first;
					it->second.assign();
					return;
				}
			}
			std::cout << "Not Found" << std::endl;
			throw std::runtime_error("No matching instrument available");
		};
		~InputDev() {
			std::cout << "Releasing @" << m_device_id << std::endl;
			input::Private::devices[m_device_id].unassign();
		};
		bool tryPoll(Event& _e) {return input::Private::devices[m_device_id].tryPoll(_e);};
		void addEvent(Event _e) {input::Private::devices[m_device_id].addEvent(_e);};
		bool pressed(int _button) {return input::Private::devices[m_device_id].pressed(_button);}; // Current state
	  private:
		unsigned int m_device_id; // should be some kind of reference
	};

	// Initialize all event stuffs
	void init();
	// Returns true if event is taken, feed an InputDev by transforming SDL_Event into Event
	bool pushEvent(SDL_Event);
};


#pragma once

#include "SDL_events.h"
#include "SDL_joystick.h"
#include "audio.hh"

#define PS3_DRUM_CONTROLLER_BLUE   0
#define PS3_DRUM_CONTROLLER_GREEN  1
#define PS3_DRUM_CONTROLLER_RED    2
#define PS3_DRUM_CONTROLLER_YELLOW 3
#define PS3_DRUM_CONTROLLER_XXX    4
#define PS3_DRUM_CONTROLLER_ORANGE 5

class JoystickEvent {
  public:
	enum Type {BUTTON_DOWN, BUTTON_UP, HAT_MOTION, AXIS_MOTION, BALL_MOTION};
	JoystickEvent(): button_id(), button_state(), hat_id(), hat_direction(CENTERED), axis_id(), axis_value(), ball_id(), ball_dx(), ball_dy()  {};
	JoystickEvent(Type _type): type(_type), button_id(), button_state(), hat_id(), hat_direction(CENTERED), axis_id(), axis_value(), ball_id(), ball_dx(), ball_dy()  {};
	Type type;
	// for BUTTON_DOWN and BUTTON_UP
	unsigned char button_id;
	bool button_state;
	// for HAT_MOTION
	enum HatDirection {
		LEFT_UP, UP, RIGHT_UP,
		LEFT, CENTERED, RIGHT,
		LEFT_DOWN, DOWN, RIGHT_DOWN
	};
	unsigned char hat_id;
	HatDirection hat_direction;
	// for AXIS_MOTION
	unsigned char axis_id;
	short axis_value;
	// for BALL_MOTION
	unsigned char ball_id;
	short ball_dx;
	short ball_dy;
};

/// joystick class used for dealing with ps3 drumsets
class Joystick {
  public:
  	enum Type {UNKNOWN, GUITARHERO, ROCKBAND};
	/// create joystick object
	Joystick(Joystick::Type _type = Joystick::UNKNOWN);
	~Joystick();
	/// Return joystick Type
	Joystick::Type getType() const;
	/// add an even from an SDL_JoyAxisEvent
	void addEvent(SDL_JoyAxisEvent event);
	/// add an even from an SDL_JoyBallEvent
	void addEvent(SDL_JoyBallEvent event);
	/// add an even from an SDL_JoyHatEvent
	void addEvent(SDL_JoyHatEvent event);
	/// add an even from an SDL_JoyButtonEvent
	void addEvent(SDL_JoyButtonEvent event);
	/// try to poll an event, return false if no more event is available
	bool tryPollEvent(JoystickEvent& _event);
	/// clear all the events
	void clearEvents();
  private:
	std::deque<JoystickEvent> m_events;
	Type m_type;
};

typedef std::map<unsigned int,Joystick> Joysticks;
extern Joysticks joysticks;

/**
 * New input management, superseed all joysticks stuffs
 */
namespace input {
	enum DevType { GUITAR, DRUMS };

	static const std::size_t BUTTONS = 6;

	typedef std::map<unsigned int,SDL_Joystick*> SDL_devices;
	extern SDL_devices sdl_devices;
	void init_devices();

	struct InputDevEvent {
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
			bool tryPoll(InputDevEvent& _event) {
				if( m_events.empty() ) return false;
				_event = m_events.front();
				m_events.pop_front();
				return true;
			};
			void addEvent(InputDevEvent _event) {
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
			bool type_match( input::Type _type) {
				if( _type == input::UNKNOWN && m_type == input::Private::UNKNOWN ) {
					return true;
				} else if( _type == input::GUITAR && 
					(m_type == input::Private::GUITAR_GH || m_type == input::Private::GUITAR_RB) ) {
					return true;
				} else if( _type == input::DRUM && 
					(m_type == input::Private::DRUM_GH || m_type == input::Private::DRUM_RB) ) {
					return true;
				} else {
					return false;
				}
			};
			void setType(input::Type _type) {
				if( _type == input::GUITAR ) {
					m_type = input::Private::GUITAR_RB;
				} else if( _type == input::DRUM ) {
					m_type = input::Private::DRUM_RB;
				} else {
					m_type = input::Private::UNKNOWN;
				}
				init_devices();
			};
		  private:
			std::deque<InputDevEvent> m_events;
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
			if( devices.size() == 0 ) throw std::runtime_error("No InputDev available");
			for(InputDevs::iterator it = devices.begin() ; it != devices.end() ; ++it) {
				if( !it->second.assigned() && it->second.type_match(_type) ) {
					m_device_id = it->first;
					it->second.assign();
					return;
				}
			}
			for(InputDevs::iterator it = devices.begin() ; it != devices.end() ; ++it) {
				if( !it->second.assigned() && it->second.type_match(input::UNKNOWN) ) {
					m_device_id = it->first;
					it->second.setType(_type);
					it->second.assign();
					return;
				}
			}
			throw std::runtime_error("No matching instrument available");
		};
		~InputDev() {input::Private::devices[m_device_id].unassign();};
		bool tryPoll(InputDevEvent& _e) {return input::Private::devices[m_device_id].tryPoll(_e);};
		void addEvent(InputDevEvent _e) {input::Private::devices[m_device_id].addEvent(_e);};
		bool pressed(int _button) {return input::Private::devices[m_device_id].pressed(_button);}; // Current state
	  private:
		unsigned int m_device_id; // should be some kind of reference
	};

	// Initialize all event stuffs
	void init();
	// clear all event stuffs
	void clear();
	// Returns true if event is taken, feed an InputDev by transforming SDL_Event into InputDevEvent
	bool pushEvent(SDL_Event);
};


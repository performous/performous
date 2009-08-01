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
	Joystick() {};
	/// create joystick object with given id
	Joystick(unsigned int _id);
	~Joystick();
	/// Return joystick Type
	Joystick::Type getType() const;
	/// get name of joystick
	std::string getName() const;
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
	/// returns button state (true if pressed, else false)
	bool buttonState(unsigned char _button_id)  const;
	/// returns hi-hat state
	JoystickEvent::HatDirection hat(unsigned char _hat_id) const;
	/// returns axis state
	short axis(unsigned char _axis_id) const;
	/// returns ball state
	std::pair<int, int> ball(int _ball_id) const;
  private:
	SDL_Joystick * m_joystick;
	std::deque<JoystickEvent> m_events;
	unsigned int m_id;
	Type m_type;
};

typedef std::map<unsigned int,Joystick> Joysticks;
extern Joysticks joysticks;

/**
 * New input management, superseed all joysticks stuffs
 */
namespace input {
	enum Type {UNKNOWN, GUITAR, DRUM};

	static const std::size_t BUTTONS = 6;

	struct InputDevEvent {
		enum Type { PRESS, RELEASE, PICK };
		Type type;
		int button; // Translated button number for press/release events. 0 for pick down, 1 for pick up (NOTE: these are NOT pick press/release events but rather different directions)
		bool pressed[BUTTONS]; // All events tell the button state right after the event happened
		// More stuff later, when it is actually used
	};
	
	namespace Private {
		class InputDevPrivate {
		  public:
			InputDevPrivate() : m_assigned(false), m_type(input::UNKNOWN) {};
			bool tryPoll(InputDevEvent&) {return true;};
			void addEvent(InputDevEvent) {};
			void clearEvents() {m_events.clear();};
			bool status(InputDevEvent::Type) {return false;}; // for buttons
			short value(InputDevEvent::Type) {return 0;}; // for axis
			void assign() {m_assigned = true;};
			void unassign() {m_assigned = false;};
			bool assigned() {return m_assigned;};
		  private:
			std::deque<InputDevEvent> m_events;
			bool m_assigned;
			input::Type m_type;
		};
	
		typedef std::map<unsigned int,InputDevPrivate> InputDevs;
		extern InputDevs devices;
	}

	class InputDev {
	  public:
		// Throw an exception if none is available
		InputDev(input::Type) : m_device_id() {input::Private::devices[m_device_id].assign();};
		~InputDev() {input::Private::devices[m_device_id].unassign();};
		bool tryPoll(InputDevEvent& _e) {return input::Private::devices[m_device_id].tryPoll(_e);};
		void addEvent(InputDevEvent _e) {input::Private::devices[m_device_id].addEvent(_e);};
		bool status(InputDevEvent::Type _t) {return input::Private::devices[m_device_id].status(_t);}; // for buttons
		short value(InputDevEvent::Type _t) {return input::Private::devices[m_device_id].value(_t);}; // for axis
		bool pressed[BUTTONS]; // Current state
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


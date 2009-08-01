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

void joysticks_init();

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
	/// get longer description of joystick
	std::string getDescription() const;
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

	struct InputDevEvent {
		enum Type {GREEN, RED, YELLOW, BLUE, ORANGE, KICK, PICK, WHAMMY};
		Type type;
		bool pressed; // for buttons
		short value; // for axis
	};
	
	namespace Private {
		class InputDevPrivate {
		  public:
			InputDevPrivate() {};
			bool tryPoll(InputDevEvent&) {return true;};
			void addEvent(InputDevEvent) {};
			bool status(InputDevEvent::Type) {return false;}; // for buttons
			short value(InputDevEvent::Type) {return 0;}; // for axis
		  private:
			bool m_assigned;
			input::Type m_type;
		};
	
		typedef std::map<unsigned int,InputDevPrivate> InputDevs;
		extern InputDevs devices;
	}

	class InputDev {
	  public:
		InputDev(input::Private::InputDevPrivate& _device):m_device(_device) {};
		bool tryPoll(InputDevEvent& _e) {return m_device.tryPoll(_e);};
		void addEvent(InputDevEvent _e) {m_device.addEvent(_e);};
		bool status(InputDevEvent::Type _t) {return m_device.status(_t);}; // for buttons
		short value(InputDevEvent::Type _t) {return m_device.value(_t);}; // for axis
	  private:
		input::Private::InputDevPrivate& m_device;
	};

	// Throw an exception if none is available
	InputDev& assign(input::Type);
	// Returns true if event is taken, feed an InputDev by transforming SDL_Event into InputDevEvent
	bool pushEvent(SDL_Event);
};


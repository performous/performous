#pragma once

#include <climits>
#include <deque>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <boost/noncopyable.hpp>

#include "SDL_events.h"

#include "xtime.hh"
#include "configuration.hh"

namespace input {
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

	enum SourceType { MIDI, KEYBOARD, JOYSTICK };	
	enum DevType { GUITAR, DRUMS, KEYTAR, DANCEPAD };
	enum NavButton { NONE, UP, DOWN, LEFT, RIGHT, START, SELECT, CANCEL, PAUSE, MOREUP, MOREDOWN, VOLUME_UP, VOLUME_DOWN };

	/// Each controller has unique SourceId that can be used for telling players apart etc.
	struct SourceId {
		SourceType type;
		unsigned device, major, minor;
	};

	bool operator==(SourceId const& a, SourceId const& b) {
		return a.type == b.type && a.device == b.device && a.major == b.major && a.minor == b.minor;
	}
	
	bool operator!=(SourceId const& a, SourceId const& b) { return !(a == b); }
	
	/// NavEvent is a menu navigation event, generalized for all controller type so that the user doesn't need to know about controllers.
	struct NavEvent {
		NavButton button;
		boost::xtime time;
		unsigned repeat;  ///< Zero for hardware event, increased by one for each auto-repeat
		SourceId source;
	};
	
	struct Event {
		enum Type { PRESS, RELEASE, PICK };
		Type type;
		int button; // Translated button number for press/release events. 0 for pick down, 1 for pick up (NOTE: these are NOT pick press/release events but rather different directions)
		bool pressed[BUTTONS]; // All events tell the button state right after the event happened
		NavButton nav; // Event translated to NavButton
		boost::xtime time;
		// More stuff later, when it is actually used
	};

	class SDLDevice;
	class MidiDevice;
	
	class Controllers {
	public:
		static bool midiEnabled() { return true; /* FIXME */ }
		Controllers();
		~Controllers();
		/// Internally poll for new events. The current time is passed for reference.
		void process(boost::xtime const& now);
		/// Push an SDL event to the queue. Returns true if the event was taken (recognized and accepted).
		bool pushEvent(SDL_Event event, boost::xtime const& now);
		/// Return true and an event if there are any in queue. Otherwise return false.
		bool tryPoll(NavEvent& ev);
		
	private:
		boost::scoped_ptr<SDLDevice> sdldev;
		boost::ptr_vector<MidiDevice> mididevs;
	};

	struct NoDevError: std::runtime_error {
		NoDevError(): runtime_error("No instrument of the requested type was available") {}
	};

	/// A handle that reserves one controller of the given type for gameplay. Throws NoDevError if none are available.
	class InputDev: boost::noncopyable {
	public:
		const SourceId source;
		InputDev(Controllers& controllers, DevType type);
		~InputDev();
		bool tryPoll(Event& _e);
		void addEvent(Event _e);
		bool pressed(int _button);
	};

}


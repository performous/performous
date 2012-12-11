#pragma once

#include <climits>
#include <deque>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/smart_ptr/scoped_ptr.hpp>

#include "SDL_events.h"

#include "xtime.hh"
#include "configuration.hh"

namespace input {
	enum SourceType { SOURCETYPE_NONE, SOURCETYPE_JOYSTICK, SOURCETYPE_MIDI, SOURCETYPE_KEYBOARD, SOURCETYPE_N };
	enum DevType { DEVTYPE_NONE, DEVTYPE_GUITAR, DEVTYPE_DRUMS, DEVTYPE_KEYTAR, DEVTYPE_PIANO, DEVTYPE_DANCEPAD, DEVTYPE_N };
	/// Generalized mapping of navigation actions
	enum NavButton { NONE, UP, DOWN, LEFT, RIGHT, START, SELECT, CANCEL, PAUSE, MOREUP, MOREDOWN, VOLUME_UP, VOLUME_DOWN };
	/// Alternative orientation-agnostic mapping where PRIMARY axis is the one that is easiest to access (e.g. guitar pick) and SECONDARY might not be available on all devices
	enum NavMenu { NO_MENUNAV, PRIMARY_PREV, PRIMARY_NEXT, SECONDARY_PREV, SECONDARY_NEXT };

	enum Button {
		GENERIC_UNASSIGNED = 0x100,  // Listed in controllers.xml but not used for any function
		GENERIC_START = 0x101,
		GENERIC_SELECT = 0x102,
		// Button constants for each DevType
		#define DEFINE_BUTTON(devtype, button, num, nav) devtype##_##button = num,
		#include "controllers-buttons.ii"
	};

	typedef unsigned HWButton;
	
	/// Each controller has unique SourceId that can be used for telling players apart etc.
	struct SourceId {
		SourceId(SourceType type = SOURCETYPE_NONE, unsigned device = 0, unsigned channel = 0): type(type), device(device), channel(channel) {
			if (device >= 1024) throw std::invalid_argument("SourceId device must be smaller than 1024.");
			if (channel >= 1024) throw std::invalid_argument("SourceId channel must be smaller than 1024.");
		}
		SourceType type;
		unsigned device, channel;  ///< Device number and channel (0..1023)
		/// Provide numeric conversion for comparison and ordered containers
		operator unsigned() const { return unsigned(type)<<20 | device<<10 | channel; }
	};

	/// NavEvent is a menu navigation event, generalized for all controller type so that the user doesn't need to know about controllers.
	struct NavEvent {
		SourceId source;
		NavButton button;
		NavMenu menu;
		boost::xtime time;
		unsigned repeat;  ///< Zero for hardware event, increased by one for each auto-repeat
		NavEvent(): source(), button(), menu(), time(), repeat() {}
	};
	
	struct Event {
		SourceId source; ///< Where did it originate from
		HWButton hw; ///< Hardware button number (for internal use and debugging only)
		Button id; ///< Mapped button id
		NavButton navButton; ///< Navigational button interpretation
		double value; ///< Zero for button release, up to 1.0 for press (e.g. velocity value), or axis value (-1.0 .. 1.0)
		boost::xtime time; ///< When did the event occur
		DevType devType; ///< Device type
		Event(): source(), hw(), id(GENERIC_UNASSIGNED), value(), time(), devType() {}
	};

	/// EventHandler is a function called to process an event as soon as it arrives. Returns true if the event was accepted.
	typedef boost::function<bool (Event const& ev)> EventHandler;

	/// MissingEventHandler is a function called if whenever an event not currently assigned an EventHandler is received.
	/// Returns NULL if such events should be ignored or an EventHandler that will then be instantly called to process the event and bound for any future events.
	typedef boost::function<EventHandler (DevType devType)> MissingEventHandler;
	
	class Controllers {
	public:
		Controllers();
		~Controllers();
		/// Return true and an event if there are any in queue. Otherwise return false.
		bool getNav(NavEvent& ev);
		/// Test if a particular button is currently being held (returns last value, zero if not pressed)
		double pressed(SourceId const& source, Button button);
		/// Internally poll for new events. The current time is passed for reference.
		void process(boost::xtime const& now);
		/// Push an SDL event for processing. Returns true if the event was taken (recognized and accepted).
		bool pushEvent(SDL_Event const& sdlEv, boost::xtime const& now);
	private:
		struct Impl;
		boost::scoped_ptr<Impl> self;
	};

	/// Base class for different types of hardware backends.
	class Hardware {
	public:
		static bool midiEnabled();
		virtual ~Hardware() {}
		/// Return an Event and true if any are available. The Event is pre-initialized with current time.
		virtual bool process(Event&) { return false; }
		/// Convert an SDL event into Event. The Event is pre-initialized with event's time. Returns false if SDL_Event was not handled.
		virtual bool process(Event&, SDL_Event const&) { return false; }
		// Note: process functions are expected to return Event with source, hw and value set and possibly with time adjusted.
		typedef boost::shared_ptr<Hardware> ptr;
	};
	
	Hardware::ptr constructKeyboard();
	Hardware::ptr constructJoysticks();
	Hardware::ptr constructMidi();
}



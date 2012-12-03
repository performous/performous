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
	enum SourceType { SOURCETYPE_NONE, SOURCETYPE_JOYSTICK, SOURCETYPE_MIDI, SOURCETYPE_KEYBOARD };
	enum DevType { DEVTYPE_RAW, DEVTYPE_GUITAR, DEVTYPE_DRUMS, DEVTYPE_KEYTAR, DEVTYPE_PIANO, DEVTYPE_DANCEPAD };
	/// Generalized mapping of navigation actions
	enum NavButton { NONE, UP, DOWN, LEFT, RIGHT, START, SELECT, CANCEL, PAUSE, MOREUP, MOREDOWN, VOLUME_UP, VOLUME_DOWN };
	/// Alternative orientation-agnostic mapping where PRIMARY axis is the one that is easiest to access (e.g. guitar pick) and SECONDARY might not be available on all devices
	enum NavMenu { NO_MENUNAV, PRIMARY_PREV, PRIMARY_NEXT, SECONDARY_PREV, SECONDARY_NEXT };

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
		NavButton button;
		NavMenu menu;
		boost::xtime time;
		unsigned repeat;  ///< Zero for hardware event, increased by one for each auto-repeat
		SourceId source;
	};
	
	struct Event {
		SourceId source; ///< Where did it originate from
		unsigned id; ///< Originally hardware button number as-is, later mapped according to controllers.xml
		double value; ///< Zero for button release, up to 1.0 for press (e.g. velocity value), or axis value (-1.0 .. 1.0)
		boost::xtime time; ///< When did the event occur
		DevType devType; ///< What type of device (RAW for events not yet mapped)
		Event(SourceId source, unsigned id, unsigned value, boost::xtime const& time): source(source), id(id), value(value), time(time), devType(DEVTYPE_RAW) {}
	};

	/// EventHandler is a function called to process an event as soon as it arrives. Returns true if the event was accepted.
	typedef boost::function<bool (Event const& ev)> EventHandler;

	/// MissingEventHandler is a function called if whenever an event not currently assigned an EventHandler is received.
	/// Returns NULL if such events should be ignored or an EventHandler that will then be instantly called to process the event and bound for any future events.
	typedef boost::function<EventHandler (DevType devType)> MissingEventHandler;
	
	class Controllers {
	public:
		static bool midiEnabled() { return true; /* FIXME */ }
		Controllers();
		~Controllers();
		/// Internally poll for new events. The current time is passed for reference.
		void process(boost::xtime const& now);
		/// Push an SDL event for processing. Returns true if the event was taken (recognized and accepted).
		bool pushEvent(SDL_Event event, boost::xtime const& now);
		/// Return true and an event if there are any in queue. Otherwise return false.
		bool getNav(NavEvent& ev);
		/// Test if a particular button is currently being held
		bool pressed(SourceId const&, unsigned button);
				
	private:
		struct Impl;
		boost::scoped_ptr<Impl> self;
	};

}

// Button constants
#include "controllers-const.ii"


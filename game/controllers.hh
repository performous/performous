#pragma once

#include "chrono.hh"
#include "configuration.hh"
#include "util.hh"
#include <SDL_events.h>
#include <climits>
#include <deque>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace input {
	enum class SourceType { NONE, JOYSTICK, MIDI, KEYBOARD, N };
	enum class DevType : unsigned { GENERIC, VOCALS, GUITAR, DRUMS, KEYTAR, PIANO, DANCEPAD, N };
	/// Generalized mapping of navigation actions
	enum class NavButton {
		NONE /* No NavEvent emitted */, SOME /* Major gameplay button with no direct nav function, used for joining instruments */,
		START, CANCEL, PAUSE,
		REPEAT = 0x80 /* Anything after this is auto-repeating */,
		UP, DOWN, LEFT, RIGHT, MOREUP, MOREDOWN, VOLUME_UP, VOLUME_DOWN
	};
	/// Alternative orientation-agnostic mapping where A axis is the one that is easiest to access (e.g. guitar pick) and B might not be available on all devices
	enum class NavMenu { NAVMENU_NONE, NAVMENU_A_PREV, NAVMENU_A_NEXT, NAVMENU_B_PREV, NAVMENU_B_NEXT };

	enum class ButtonId: unsigned {
		// Button constants for each DevType
		#define DEFINE_BUTTON(devtype, button, num, nav) devtype##_##button = num,
		#include "controllers-buttons.ii"
	};

	struct Button {
		ButtonId id;
		Button(ButtonId id = ButtonId::GENERIC_UNASSIGNED): id(id) {}
		Button(unsigned layer, unsigned num): id(ButtonId(layer << 8 | num)) {}
		operator ButtonId() const { return id; }
		unsigned layer() const { return to_underlying(id) >> 8; }
		unsigned num() const { return to_underlying(id) & 0xFF; }
		bool generic() const { return layer() == 0x100; }
	};

	typedef unsigned HWButton;
	static const MinMax<HWButton> hwIsAxis(0x10000000u, 0x1000FFFFu);
	static const MinMax<HWButton> hwIsHat(0x11000000u, 0x1100FFFFu);
	
	/// Each controller has unique SourceId that can be used for telling players apart etc.
	struct SourceId {
		SourceId(SourceType type = SourceType::NONE, unsigned device = 0, unsigned channel = 0): type(type), device(device), channel(channel) {
		}
		SourceType type;
		unsigned device, channel;  ///< Device number and channel (0..1023)
		/// Provide numeric conversion for comparison and ordered containers
		operator unsigned() const { return unsigned(type)<<20 | device<<10 | channel; }
		bool isKeyboard() const { return type == SourceType::KEYBOARD; }  ///< This is so common test that a helper is provided
	};
	
	struct Event {
		SourceId source; ///< Where did it originate from
		HWButton hw; ///< Hardware button number (for internal use and debugging only)
		Button button; ///< Mapped button id
		NavButton nav; ///< Navigational button interpretation
		double value; ///< Zero for button release, up to 1.0 for press (e.g. velocity value), or axis value (-1.0 .. 1.0)
		Time time; ///< When did the event occur
		DevType devType; ///< Device type
		Event(): source(), hw(), nav(NavButton::NONE), value(), time(), devType() {}
		bool pressed() const { return value != 0.0; }
	};

	/// NavEvent is a menu navigation event, generalized for all controller type so that the user doesn't need to know about controllers.
	struct NavEvent {
		SourceId source;
		DevType devType;
		NavButton button;
		NavMenu menu;
		Time time;
		unsigned repeat;  ///< Zero for hardware event, increased by one for each auto-repeat
		NavEvent(): source(), devType(), button(), menu(), time(), repeat() {}
		explicit NavEvent(Event const& ev): source(ev.source), devType(ev.devType), button(ev.nav), menu(), time(ev.time), repeat() {}
	};
	
	/// A handle for receiving device events
	class Device {
		typedef std::deque<Event> Events;
		Events m_events;
	public:
		Device(const Device&) = delete;
  		const Device& operator=(const Device&) = delete;
		const SourceId source;
		const DevType type;
		Device(SourceId const& source, DevType type): source(source), type(type) {}
		bool getEvent(Event&);
		void pushEvent(Event const&);
	};
	typedef std::shared_ptr<Device> DevicePtr;

	/// The main controller class that contains everything
	class Controllers {
	public:
		Controllers(const Controllers&) = delete;
  		const Controllers& operator=(const Controllers&) = delete;
		Controllers();
		~Controllers();
		/// Return true and a nav event if there are any in queue. Otherwise return false.
		bool getNav(NavEvent& ev);
		/// Enable or disable event processing (pending events will be cleared).
		void enableEvents(bool state);
		/// Adopt a specific orphan device (for receiving Events).
		DevicePtr registerDevice(SourceId const& source);
		/// Internally poll for new events. The current time is passed for reference.
		void process(Time now);
		/// Push an SDL event for processing. Returns true if the event was taken (recognized and accepted).
		bool pushEvent(SDL_Event const& sdlEv, Time now);
	private:
		struct Impl;
		std::unique_ptr<Impl> self;
	};

	/// Base class for different types of hardware backends.
	class Hardware {
	public:
		Hardware(const Hardware&) = delete;
  		const Hardware& operator=(const Hardware&) = delete;
		static bool midiEnabled();
		static void enableKeyboardInstruments(bool state);
		Hardware() {}
		virtual ~Hardware() {}
		/// Get the name of a specific device of this type
		virtual std::string getName(unsigned) const { return std::string(); }
		/// Return an Event and true if any are available. The Event is pre-initialized with current time.
		virtual bool process(Event&) { return false; }
		/// Convert an SDL event into Event. The Event is pre-initialized with event's time. Returns false if SDL_Event was not handled.
		virtual bool process(Event&, SDL_Event const&) { return false; }
		// Note: process functions are expected to return Event with source, hw and value set and possibly with time adjusted.
		typedef std::shared_ptr<Hardware> ptr;
	};
	
	Hardware::ptr constructKeyboard();
	Hardware::ptr constructJoysticks();
	Hardware::ptr constructMidi();
}



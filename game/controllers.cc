#include "controllers.hh"

#include "chrono.hh"
#include "fs.hh"
#include "libxml++-impl.hh"
#include "unicode.hh"
#include <regex>
#include <SDL2/SDL_joystick.h>

#include <deque>
#include <stdexcept>
#include <algorithm>

using namespace input;

namespace {
	struct XMLError {
		XMLError(xmlpp::Element const& e, std::string const& msg): elem(e), message(msg) {}
		xmlpp::Element const& elem;
		std::string message;
	};
	std::string getAttribute(xmlpp::Element const& elem, std::string const& attr) {
		xmlpp::Attribute const* a = elem.get_attribute(attr);
		if (!a) throw XMLError(elem, "attribute " + attr + " not found");
		return a->get_value();
	}
	template <typename T> bool tryGetAttribute(xmlpp::Element const& elem, std::string const& attr, T& var) {
		xmlpp::Attribute const* a = elem.get_attribute(attr);
		if (!a) return false;
		try {
			var = sconv<T>(a->get_value());
		} catch (std::exception&) {
			throw XMLError(elem, "attribute " + attr + " value invalid: " + a->get_value());
		}
		return true;
	}
	template <typename Numeric> void parse(MinMax<Numeric>& range, xmlpp::Element const& elem) {
		tryGetAttribute(elem, "min", range.min);
		tryGetAttribute(elem, "max", range.max);
	}
	// Return a NavButton corresponding to an Event
	NavButton navigation(Event const& ev) {
		#define DEFINE_BUTTON(dt, btn, num, nav) if ((DEVTYPE_##dt == DEVTYPE_GENERIC || ev.devType == DEVTYPE_##dt) && ev.button == dt##_##btn) return nav;
		#include "controllers-buttons.ii"
		return NAV_NONE;
	}

	std::ostream& operator<<(std::ostream& os, SourceId const& source) {
		switch (source.type) {
			case SOURCETYPE_NONE: return os << "(none)";
			case SOURCETYPE_KEYBOARD: return os << "(keyboard " << source.device << " instrument " << source.channel << ")";
			case SOURCETYPE_JOYSTICK: return os << "(joystick " << source.device << ")";
			case SOURCETYPE_MIDI: return os << "(midi " << source.device << " channel " << source.channel << ")";
			case SOURCETYPE_N: break;
		}
		throw std::logic_error("Unknown SOURCETYPE in controllers.cc SourceId operator<<");
	}
	std::string buttonDebug(DevType type, Button b) {
		#define DEFINE_BUTTON(dt, btn, num, nav) if ((DEVTYPE_##dt == DEVTYPE_GENERIC || type == DEVTYPE_##dt) && b == dt##_##btn) \
		  return #dt " " #btn " (" #nav ")";
		#include "controllers-buttons.ii"
		throw std::logic_error("Invalid Button value in controllers.cc buttonDebug");
	}
	std::ostream& operator<<(std::ostream& os, Event const& ev) {
		os << ev.source << ' ';
		// Print hw button info if the event is not assigned to a function, otherwise print assignments
		if (ev.button == GENERIC_UNASSIGNED) {
			if (hwIsAxis.matches(ev.hw)) {
				os << "axis hw=" << ev.hw - hwIsAxis.min << " value=" << ev.value;
			} else if (hwIsHat.matches(ev.hw)) {
				os << "hat hw=" << ev.hw - hwIsHat.min << ' ';
				std::string dir;
				unsigned val = ev.value;
				if (val & SDL_HAT_UP) dir += "up";
				if (val & SDL_HAT_DOWN) dir += "down";
				if (val & SDL_HAT_LEFT) dir += "left";
				if (val & SDL_HAT_RIGHT) dir += "right";
				os << (dir.empty() ? "centered" : dir);
			}
			else os << "button hw=" << ev.hw << " value=" << ev.value;
		} else {
			os << buttonDebug(ev.devType, ev.button) << " value=" << ev.value;
		}
		return os;
	}
}

struct ButtonMap {
	Button map;  // Generic action
	Button negative, positive;  // Half-axis movement
	Button up, down, left, right;  // Hat direction
};

typedef std::map<HWButton, ButtonMap> ButtonMapping;

/// A controller definition from controllers.xml
struct ControllerDef {
	std::string name;
	std::string description;
	SourceType sourceType;
	DevType devType;
	double latency;
	std::regex deviceRegex;
	MinMax<unsigned> deviceMinMax;
	MinMax<unsigned> channelMinMax;
	ButtonMapping mapping;
	ControllerDef(): sourceType(), devType(), latency() {}
	bool matches(Event const& ev, std::string const& devName) const {
		if (ev.source.type != sourceType) return false;
		if (!deviceMinMax.matches(ev.source.device)) return false;
		if (!channelMinMax.matches(ev.source.channel)) return false;
		if (!regex_search(devName, deviceRegex)) return false;
		return true;
	}
};

struct Controllers::Impl {
	typedef std::map<std::string, ControllerDef> ControllerDefs;
	ControllerDefs m_controllerDefs;

	typedef std::map<std::string, Button> NameToButton;
	NameToButton m_buttons[DEVTYPE_N];

	typedef std::map<SourceId, ControllerDef const*> Assignments;
	Assignments m_assignments;
	
	typedef std::map<SourceType, Hardware::ptr> HW;
	HW m_hw;

	std::deque<NavEvent> m_navEvents;
	std::map<SourceId, DevicePtr> m_orphans;
	std::map<SourceId, std::weak_ptr<Device> > m_devices;
	bool m_eventsEnabled;
	
	typedef std::pair<SourceId, ButtonId> UniqueButton;
	std::map<UniqueButton, double> m_values;

	std::map<NavButton, NavEvent> m_navRepeat;

	Time m_prevProcess{};

	Impl(): m_eventsEnabled() {
		#define DEFINE_BUTTON(devtype, button, num, nav) m_buttons[DEVTYPE_##devtype][#button] = devtype##_##button;
		#include "controllers-buttons.ii"
		readControllers(getShareDir() / "config/controllers.xml");
		readControllers(getConfigDir() / "controllers.xml");
		m_hw[SOURCETYPE_KEYBOARD] = constructKeyboard();
		m_hw[SOURCETYPE_JOYSTICK] = constructJoysticks();
		if (Hardware::midiEnabled()) m_hw[SOURCETYPE_MIDI] = constructMidi();
	}
	
	void readControllers(fs::path const& file) {
		if (!fs::is_regular_file(file)) {
			std::clog << "controllers/info: Skipping " << file << " (not found)" << std::endl;
			return;
		}
		std::clog << "controllers/info: Parsing " << file << std::endl;
		xmlpp::DomParser domParser(file.string());
		try {
			parseControllers(domParser, "/controllers/joystick/controller", SOURCETYPE_JOYSTICK);
			parseControllers(domParser, "/controllers/midi/controller", SOURCETYPE_MIDI);
		} catch (XMLError& e) {
			int line = e.elem.get_line();
			std::string name = e.elem.get_name();
			throw std::runtime_error(file.string() + ":" + std::to_string(line) + " element " + name + " " + e.message);
		}
	}

	void parseControllers(xmlpp::DomParser const& dom, std::string const& path, SourceType sourceType) {
		auto n = dom.get_document()->get_root_node()->find(path);
		for (auto nodeit = n.begin(), end = n.end(); nodeit != end; ++nodeit) {
			const xmlpp::Element& elem = dynamic_cast<const xmlpp::Element&>(**nodeit);
			ControllerDef def;
			def.name = getAttribute(elem, "name");
			def.sourceType = sourceType;
			// Device type
			{
				std::string type = getAttribute(elem, "type");
				if (type == "guitar") def.devType = DEVTYPE_GUITAR;
				else if (type == "drumkit") def.devType = DEVTYPE_DRUMS;
				else if (type == "keytar") def.devType = DEVTYPE_KEYTAR;
				else if (type == "piano") def.devType = DEVTYPE_PIANO;
				else if (type == "dancepad") def.devType = DEVTYPE_DANCEPAD;
				else {
					std::clog << "controllers/warning: " << type << ": Unknown controller type in controllers.xml (skipped)" << std::endl;
					continue;
				}
			}
			// Device description
			auto ns = elem.find("description/text()");
			if (ns.size() == 1) def.description = dynamic_cast<const xmlpp::TextNode&>(*ns[0]).get_content();
			// Read filtering rules
			ns = elem.find("device");
			if (ns.size() == 1) {
				const xmlpp::Element& elem = dynamic_cast<const xmlpp::Element&>(*ns[0]);
				std::string regex;
				if (tryGetAttribute(elem, "regex", regex)) { def.deviceRegex = regex; }
				parse(def.deviceMinMax, elem);
				double latency;
				if (tryGetAttribute(elem, "latency", latency)) def.latency = latency;
			}
			ns = elem.find("channel");
			if (ns.size() == 1) parse(def.channelMinMax, dynamic_cast<const xmlpp::Element&>(*ns[0]));
			// Read button mapping
			ns = elem.find("mapping/*");
			for (auto nodeit2 = ns.begin(), end = ns.end(); nodeit2 != end; ++nodeit2) {
				const xmlpp::Element& elem = dynamic_cast<const xmlpp::Element&>(**nodeit2);
				HWButton hw;
				if (!tryGetAttribute(elem, "hw", hw)) throw XMLError(elem, "Mandatory attribute hw is missing or invalid");
				// Axes and hats use different HWButton ranges internally
				if (elem.get_name() == "axis") hw = HWButton(hw + hwIsAxis.min);
				else if (elem.get_name() == "hat") hw = HWButton(hw + hwIsHat.min);
				// Parse the mapping for this HWButton
				ButtonMap& m = def.mapping[hw];
				m.map = parseButton(elem, "map", def);
				m.negative = parseButton(elem, "negative", def);
				m.positive = parseButton(elem, "positive", def);
				m.up = parseButton(elem, "up", def);
				m.down = parseButton(elem, "down", def);
				m.left = parseButton(elem, "left", def);
				m.right = parseButton(elem, "right", def);
			}
			// Add/replace the controller definition
			std::clog << "controllers/info: " << (m_controllerDefs.find(def.name) == m_controllerDefs.end() ? "Controller" : "Overriding")
			  << " definition: " << def.name << ": " << def.description << std::endl;
			m_controllerDefs[def.name] = def;
		}
	}
	/// Read a button attribute from XML element
	Button parseButton(xmlpp::Element const& elem, std::string const& attr, ControllerDef const& def) {
		std::string action;
		tryGetAttribute(elem, attr, action);
		return findButton(def.devType, action);
	}
	/// Find button by name, either of given type or of generic type
	Button findButton(DevType type, std::string name) {
		Button button = GENERIC_UNASSIGNED;
		if (name.empty()) return button;
		name = UnicodeUtil::toUpper(name);
		std::replace( name.begin(), name.end(), '-', '_');
		// Try getting button first from devtype-specific, then generic names
		buttonByName(type, name, button) || buttonByName(DEVTYPE_GENERIC, name, button) ||
		  std::clog << "controllers/warning: " << name << ": Unknown button name in controllers.xml." << std::endl;
		return button;
	}
	/// Try to find button of specific type
	bool buttonByName(DevType type, std::string const& name, Button& button) {
		NameToButton const& n2b = m_buttons[type];
		auto it = n2b.find(name);
		if (it == n2b.end()) return false;
		button = it->second;
		return true;
	}
	/// Return the next available navigation event from queue, if available
	bool getNav(NavEvent& ev) {
		if (m_navEvents.empty()) return false;
		ev = m_navEvents.front();
		m_navEvents.pop_front();
		return true;
	}
	/// Register an orphan device
	DevicePtr registerDevice(SourceId const& source) {
		auto it = m_orphans.find(source);
		if (it == m_orphans.end()) return DevicePtr();
		DevicePtr ret = it->second;
		m_orphans.erase(it);
		return ret;
	}
	void enableEvents(bool state) {
		m_eventsEnabled = state;
		Hardware::enableKeyboardInstruments(state);
	}
	/// Do internal event processing (poll for MIDI events etc)
	void process(Time now) {
		for (auto& typehw: m_hw) {
			while (true) {
				Event event;
				event.time = now;
				if (!typehw.second->process(event)) break;
				pushHWEvent(event);
			}
		}
		// Reset all key repeat timers if there is a latency spike
		if (now - m_prevProcess > 50ms) {
			for (auto& kv: m_navRepeat) kv.second.time = now;
		}
		m_prevProcess = now;
		// Spawn key repeat events when holding buttons
		for (auto& kv: m_navRepeat) {
			NavEvent& ne = kv.second;
			Seconds delay(2.0 / (10 + ne.repeat));
			Seconds since = now - ne.time;
			if (since < delay) continue;  // Not yet time to repeat
			// Emit auto-repeated event
			// Note: We intentionally only emit one per frame (call to process) to avoid surprises when latency spikes occur.
			++ne.repeat;
			ne.time += clockDur(delay);  // Increment rather than set to now, so that repeating is smoother.
			std::clog << "controllers/debug: NavEvent auto repeat " << ne.repeat << " after " << since.count() << " s, next delay " << delay.count() << "s " << std::endl;
			m_navEvents.push_back(ne);
		}
	}
	/// Handle an incoming SDL event
	bool pushEvent(SDL_Event const& sdlEv, Time t) {
		for (auto& typehw: m_hw) {
			Event event;
			event.time = t;
			if (typehw.second->process(event, sdlEv)) { pushHWEvent(event); return true; }
		}
		return false;
	}
	/// Assign event's source a ControllerDef (if not already assigned) and return it
	ControllerDef const* assign(Event const& event) {
		// Attempt insertion (does not override existing values)
		std::pair<Assignments::iterator, bool> ret = m_assignments.insert(Assignments::value_type(event.source, nullptr));
		ControllerDef const*& def = ret.first->second;  // A reference to the value inside the map
		if (!ret.second) return def;  // Source already assigned, just return it.
		std::string devName = m_hw[event.source.type]->getName(event.source.device);
		// Find a matching ControllerDef
		for (ControllerDefs::const_iterator it = m_controllerDefs.begin(); it != m_controllerDefs.end() && !def; ++it) {
			if (it->second.matches(event, devName)) def = &it->second;
		}
		if (def) std::clog << "controllers/info: Assigned " << event.source << " as " << def->name << std::endl;
		else if (!event.source.isKeyboard()) std::clog << "controllers/warning: \"" << devName << " " << event.source << "\" not found from controllers.xml. Please report a bug if this is a game controller." << std::endl;
		return def;
	}
	/// Handle an incoming hardware event
	void pushHWEvent(Event event) {
		ControllerDef const* def = assign(event);
		if (!def) {
			pushMappedEvent(event);  // This is for keyboard events mainly (they have no ControllerDefs)
			return;
		}
		event.time -= clockDur(def->latency * 1s);
		event.devType = def->devType;
		// Mapping from controllers.xml
		auto it = def->mapping.find(event.hw);
		if (it != def->mapping.end()) {
			Event ev = event;  // Make a copy for fiddling
			bool handled = false;
			ButtonMap const& m = it->second;
			double value = ev.value;
			unsigned val = value;
			ev.button = m.map; if (pushMappedEvent(ev)) handled = true;
			ev.button = m.positive; ev.value = clamp(value); if (pushMappedEvent(ev)) handled = true;
			ev.button = m.negative; ev.value = clamp(-value); if (pushMappedEvent(ev)) handled = true;
			if (hwIsHat.matches(ev.hw)) {
				ev.button = m.up; ev.value = !!(val & SDL_HAT_UP); if (pushMappedEvent(ev)) handled = true;
				ev.button = m.down; ev.value = !!(val & SDL_HAT_DOWN); if (pushMappedEvent(ev)) handled = true;
				ev.button = m.left; ev.value = !!(val & SDL_HAT_LEFT); if (pushMappedEvent(ev)) handled = true;
				ev.button = m.right; ev.value = !!(val & SDL_HAT_RIGHT); if (pushMappedEvent(ev)) handled = true;
			}
			if (!handled) std::clog << "controllers/info: ignored " << event << std::endl;  // No matching attribute in controllers.xml
		} else {
			std::clog << "controllers/warning: not mapped " << event << std::endl;  // No matching button/axis/hat element in controllers.xml
		}
	}
	bool pushMappedEvent(Event& ev) {
		if (ev.button == GENERIC_UNASSIGNED) return false;
		if (!valueChanged(ev)) return false;  // Avoid repeated or other useless events
		std::clog << "controllers/debug: processing " << ev << std::endl;
		ev.nav = navigation(ev);
		// Emit nav event (except if device is currently registered for events)
		if (ev.nav != NAV_NONE) {
			NavEvent ne(ev);
			// Menu navigation mapping
			{
				bool vertical = (ev.devType == DEVTYPE_GUITAR);
				if (ne.button == NAV_UP) ne.menu = (vertical ? NAVMENU_A_PREV : NAVMENU_B_PREV);
				else if (ne.button == NAV_DOWN) ne.menu = (vertical ? NAVMENU_A_NEXT : NAVMENU_B_NEXT);
				else if (ne.button == NAV_LEFT) ne.menu = (vertical ? NAVMENU_B_PREV : NAVMENU_A_PREV);
				else if (ne.button == NAV_RIGHT) ne.menu = (vertical ? NAVMENU_B_NEXT : NAVMENU_A_NEXT);
			}
			if (ev.value != 0.0) {
				m_navEvents.push_back(ne);
				if (ne.button >= NAV_REPEAT) m_navRepeat.insert(std::make_pair(ne.button, ne));
			} else {
				if (ne.button >= NAV_REPEAT) m_navRepeat.erase(ne.button);
			}
		}
		if (m_eventsEnabled) {
			// Emit Event and construct a new Device first if needed
			DevicePtr ptr = m_devices[ev.source].lock();
			if (!ptr) {
				ptr = std::make_shared<Device>(ev.source, ev.devType);
				m_orphans[ev.source] = ptr;
				m_devices[ev.source] = ptr;
			}
			ptr->pushEvent(ev);
		}
		return true;
	}
	/// Test if button's value has changed since the last call to this function
	bool valueChanged(Event const& ev) {
		// Find the matching UniqueButton or add a new one with NaN value
		auto res = m_values.insert(std::make_pair(UniqueButton(ev.source, ev.button), getNaN()));
		double& value = res.first->second;
		// Check and update value
		if (value == ev.value) return false;
		value = ev.value;
		return true;
	}
};

// External API simply wraps self (pImpl)
Controllers::Controllers(): self(new Controllers::Impl()) {}
Controllers::~Controllers() {}
bool Controllers::getNav(NavEvent& ev) { return self->getNav(ev); }
DevicePtr Controllers::registerDevice(SourceId const& source) { return self->registerDevice(source); }
void Controllers::enableEvents(bool state) { self->enableEvents(state); }
void Controllers::process(Time now) { self->process(now); }
bool Controllers::pushEvent(SDL_Event const& ev, Time t) { return self->pushEvent(ev, t); }

bool Device::getEvent(Event& ev) {
	if (m_events.empty()) return false;
	ev = m_events.front();
	m_events.pop_front();
	return true;
}

void Device::pushEvent(Event const& ev) {
	m_events.push_back(ev);
}


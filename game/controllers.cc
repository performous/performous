#include "controllers.hh"

#include "fs.hh"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/smart_ptr/weak_ptr.hpp>
#include <libxml++/libxml++.h>
#include <SDL_joystick.h>
#include <deque>
#include <stdexcept>

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
			var = boost::lexical_cast<T>(a->get_value());
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

}

struct ButtonMap {
	Button map, negative, positive;
	ButtonMap(): map(GENERIC_UNASSIGNED), negative(GENERIC_UNASSIGNED), positive(GENERIC_UNASSIGNED) {}
};

typedef std::map<HWButton, ButtonMap> ButtonMapping;

/// A controller definition from controllers.xml
struct ControllerDef {
	std::string name;
	std::string description;
	SourceType sourceType;
	DevType devType;
	boost::regex deviceRegex;
	MinMax<unsigned> deviceMinMax;
	MinMax<unsigned> channelMinMax;
	ButtonMapping mapping;
	ControllerDef(): sourceType(), devType() {}
	bool matches(Event const& ev, std::string const& devName) const {
		if (ev.source.type != sourceType) return false;
		if (!deviceMinMax.matches(ev.source.device)) return false;
		if (!channelMinMax.matches(ev.source.channel)) return false;
		if (!deviceRegex.empty() && !regex_search(devName, deviceRegex)) return false;
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
	std::deque<DevicePtr> m_orphans;
	std::map<SourceId, boost::weak_ptr<Device> > m_devices;
	bool m_eventsEnabled;

	Impl(): m_eventsEnabled() {
		#define DEFINE_BUTTON(devtype, button, num, nav) m_buttons[DEVTYPE_##devtype][#button] = devtype##_##button;
		#include "controllers-buttons.ii"
		readControllers(getDefaultConfig(fs::path("/config/controllers.xml")));
		readControllers(getConfigDir() / "controllers.xml");
		m_hw[SOURCETYPE_KEYBOARD] = constructKeyboard();
		m_hw[SOURCETYPE_JOYSTICK] = constructJoysticks();
		if (Hardware::midiEnabled()) m_hw[SOURCETYPE_MIDI] = constructMidi();
	}
	
	void readControllers(fs::path const& file) {
		if (!fs::exists(file)) {
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
			throw std::runtime_error(file.string() + ":" + boost::lexical_cast<std::string>(line) + " element " + name + " " + e.message);
		}
	}

	void parseControllers(xmlpp::DomParser const& dom, std::string const& path, SourceType sourceType) {
		xmlpp::NodeSet n = dom.get_document()->get_root_node()->find(path);
		for (xmlpp::NodeSet::const_iterator nodeit = n.begin(), end = n.end(); nodeit != end; ++nodeit) {
			xmlpp::Element& elem = dynamic_cast<xmlpp::Element&>(**nodeit);
			ControllerDef def;
			def.name = getAttribute(elem, "name");
			def.sourceType = sourceType;
			// Device type
			{
				std::string type = getAttribute(elem, "type");
				if (type == "guitar") def.devType = DEVTYPE_GUITAR;
				else if (type == "drums") def.devType = DEVTYPE_DRUMS;
				else if (type == "keytar") def.devType = DEVTYPE_KEYTAR;
				else if (type == "piano") def.devType = DEVTYPE_PIANO;
				else if (type == "dancepad") def.devType = DEVTYPE_DANCEPAD;
				else {
					std::clog << "controllers/warn: " << type << ": Unknown controller type in controllers.xml (skipped)" << std::endl;
					continue;
				}
			}
			xmlpp::NodeSet ns;
			// Device description
			ns = elem.find("description/text()");
			if (ns.size() == 1) def.description = dynamic_cast<xmlpp::TextNode&>(*ns[0]).get_content();
			// Read filtering rules
			ns = elem.find("device");
			if (ns.size() == 1) {
				xmlpp::Element& elem = dynamic_cast<xmlpp::Element&>(*ns[0]);
				std::string regex;
				if (tryGetAttribute(elem, "regex", regex)) def.deviceRegex = regex;
				parse(def.deviceMinMax, elem);
			}
			ns = elem.find("channel");
			if (ns.size() == 1) parse(def.channelMinMax, dynamic_cast<xmlpp::Element&>(*ns[0]));
			// Read button mapping
			ns = elem.find("mapping/*");
			for (xmlpp::NodeSet::const_iterator nodeit2 = ns.begin(), end = ns.end(); nodeit2 != end; ++nodeit2) {
				xmlpp::Element& elem = dynamic_cast<xmlpp::Element&>(**nodeit2);
				HWButton hw;
				if (!tryGetAttribute(elem, "hw", hw)) throw XMLError(elem, "Mandatory attribute hw is missing or invalid");
				if (elem.get_name() == "axis") hw = HWButton(hw + hwIsAxis.min);  // Fix the value for internal axis numbering
				std::string map, negative, positive;
				tryGetAttribute(elem, "map", map);
				tryGetAttribute(elem, "negative", negative);
				tryGetAttribute(elem, "positive", positive);
				ButtonMap& m = def.mapping[hw];
				m.map = findButton(def.devType, map);
				m.negative = findButton(def.devType, negative);
				m.positive = findButton(def.devType, positive);
			}
			// Add/replace the controller definition
			std::clog << "controllers/info: " << (m_controllerDefs.find(def.name) == m_controllerDefs.end() ? "Controller" : "Overriding")
			  << " definition: " << def.name << ": " << def.description << std::endl;
			m_controllerDefs[def.name] = def;
		}
	}
	/// Find button by name, either of given type or of generic type
	Button findButton(DevType type, std::string name) {
		Button button = GENERIC_UNASSIGNED;
		if (name.empty()) return button;
		boost::algorithm::to_upper(name);
		boost::algorithm::replace_all(name, "-", "_");
		// Try getting button first from generic names, then device-specific
		buttonByName(DEVTYPE_GENERIC, name, button) || buttonByName(type, name, button) ||
		  std::clog << "controllers/warn: " << name << ": Unknown button name in controllers.xml." << std::endl;
		return button;
	}
	/// Try to find button of specific type
	bool buttonByName(DevType type, std::string const& name, Button& button) {
		NameToButton const& n2b = m_buttons[type];
		NameToButton::const_iterator it = n2b.find(name);
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
	/// Return the next orphan device from queue, if available
	bool getDevice(DevicePtr& dev) {
		if (m_orphans.empty()) return false;
		dev = *m_orphans.begin();
		m_orphans.erase(m_orphans.begin());
		return true;
	}
	void enableEvents(bool state) {
		m_eventsEnabled = state;
		Hardware::enableKeyboardInstruments(state);
		m_orphans.clear();
		m_devices.clear();
	}
	/// Do internal event processing (poll for MIDI events etc)
	void process(boost::xtime const& now) {
		for (HW::iterator it = m_hw.begin(); it != m_hw.end(); ++it) {
			while (true) {
				Event event;
				event.time = now;
				if (!it->second->process(event)) break;
				pushHWEvent(event);
			}
		}
	}
	/// Handle an incoming SDL event
	bool pushEvent(SDL_Event const& sdlEv, boost::xtime const& t) {
		for (HW::iterator it = m_hw.begin(); it != m_hw.end(); ++it) {
			Event event;
			event.time = t;
			if (it->second->process(event, sdlEv)) { pushHWEvent(event); return true; }
		}
		return false;
	}
	/// Assign event's source a ControllerDef (if not already assigned) and return it
	ControllerDef const* assign(Event const& event) {
		// Attempt insertion (does not override existing values)
		std::pair<Assignments::iterator, bool> ret = m_assignments.insert(Assignments::value_type(event.source, NULL));
		ControllerDef const*& def = ret.first->second;  // A reference to the value inside the map
		if (!ret.second) return def;  // Source already assigned, just return it.
		std::string devName = m_hw[event.source.type]->getName(event.source.device);
		// Find a matching ControllerDef
		for (ControllerDefs::const_iterator it = m_controllerDefs.begin(); it != m_controllerDefs.end() && !def; ++it) {
			if (it->second.matches(event, devName)) def = &it->second;
		}
		std::clog << "controllers/info: Assigned " << event.source << " as " << (def ? def->name : "(none)") << std::endl;
		return def;
	}
	/// Handle an incoming hardware event
	void pushHWEvent(Event ev) {
		ControllerDef const* def = assign(ev);
		if (!def) {
			pushMappedEvent(ev);  // This is for keyboard events mainly (they have no ControllerDefs)
			return;
		}
		// Handle directional controllers (not possible via XML)
		if (hwIsHat.matches(ev.hw)) {
			HWButton val = ev.value;
			ev.button = GENERIC_UP; ev.value = !!(val & SDL_HAT_UP); pushMappedEvent(ev);
			ev.button = GENERIC_LEFT; ev.value = !!(val & SDL_HAT_LEFT); pushMappedEvent(ev);
			ev.button = GENERIC_RIGHT; ev.value = !!(val & SDL_HAT_RIGHT); pushMappedEvent(ev);
			ev.button = GENERIC_DOWN; ev.value = !!(val & SDL_HAT_DOWN); pushMappedEvent(ev);
			return;
		}
		// Mapping from controllers.xml
		ev.devType = def->devType;
		ButtonMapping::const_iterator it = def->mapping.find(ev.hw);
		if (it != def->mapping.end()) {
			ButtonMap const& m = it->second;
			double value = ev.value;
			ev.button = m.map; pushMappedEvent(ev);
			ev.button = m.positive; ev.value = clamp(value); pushMappedEvent(ev);
			ev.button = m.negative; ev.value = clamp(-value); pushMappedEvent(ev);
		}
		else std::clog << "controller-events/warn: " << ev.source << " unmapped hw=" << ev.hw << " " << buttonDebug(ev.devType, ev.button) << " value=" << ev.value << std::endl;
	}
	void pushMappedEvent(Event ev) {
		if (ev.button == GENERIC_UNASSIGNED) return;
		std::clog << "controller-events/info: " << ev.source << " processing " << buttonDebug(ev.devType, ev.button) << " value=" << ev.value << std::endl;
		ev.nav = navigation(ev);
		// Emit nav event
		if (ev.nav != NAV_NONE && ev.value != 0.0) {
			NavEvent ne;
			ne.source = ev.source;
			ne.button = ev.nav;
			// Menu navigation mapping
			{
				bool vertical = (ev.devType == DEVTYPE_GUITAR);
				if (ne.button == NAV_UP) ne.menu = (vertical ? NAVMENU_A_PREV : NAVMENU_B_PREV);
				else if (ne.button == NAV_DOWN) ne.menu = (vertical ? NAVMENU_A_NEXT : NAVMENU_B_NEXT);
				else if (ne.button == NAV_LEFT) ne.menu = (vertical ? NAVMENU_B_PREV : NAVMENU_A_PREV);
				else if (ne.button == NAV_RIGHT) ne.menu = (vertical ? NAVMENU_B_NEXT : NAVMENU_A_NEXT);
			}
			ne.time = ev.time;
			m_navEvents.push_back(ne);
		}
		if (!m_eventsEnabled) return;
		// Emit Event and construct a new Device first if needed
		DevicePtr ptr = m_devices[ev.source].lock();
		if (!ptr) {
			ptr.reset(new Device(ev.source, ev.devType));
			m_orphans.push_back(ptr);
			m_devices[ev.source] = ptr;
		}
		ptr->pushEvent(ev);
	}
};

// External API simply wraps self (pImpl)
Controllers::Controllers(): self(new Controllers::Impl()) {}
Controllers::~Controllers() {}
bool Controllers::getNav(NavEvent& ev) { return self->getNav(ev); }
bool Controllers::getDevice(DevicePtr& dev) { return self->getDevice(dev); }
void Controllers::enableEvents(bool state) { self->enableEvents(state); }
void Controllers::process(boost::xtime const& now) { self->process(now); }
bool Controllers::pushEvent(SDL_Event const& ev, boost::xtime const& t) { return self->pushEvent(ev, t); }

bool Device::getEvent(Event& ev) {
	if (m_events.empty()) return false;
	ev = m_events.front();
	m_events.pop_front();
	return true;
}

void Device::pushEvent(Event const& ev) {
	m_events.push_back(ev);
}


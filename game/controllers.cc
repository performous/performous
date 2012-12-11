#include "controllers.hh"

#include "fs.hh"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
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
	template <typename Numeric> struct MinMax {
		Numeric min, max;
		MinMax():
		  min(std::numeric_limits<Numeric>::min()),
		  max(std::numeric_limits<Numeric>::max())
		{}
		void parseFrom(xmlpp::Element const& elem) {
			tryGetAttribute(elem, "min", min);
			tryGetAttribute(elem, "max", max);
		}
		bool matches(Numeric val) const { return val >= min && val <= max; }
	};
	// Return a NavButton corresponding to an Event
	NavButton navigation(Event const& ev) {
		if (ev.navButton != input::NONE) return ev.navButton;
		if (ev.source.type == SOURCETYPE_KEYBOARD) return input::NONE;  // No translation is needed for keyboard instruments
		if (ev.devType == DEVTYPE_NONE) return input::NONE;
		#define DEFINE_BUTTON(devtype, button, num, nav) if (ev.devType == DEVTYPE_##devtype && ev.id == devtype##_##button) return nav;
		#include "controllers-buttons.ii"
		return input::NONE;
	}

	std::ostream& operator<<(std::ostream& os, SourceId const& source) {
		switch (source.type) {
			case SOURCETYPE_NONE: return os << "(none)";
			case SOURCETYPE_KEYBOARD: return os << "(keyboard " << source.device << " instrument " << source.channel << ")";
			case SOURCETYPE_JOYSTICK: return os << "(joystick " << source.device << ")";
			case SOURCETYPE_MIDI: return os << "(midi " << source.device << " channel " << source.channel << ")";
		}
		return os << "(unknown SOURCETYPE)";
	}
}

typedef std::map<HWButton, Button> ButtonMapping;

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
	bool matches(Event const& ev) const {
		if (ev.source.type != sourceType) return false;
		if (!deviceMinMax.matches(ev.source.device)) return false;
		if (!channelMinMax.matches(ev.source.channel)) return false;
		// TODO: deviceRegex matching (needs device name)
	}
	void mapButton(Event& ev) const {
		ButtonMapping::const_iterator it = mapping.find(ev.hw);
		if (it == mapping.end()) return;
		ev.id = it->second;
	}
};

struct Controllers::Impl {
	typedef std::map<std::string, ControllerDef> ControllerDefs;
	ControllerDefs m_controllerDefs;

	typedef std::map<std::string, Button> NameToButton;
	NameToButton m_buttons[DEVTYPE_N];

	typedef std::map<SourceId, ControllerDef const*> Assignments;
	Assignments m_assignments;
	
	std::vector<Hardware::ptr> m_hw;
	std::deque<NavEvent> m_navEvents;

	Impl() {
		#define DEFINE_BUTTON(devtype, button, num, nav) m_buttons[DEVTYPE_##devtype][#button] = devtype##_##button;
		#include "controllers-buttons.ii"
		readControllers(getDefaultConfig(fs::path("/config/controllers.xml")));
		readControllers(getConfigDir() / "controllers.xml");
		m_hw.push_back(constructKeyboard());
		m_hw.push_back(constructJoysticks());
		if (Hardware::midiEnabled()) m_hw.push_back(constructMidi());
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
				def.deviceMinMax.parseFrom(elem);
			}
			ns = elem.find("channel");
			if (ns.size() == 1) def.channelMinMax.parseFrom(dynamic_cast<xmlpp::Element&>(*ns[0]));
			// Read button mapping
			ns = elem.find("mapping/button");
			for (xmlpp::NodeSet::const_iterator nodeit2 = ns.begin(), end = ns.end(); nodeit2 != end; ++nodeit2) {
				xmlpp::Element& button_elem = dynamic_cast<xmlpp::Element&>(**nodeit2);
				HWButton hw;
				tryGetAttribute(button_elem, "hw", hw);
				Button button = GENERIC_UNASSIGNED;
				std::string map = getAttribute(button_elem, "map");
				if (!map.empty()) {
					boost::algorithm::to_upper(map);
					boost::algorithm::replace_all(map, "-", "_");
					NameToButton const& n2b = m_buttons[def.devType];
					NameToButton::const_iterator mapit = n2b.find(map);
					if (mapit == n2b.end()) throw XMLError(button_elem, map + ": Invalid value for map attribute");
					button = mapit->second;
				}
				def.mapping[hw] = button;
			}
			// Add/replace the controller definition
			std::clog << "controllers/info: " << (m_controllerDefs.find(def.name) == m_controllerDefs.end() ? "Controller" : "Overriding")
			  << " definition: " << def.name << ": " << def.description << std::endl;
			m_controllerDefs[def.name] = def;
		}
	}
	/// Return the next available navigation event from queue, if available
	bool getNav(NavEvent& ev) {
		if (m_navEvents.empty()) return false;
		ev = m_navEvents.front();
		m_navEvents.pop_front();
		return true;
	}
	bool pressed(SourceId const& source, Button button) { return false; }
	/// Do internal event processing (poll for MIDI events etc)
	void process(boost::xtime const& now) {
		for (size_t i = 0; i < m_hw.size(); ++i) {
			while (true) {
				Event event;
				event.time = now;
				if (!m_hw[i]->process(event)) break;
				pushHWEvent(event);
			}
		}
	}
	/// Handle an incoming SDL event
	bool pushEvent(SDL_Event const& sdlEv, boost::xtime const& t) {
		for (size_t i = 0; i < m_hw.size(); ++i) {
			Event event;
			event.time = t;
			if (m_hw[i]->process(event, sdlEv)) return pushHWEvent(event);
		}
		return false;
	}
	/// Assign event's source a ControllerDef (if not already assigned) and return it
	ControllerDef const* assign(Event const& event) {
		ControllerDef const* def = m_assignments[event.source];
		if (def) return def;
		// Find a matching ControllerDef
		for (ControllerDefs::const_iterator it = m_controllerDefs.begin(); it != m_controllerDefs.end() && !def; ++it) {
			if (it->second.matches(event)) def = &it->second;
		}
		std::clog << "controllers/info: Assigned " << event.source << " as " << (def ? def->name : "(none)") << std::endl;
		return m_assignments[event.source] = def;
	}
	/// Handle an incoming hardware event
	bool pushHWEvent(Event event) {
		ControllerDef const* def = assign(event);
		if (def) def->mapButton(event);
		event.navButton = navigation(event);
		if (event.navButton != input::NONE && event.value != 0.0) {
			NavEvent ne;
			ne.source = event.source;
			ne.button = event.navButton;
			ne.time = event.time;
			m_navEvents.push_back(ne);
		}
		return true;
	}
};

// External API simply wraps self (pImpl)
Controllers::Controllers(): self(new Controllers::Impl()) {}
Controllers::~Controllers() {}
bool Controllers::getNav(NavEvent& ev) { return self->getNav(ev); }
double Controllers::pressed(SourceId const& source, Button button) { return self->pressed(source, button); }
void Controllers::process(boost::xtime const& now) { self->process(now); }
bool Controllers::pushEvent(SDL_Event const& ev, boost::xtime const& t) { return self->pushEvent(ev, t); }



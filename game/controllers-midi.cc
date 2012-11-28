#ifdef USE_PORTMIDI

#include "controllers.hh"
#include "portmidi.hh"
#include "fs.hh"
#include <boost/lexical_cast.hpp>
#include <libxml++/libxml++.h>

namespace {
	struct XMLError {
		XMLError(xmlpp::Element& e, std::string msg): elem(e), message(msg) {}
		xmlpp::Element& elem;
		std::string message;
	};
	std::string getAttribute(xmlpp::Element& elem, std::string const& attr) {
		xmlpp::Attribute* a = elem.get_attribute(attr);
		if (!a) throw XMLError(elem, "attribute " + attr + " not found");
		return a->get_value();
	}
}

extern input::Instruments g_instruments;

using namespace input;

typedef std::map<unsigned, unsigned> Map;

class MidiDevice {
public:
	MidiDevice();
	void process();
private:
	pm::Input stream;
	unsigned int devnum;
	input::Event event;
	Map map;
};

void readConfig(Map& map, fs::path const& file) {
	if (!fs::exists(file)) {
		std::clog << "controllers/info: Skipping " << file << " (not found)" << std::endl;
		return;
	}
	std::clog << "controllers/info: Parsing " << file << std::endl;
	xmlpp::DomParser domParser(file.string());
	try {
		xmlpp::NodeSet n = domParser.get_document()->get_root_node()->find("/mididrums/note");
		for (xmlpp::NodeSet::const_iterator nodeit = n.begin(), end = n.end(); nodeit != end; ++nodeit) {
			xmlpp::Element& elem = dynamic_cast<xmlpp::Element&>(**nodeit);
			int id = boost::lexical_cast<int>(getAttribute(elem, "id"));
			std::string value = getAttribute(elem, "value");
			std::string name = getAttribute(elem, "name");

			// lets erase old mapping for this note
			map.erase(id);

			if(value == "kick") map[id] = input::KICK_BUTTON;
			else if(value == "red") map[id] = input::RED_TOM_BUTTON;
			else if(value == "yellow") map[id] = input::YELLOW_TOM_BUTTON;
			else if(value == "blue") map[id] = input::BLUE_TOM_BUTTON;
			else if(value == "green") map[id] = input::GREEN_TOM_BUTTON;
			else if(value == "orange") map[id] = input::ORANGE_TOM_BUTTON;
		}
	} catch (XMLError& e) {
		int line = e.elem.get_line();
		std::string name = e.elem.get_name();
		throw std::runtime_error(file.string() + ":" + boost::lexical_cast<std::string>(line) + " element " + name + " " + e.message);
	}
}

MidiDevice::MidiDevice(): stream(pm::findDevice(true, config["game/midi_input"].s())), devnum(0x8000) {
	readConfig(map, getDefaultConfig(fs::path("/config/mididrums.xml")));
	readConfig(map, getConfigDir() / "mididrums.xml");

	while (detail::devices.find(devnum) != detail::devices.end()) ++devnum;
	detail::devices.insert(std::make_pair(devnum, detail::InputDevPrivate(g_instruments.find("DRUMS_GUITARHERO")->second)));
	event.type = Event::PRESS;
	for (unsigned int i = 0; i < BUTTONS; ++i) event.pressed[i] = false;
}

void MidiDevice::process() {
	PmEvent ev;
	while (Pm_Read(stream, &ev, 1) == 1) {
		unsigned char evnt = ev.message & 0xF0;
		unsigned char note = ev.message >> 8;
		unsigned char vel  = ev.message >> 16;
		unsigned chan = (ev.message & 0x0F) + 1;  // It is conventional to use one-based indexing
		if (chan < 10 || chan > 11) continue; // only accept channels 10-11 (percussion in GM and GM level 2)
		if (evnt != 0x90) continue; // 0x90 = any channel note-on
		if (vel  == 0x00) continue; // velocity 0 is often used instead of note-off
		Map::const_iterator it = map.find(note);
		if (it == map.end()) {
			std::clog << "controller-midi/warn: Unassigned MIDI drum event: ch=" << unsigned(chan) << " note=" << unsigned(note) << std::endl;
			continue;
		}
		event.button = it->second;
		event.time = now();
		std::clog << "controller-midi/info: Processed MIDI drum event: ch=" << unsigned(chan) << " note=" << unsigned(note) << " button=" << event.button << std::endl;
		for (unsigned int i = 0; i < BUTTONS; ++i) event.pressed[i] = false;
		event.pressed[it->second] = true;
		detail::devices.find(devnum)->second.addEvent(event);
	}
}

#endif


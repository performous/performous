#ifdef USE_PORTMIDI

#include "controllers.hh"
#include "fs.hh"
#include "portmidi.hh"
#include <regex>
#include <unordered_map>
#include <sstream>

namespace input {

	class Midi: public Hardware {
	public:
		Midi() {
			std::regex re(config["game/midi_input"].s());
			for (int dev = 0; dev < Pm_CountDevices(); ++dev) {
				try {
					PmDeviceInfo const* info = Pm_GetDeviceInfo(dev);
					if (!info->input) continue;  // Not an input device
					if (info->opened) continue;  // Already opened
					std::string name = getName(dev);
					if (!regex_search(name, re)) continue;
					// Now actually open the device
					m_streams.emplace(dev, std::unique_ptr<pm::Input>(new pm::Input(dev)));
					std::clog << "controller-midi/info: Opened MIDI device " << name << std::endl;
				} catch (std::runtime_error& e) {
					std::clog << "controller-midi/warning: " << e.what() << std::endl;
				}
			}
		}
		std::string getName(unsigned dev) const override {
			PmDeviceInfo const* info = Pm_GetDeviceInfo(dev);
			if (!info) throw std::logic_error("Invalid MIDI device requested in Midi::getName");
			std::ostringstream name;
			name << dev << ": " << info->name;
			return name.str();
		}
		bool process(Event& event) override {
			PmEvent ev;
			for (auto it = m_streams.begin(); it != m_streams.end(); ++it) {
				if (Pm_Read(*it->second, &ev, 1) != 1) continue;
				unsigned char evnt = ev.message & 0xF0;
				unsigned char note = ev.message >> 8;
				unsigned char vel  = ev.message >> 16;
				unsigned chan = (ev.message & 0x0F) + 1;  // It is conventional to use one-based indexing
				if (evnt == 0x80 /* NOTE OFF */) { evnt = 0x90; vel = 0; }  // Translate NOTE OFF into NOTE ON with zero-velocity
				if (evnt != 0x90 /* NOTE ON */) continue;  // Ignore anything that isn't NOTE ON/OFF
				std::clog << "controller-midi/info: MIDI NOTE ON/OFF event: ch=" << unsigned(chan) << " note=" << unsigned(note) << " vel=" << unsigned(vel) << std::endl;
				event.source = SourceId(SOURCETYPE_MIDI, it->first, chan);
				event.hw = note;
				event.value = vel / 127.0;
				return true;
			}
			return false;
		}
	private:
		pm::Initialize m_init;
		std::unordered_map<unsigned, std::unique_ptr<pm::Input>> m_streams;
	};

	Hardware::ptr constructMidi() { return Hardware::ptr(new Midi()); }
	bool Hardware::midiEnabled() { return true; }
}

#else

#include "controllers.hh"

namespace input {
	Hardware::ptr constructMidi() { return Hardware::ptr(); }
	bool Hardware::midiEnabled() { return false; }
}

#endif

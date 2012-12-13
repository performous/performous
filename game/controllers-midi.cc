#ifdef USE_PORTMIDI

#include "controllers.hh"
#include "portmidi.hh"
#include "fs.hh"
#include <boost/lexical_cast.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace input {

	class Midi: public Hardware {
	public:
		Midi() {
			// TODO: pm::findDevice(true, config["game/midi_input"].s() not used
			for (int dev = 0; dev < Pm_CountDevices(); ++dev) {
				try {
					m_streams.push_back(new pm::Input(dev));
					std::clog << "controller-midi/info: Opened MIDI device " << dev << ": " << getName(dev) << std::endl;
				} catch (std::runtime_error& e) {
					std::clog << "controller-midi/warn: " << e.what() << std::endl;
					m_streams.push_back(NULL);
				}
			}
		}
		std::string getName(unsigned dev) const {
			PmDeviceInfo const* info = Pm_GetDeviceInfo(dev);
			return info->name;
		}
		bool process(Event& event) {
			PmEvent ev;
			for (size_t dev = 0; dev < m_streams.size(); ++dev) {
				if (m_streams.is_null(dev)) continue;
				if (Pm_Read(m_streams[dev], &ev, 1) != 1) continue;
				unsigned char evnt = ev.message & 0xF0;
				unsigned char note = ev.message >> 8;
				unsigned char vel  = ev.message >> 16;
				unsigned chan = (ev.message & 0x0F) + 1;  // It is conventional to use one-based indexing
				if (evnt == 0x80 /* NOTE OFF */) { evnt = 0x90; vel = 0; }  // Translate NOTE OFF into NOTE ON with zero-velocity
				if (evnt != 0x90 /* NOTE ON */) continue;  // Ignore anything that isn't NOTE ON/OFF
				std::clog << "controller-midi/info: MIDI NOTE ON/OFF event: ch=" << unsigned(chan) << " note=" << unsigned(note) << " vel=" << unsigned(vel) << std::endl;
				event.source = SourceId(SOURCETYPE_MIDI, dev, chan);
				event.hw = note;
				event.value = vel / 127.0;
				return true;
			}
			return false;
		}
	private:
		boost::ptr_vector<boost::nullable<pm::Input> > m_streams;
	};

	Hardware::ptr constructMidi() { return Hardware::ptr(new Midi()); }
	bool Hardware::midiEnabled() { return true; }
}

#else

namespace input {
	Hardware::ptr constructMidi() { return Hardware::ptr(); }
	bool Hardware::midiEnabled() { return false; }
}

#endif


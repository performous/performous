#pragma once

#include <portmidi.h>
#include <stdexcept>

namespace pm {
	struct Initialize {
		Initialize() { Pm_Initialize(); }
		~Initialize() { Pm_Terminate(); }
	};
	class Stream {
	public:
		operator PortMidiStream*() { return m_handle; }
		
	protected:
		PortMidiStream* m_handle;
		Stream(): m_handle() {}
		~Stream() { if (m_handle) Pm_Close(m_handle); }
	};
	
	class Input: public Stream {
	public:
		Input(int devId) {
			PmError err = Pm_OpenInput(&m_handle, devId, 0, 1024, 0, 0);
			if (err) throw std::runtime_error("Pm_OpenInput failed");
		}
	};
}


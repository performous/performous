#pragma once

#include <portmidi.h>
#include <cstdint>
#include <stdexcept>
#include <iostream>

namespace pm {
	struct Initialize {
		Initialize() { Pm_Initialize(); }
		~Initialize() { Pm_Terminate(); }
	};
	class Stream {
	public:
		operator PortMidiStream*() { return m_handle; }
		Stream(Stream const&) = delete;
	protected:
		PortMidiStream* m_handle;
		Stream(): m_handle() {}
		void abort() { Pm_Abort(m_handle); m_handle = nullptr; }
		~Stream() { if (m_handle) Pm_Close(m_handle); }
	};

	class Input: public Stream {
	public:
		Input(int devId) {
			// Errors must be handled here because otherwise PortMidi will just exit() the program...
			if (devId < 0 || devId >= Pm_CountDevices()) throw std::runtime_error("Invalid PortMidi device ID");
			PmDeviceInfo const* info = Pm_GetDeviceInfo(devId);
			if (!info->input) throw std::runtime_error(std::string(info->name) + ": The PortMidi device is an output device (input device needed)");
			if (info->opened) throw std::runtime_error(std::string(info->name) + ": The PortMidi device is already open");
			PmError err = Pm_OpenInput(&m_handle, devId, 0, 1024, 0, 0);
			if (err) throw std::runtime_error(std::string(info->name) + ": Pm_OpenInput failed");
		}
	};
}

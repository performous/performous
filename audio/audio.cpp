#include "audio_dev.hpp"
#include <ostream>
#include <limits>

namespace da {
	const std::size_t settings::low = std::numeric_limits<std::size_t>::min();
	const std::size_t settings::high = std::numeric_limits<std::size_t>::max();

	namespace {
		std::string none = "none";
	}

	record::devlist_t record::devices() {
		devlist_t l(record_plugin::begin(), record_plugin::end());
		l.push_back(devinfo(none, "No device. Will not receive any audio data."));
		return l;
	}

	record::record(settings& s): m_handle() {
		if (s.device() == none) return;
		if (record_plugin::empty()) throw std::runtime_error("No recording devices installed");
		if (s.device().empty()) {
			// Try all normal devices
			for (record_plugin::iterator it = record_plugin::begin(); it != record_plugin::end(); ++it) {
				if (it->special()) continue;
				try {
					s.debug(">>> Recording from " + it->name());
					m_handle = it(s);
					s.set_device(it->name());
					return;
				} catch (std::exception& e) {
					s.debug(std::string("-!- ") + e.what());
				}
			}
			throw std::runtime_error("No recording devices could be used");
		} else {
			s.debug(">>> Using recording device " + s.device());
			try {
				m_handle = record_plugin::find(s.device())(s);
			} catch (record_plugin::invalid_key_error&) {
				throw std::runtime_error("Recording device " + s.device() + " not found");
			}
		}
	}

	record::~record() { delete m_handle; }

	playback::devlist_t playback::devices() {
		devlist_t l(playback_plugin::begin(), playback_plugin::end());
		l.push_back(devinfo(none, "No device. Will not emit any audio data."));
		return l;
	}

	playback::playback(settings& s): m_handle() {
		if (s.device() == none) return;
		if (playback_plugin::empty()) throw std::runtime_error("No playback devices installed");
		if (s.device().empty()) {
			// Try all normal devices
			for (playback_plugin::iterator it = playback_plugin::begin(); it != playback_plugin::end(); ++it) {
				if (it->special()) continue;
				try {
					s.debug(">>> Playing with " + it->name());
					m_handle = it(s);
					s.set_device(it->name());
					return;
				} catch (std::exception& e) {
					s.debug(std::string("-!- ") + e.what());
				}
			}
			throw std::runtime_error("No playback devices could be used");
		} else {
			s.debug(">>> Using playback device " + s.device());
			try {
				m_handle = playback_plugin::find(s.device())(s);
			} catch (record_plugin::invalid_key_error&) {
				throw std::runtime_error("Playback device " + s.device() + " not found");
			}
		}
	}

	playback::~playback() { delete m_handle; }
}


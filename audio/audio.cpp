#include "audio_dev.hpp"
#include <ostream>
#include <limits>

namespace da {
	const std::size_t settings::low = std::numeric_limits<std::size_t>::min();
	const std::size_t settings::high = std::numeric_limits<std::size_t>::max();

	namespace {
		template <typename T> std::string get_first(T const& pair) { return pair.first; }
		char const* none = "none";
	}

	std::vector<std::string> record::devices() {
		dev::map_t& m = dev::map();
		std::vector<std::string> v(m.size());
		std::transform(m.begin(), m.end(), v.begin(), get_first<dev::map_t::value_type>);
		v.push_back(none);
		return v;
	}

	record::record(settings& s): handle() {
		if (s.device == none) { s.device = none; return; }
		dev::map_t& m = dev::map();
		if (m.empty()) throw std::runtime_error("No recording devices installed");
		dev::map_t::const_iterator it;
		if (s.device.empty()) {
			// Try all normal devices
			for (it = m.begin(); it != m.end() && !it->first.empty() && it->first[0] != '~'; ++it) {
				try {
					if (s.debug) *s.debug << ">>> Recording from " << it->first << std::endl;
					handle = it->second(s);
					s.device = it->first;
					return;
				} catch (std::exception& e) {
					if (s.debug) *s.debug << "-!- " << e.what() << std::endl;
				}
			}
			throw std::runtime_error("No recording devices could be used");
		} else {
			it = m.find(s.device);
			if (it == m.end()) throw std::runtime_error("Recording device " + s.device + " not found");
			if (s.debug) *s.debug << ">>> Using recording device " << it->first << std::endl;
			handle = it->second(s);
		}
	}

	record::~record() {	delete handle; }

}


#include "portaudio.hpp"

#include "../platform.hh"

namespace portaudio {

	struct AudioBackend::AudioDevices {
		/// Constructor gets the PA devices into a vector
		AudioDevices(PaHostApiTypeId backend) {
			auto backendIndex = Pa_HostApiTypeIdToHostApiIndex(backend);
			for (unsigned i = 0, end = Pa_GetHostApiInfo(backendIndex)->deviceCount; i != end; ++i) {
				PaDeviceInfo const* info = Pa_GetDeviceInfo(Pa_HostApiDeviceIndexToDeviceIndex(backendIndex, i));
				if (!info) continue;
				std::string name = UnicodeUtil::convertToUTF8(info->name);
				/// Omit some useless legacy devices of PortAudio/ALSA from our list
				for (auto const& dev: { "front", "surround40", "surround41", "surround50", "surround51", "surround71", "iec958", "spdif", "dmix" }) {
					if (name.find(dev) != std::string::npos) name.clear();
				}
				if (name.empty()) continue;  // No acceptable device found
							     // Verify that the name is unique (haven't seen duplicate names occur though)
				std::string n = name;
				while (true) {
					int num = 1;
					for (auto& dev: devices) if (dev.name == n) goto rename;
					break;
rename:
					std::ostringstream oss;
					oss << name << " #" << ++num;
					n = oss.str();
				};
				devices.push_back(DeviceInfo(i, name, info->maxInputChannels, info->maxOutputChannels, Pa_HostApiDeviceIndexToDeviceIndex(backendIndex, i)));
			}
			for (auto& dev: devices) {
				// Array of regex - replacement pairs
				static char const* const replacements[][2] = {
					{ "\\(hw:\\d+,", "(hw:," },  // Remove ALSA device numbers
					{ " \\(.*\\)", "" },  // Remove the parenthesis part entirely
				};
				for (auto const& rep: replacements) {
					std::string flex = std::regex_replace(dev.flex, std::regex(rep[0]), rep[1]);
					if (flex == dev.flex) continue;  // Nothing changed
									 // Verify that flex doesn't find any wrong devices
					bool fail = false;
					try {
						if (find(flex, false, 0).idx != dev.idx) fail = true;
					} catch (...) {}  // Failure to find anything is success
					if (!fail) dev.flex = flex;
				}
			}
		}
		/// Get a printable dump of the devices
		std::string dump() const {
			std::ostringstream oss;
			for (auto const& d: devices) { oss << "    #" << d.idx << " " << d.desc() << std::endl; }
			return oss.str();
		}
		DeviceInfo const& find(std::string const& name, bool output, unsigned num) {
			if (name.empty()) { return findByChannels(output, num); }
			// Try name search with full match
			for (auto const& dev: devices) {
				if ((output ? dev.out : dev.in) < num) continue;
				if (dev.name == name) return dev;
			}
			// Try name search with partial/flexible match
			for (auto const& dev: devices) {
				if ((output ? dev.out : dev.in) < num) continue;
				if (dev.name.find(name) != std::string::npos) { return dev; }
				if (dev.flex.find(name) != std::string::npos) { return dev; }
			}
			throw std::runtime_error("No such device.");
		}
		DeviceInfo const& findByChannels(bool output, unsigned num) {
			for (auto const& dev: devices) {
				unsigned reqChannels = output ? dev.out : dev.in;
				if (reqChannels >= num) { return dev;  }
			}
			throw std::runtime_error("No such device.");
		}
		DeviceInfos devices;
	};

	AudioBackend::AudioBackend(const PaHostApiInfo &pa_info) : pa_info(pa_info), audio_devices(std::make_unique<AudioDevices>(pa_info.type)) {}

	// All default here because of p-impl
	AudioBackend::~AudioBackend() = default;
	AudioBackend::AudioBackend(AudioBackend &&) = default;
	AudioBackend &AudioBackend::operator=(AudioBackend &&) = default;

		/// Get a printable dump of the devices
	AudioBackendFactory::AudioBackendFactory() {
		for (int i = 0; i < Pa_GetHostApiCount(); ++i) {
			backends.emplace_back(Pa_GetHostApiInfo(i));
		}
		if (backends.empty()) throw std::runtime_error("No suitable audio backends found."); // Check specifically for 0 because it returns a negative error code if Pa is not initialized.

		dump();
	};

        DeviceInfos &AudioBackend::getDevices() { return audio_devices->devices;}

	DeviceInfo const& AudioBackend::find(std::string const& name, bool output, unsigned num) {
		return audio_devices->find(name, output, num);
	}

	AudioBackend AudioBackendFactory::makeBackend(std::string backendName) {
                if (backendName == "Auto")
                    backendName = Platform::defaultBackEnd();
		for (auto const& b: backends)
			if (UnicodeUtil::convertToUTF8(b->name) == backendName)
				return { *b };
                throw std::runtime_error("Invalid backend name '" + backendName + "' provided.");
	}

	void AudioBackendFactory::dump() const {
		std::cout << "audio/info: PortAudio backends:\n";
		size_t i = 0;
		for (auto const& b: backends) { std::cout << "  #" << i++ << ": " << UnicodeUtil::convertToUTF8(b->name) << " (" << b->deviceCount << " devices):\n"; }
	}

	std::vector<std::string> AudioBackendFactory::getBackendsNames() const {
		std::vector<std::string> names;
		for (auto const& temp: backends) {
			names.emplace_back(UnicodeUtil::convertToUTF8(temp->name));
		}
		return names;
	}

	struct AudioBackendFactory::Init {
		Init() { PORTAUDIO_CHECKED(Pa_Initialize, ()); }
		~Init() { Pa_Terminate(); }
	};
	AudioBackendFactory::Init AudioBackendFactory::init;
} // portaudio

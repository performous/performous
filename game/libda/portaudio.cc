#include "portaudio.hpp"

namespace portaudio {
	AudioBackendFactory::AudioBackendFactory() {
		for (int i = 0; i < Pa_GetHostApiCount(); ++i) {
			backends.emplace_back(Pa_GetHostApiInfo(i));
		}
		if (backends.empty()) throw std::runtime_error("No suitable audio backends found."); // Check specifically for 0 because it returns a negative error code if Pa is not initialized.

		dump();
	};

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

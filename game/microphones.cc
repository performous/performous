#include "microphones.hh"

namespace {
	std::unordered_map<std::string, Color> MicrophoneConfigs{
		{"blue", Color(0.0f, 43.75f / 255.0f, 1.0f, 1.0f)},
		{"red", Color(1, 0.0f, 0.0f, 1.0f)},
		{"green", Color(0.0f, 1.0f, 0.0f, 1.0f)},
		{"yellow", Color(1.0f, 1.0f, 0.0f, 1.0f)},
		{"fuchsia", Color(1.0f, 0.06f, 127 / 255.0f, 1.0f)},
		{"orange", Color(1.0f, 52.0f / 255.0f, 0.0f, 1.0f)},
		{"purple", Color(63.0f / 255.0f, 0.0f, 1.0f, 1.0f)},
		{"aqua", Color(0.0f, 1.0f, 1.0f, 1.0f)},
		{"white", Color(1.0f, 1.0f, 1.0f, 1.0f)},
		{"gray", Color(24.0f / 255.0f, 24.0f / 255.0f, 24.0f / 255.0f, 1.0f)},
		{"black", Color(3.0f / 255.0f, 3.0f / 255.0f, 3.0f / 255.0f, 1.0f)}
	};
}

std::unordered_map<std::string, Color> getMicrophoneConfig() {
		return MicrophoneConfigs;
}

Color getMicrophoneColor(std::string const& name) {
	try {
		auto color = MicrophoneConfigs.at(name);
		return color;
	}
	catch(std::out_of_range const&) {
		return Color(0.5f, 0.5f, 0.5f, 1.0f);
	}
}

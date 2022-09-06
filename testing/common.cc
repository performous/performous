#include "common.hh"

#include <fstream>

std::vector<float> loadRaw(std::string const& filepath) {
	auto buffer = std::vector<float>();
	auto file = std::ifstream(filepath);

	while(file) {
		auto sample = 0.f;

		file.read(reinterpret_cast<char*>(&sample), 4);

		buffer.emplace_back(sample);
	}

	return buffer;
}


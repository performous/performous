#include "guitar_strings.hh"

#include <cmath>
#include <stdexcept>
#include <vector>

namespace {
	const std::vector<Frequency> standardTuningFreqs = { 82.41f, 110.00f, 146.83f, 196.00f, 246.94f, 329.63f };

	StringName findClosestString(Frequency frequency) {
		auto closestStringIndex = size_t(0);
		auto minDifference = std::numeric_limits<Frequency>::max();

		// Iterate through standard tuning frequencies and find the closest one
		for (size_t i = 0; i < standardTuningFreqs.size(); ++i) {
			auto difference = std::abs(frequency - standardTuningFreqs[i]);

			if (difference < minDifference) {
				minDifference = difference;
				closestStringIndex = i;
			}
		}

		return static_cast<StringName>(closestStringIndex);
	}
}

StringName GuitarStrings::getString(Frequency frequency) const {
	return findClosestString(frequency);
}

Frequency GuitarStrings::getFrequency(StringName string, int fret) const {
	auto const base = getBaseFrequency(string);
	auto const frequency = base * std::pow(2.f, static_cast<Frequency>(fret) / 12.f);

	return frequency;
}

Frequency GuitarStrings::getBaseFrequency(StringName string) const {
	switch (string)
	{
	case StringName::E_Low:
		return 82.41f;
	case StringName::A:
		return 110.00f;
	case StringName::D:
		return 146.83f;
	case StringName::G:
		return 196.00f;
	case StringName::B:
		return 246.94f;
	case StringName::E_High:
		return 329.63f;
	default:
		throw std::logic_error("Unknown string name");
	}
}

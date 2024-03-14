#include "guitar_strings.hh"

#include <cmath>
#include <stdexcept>

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

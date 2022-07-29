#include "string.hh"

#include <cmath>

namespace {
	float const step = 1.059463094359f;
}

String::String(float frequency)
: m_frequency(frequency) {
}

float String::getFrequency(Fret fret, bool inverse) const {
	if(fret == FretNone)
		return 0;
	if(fret == 0)
		return inverse ? 0 : m_frequency;

	auto frequency = m_frequency * std::pow(step, fret);

	if(!inverse)
		return frequency;
	if(fret <= 1)
		return 0;

	return m_frequency / (1 - pow(2, -static_cast<float>(fret - 1) / 12.f));
}

bool String::hasInverseFrequency(Fret fret) const {
	if(fret == FretNone)
		return false;

	return fret >= 2;
}

StringPlay::StringPlay(const String& string, const Fret& fret)
: m_string(string), m_fret(fret) {
}

float StringPlay::getFrequency(bool inverse) const {
	return m_string.getFrequency(m_fret, inverse);
}

bool StringPlay::hasInverseFrequency() const {
	return m_fret >= 2;
}

bool StringPlay::operator<(const StringPlay& other) const {
	return getFrequency() < other.getFrequency();
}


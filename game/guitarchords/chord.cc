#include "chord.hh"

#include <stdexcept>

Chord::Chord(std::string const& name, std::set<float> const& frequencies)
: m_name(name), m_frequencies({frequencies}) {
}

const std::string & Chord::getName() const {
	return m_name;
}

Chord& Chord::addAlternate(std::set<float> const& frequencies) {
	m_frequencies.emplace_back(frequencies);

	return *this;
}

Chord::operator std::set<float> const&() const {
	return getFrequencies();
}

std::set<float> const& Chord::getFrequencies() const {
	if(m_frequencies.empty())
		throw std::logic_error("Chord::getFrequencies: Chord is empty!");

	return m_frequencies[0];
}

size_t Chord::countAlternatives() const {
	return m_frequencies.size() - 1;
}

std::set<float> const& Chord::getAlternative(size_t n) const {
	return m_frequencies.at(n + 1);
}

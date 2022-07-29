#include "chord.hh"

#include <stdexcept>

Chord::Chord(std::string const& name, std::vector<Fret> const& frets)
: m_name(name), m_frets(frets) {
}

const std::string & Chord::getName() const {
	return m_name;
}
/*
Chord& Chord::addAlternate(std::set<float> const& frequencies) {
	m_frequencies.emplace_back(frequencies);

	return *this;
}
*/
Chord::operator std::vector<Fret> const&() const {
	return getFrets();
}

bool Chord::operator==(Chord const& other) const {
	return getName() == other.getName();
}

bool Chord::operator!=(Chord const& other) const {
	return getName() != other.getName();
}

bool Chord::operator<(Chord const& other) const {
	return getName() < other.getName();
}

bool Chord::operator>(Chord const& other) const {
	return getName() > other.getName();
}

std::vector<Fret> const& Chord::getFrets() const {
	return m_frets;
}
/*
size_t Chord::countAlternatives() const {
	return m_frequencies.size() - 1;
}

std::set<float> const& Chord::getAlternative(size_t n) const {
	return m_frequencies.at(n + 1);
}
*/

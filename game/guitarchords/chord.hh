#pragma once

#include <set>
#include <string>
#include <vector>

#include "string.hh"

class Chord {
public:
	Chord(std::string const& name, std::vector<Fret> const&);

	std::string const& getName() const;

	//Chord& addAlternate(std::set<float> const& frequencies);

	operator std::vector<Fret> const&() const;
	bool operator==(Chord const&) const;
	bool operator!=(Chord const&) const;
	bool operator<(Chord const&) const;
	bool operator>(Chord const&) const;

	std::vector<Fret> const& getFrets() const;

	//size_t countAlternatives() const;
	//std::set<float> const& getAlternative(size_t n = 0) const;

private:
	std::string m_name;
	std::vector<Fret> m_frets;
};


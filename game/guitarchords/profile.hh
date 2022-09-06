#pragma once

#include <string>
#include <vector>

#include "chordmatch.hh"

class Profile {
public:
	using Factors = std::vector<std::array<float, 6>>;

	Profile(std::string const& name, Factors const& manitudeFactors, Factors const& inverseManitudeFactors);

	std::string const& getName() const;

	Factors const& getManitudeFactors() const;
	Factors const& getInverseManitudeFactors() const;

	void adjust(ChordMatch&) const;

private:
	std::string m_name;
	Factors m_manitudeFactors;
	Factors m_inverseManitudeFactors;
};

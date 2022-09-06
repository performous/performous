#include "profile.hh"

Profile::Profile(std::string const& name, Factors const& manitudeFactors, Factors const& inverseManitudeFactors)
: m_name(name), m_manitudeFactors(manitudeFactors), m_inverseManitudeFactors(inverseManitudeFactors) {
}

const std::string & Profile::getName() const {
	return m_name;
}

Profile::Factors const& Profile::getManitudeFactors() const {
	return m_manitudeFactors;
}

Profile::Factors const& Profile::getInverseManitudeFactors() const {
	return m_inverseManitudeFactors;
}

void Profile::adjust(ChordMatch& match) const {
	for(auto i = 0U; i < match.magnitudes.size(); ++i) {
		for(auto j = 0U; j < match.magnitudes[i].size(); ++j)
			match.magnitudes[i][j] *= m_manitudeFactors[i][j];
	}
	for(auto i = 0U; i < match.inverseMagnitudes.size(); ++i) {
		for(auto j = 0U; j < match.inverseMagnitudes[i].size(); ++j)
			match.inverseMagnitudes[i][j] *= m_inverseManitudeFactors[i][j];
	}
}




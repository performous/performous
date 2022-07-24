#pragma once

#include <set>
#include <string>
#include <vector>

class Chord {
public:
	Chord(std::string const& name, std::set<float> const& frequencies);

	std::string const& getName() const;

	Chord& addAlternate(std::set<float> const& frequencies);

	operator std::set<float> const&() const;

	std::set<float> const& getFrequencies() const;

	size_t countAlternatives() const;
	std::set<float> const& getAlternative(size_t n = 0) const;

private:
	std::string m_name;
	std::vector<std::set<float>> m_frequencies;
};


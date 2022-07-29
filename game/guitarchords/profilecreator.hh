#pragma once

#include "chord.hh"
#include "profile.hh"

#include <map>
#include <vector>

class ProfileCreator {
public:
	ProfileCreator& add(Chord const&, std::vector<float> const& sample);

	Profile create(std::string const& name) const;

private:
	std::map<Chord, std::vector<std::vector<ChordMatch>>> createMatches() const;
	void analyze(std::map<Chord, std::vector<std::vector<ChordMatch>>>&) const;

private:
	std::map<Chord, std::vector<std::vector<float>>> m_samples;
};

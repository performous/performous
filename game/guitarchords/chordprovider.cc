#include "chordprovider.hh"

#include <map>

ChordProvider::ChordProvider(Analyzer& analyzer)
: m_analyser(analyzer) {
}

Strings ChordProvider::getChord() const {
	auto result = std::set<float>{};
	auto sortedByDB = std::map<float, float>{};

	m_analyser.process();

	auto const tones = m_analyser.getTones();

	for(auto const& tone : tones)
		sortedByDB[static_cast<float>(tone.db)] = static_cast<float>(tone.freq);

	for(auto it = sortedByDB.rbegin(); it != sortedByDB.rend() && result.size() < 6; ++it)
		result.insert(it->second);

	return result;
}



#pragma once

#include "ichordprovider.hh"

#include "pitch.hh"

class ChordProvider : public IChordProvider {
  public:
	ChordProvider(Analyzer&);
	~ChordProvider() override = default;

	Strings getChord() const override;

private:
	Analyzer& m_analyser;
};

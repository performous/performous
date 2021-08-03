#pragma once

#include "inotegraphscaler.hh"

class FixNoteGraphScaler : public INoteGraphScaler {
public:
	~FixNoteGraphScaler() = default;

	void initialize(VocalTrack const&) override;
	NoteGraphDimension calculate(VocalTrack const&, Notes::const_iterator const&, double time) const override;

private:
	NoteGraphDimension m_dimension;
};

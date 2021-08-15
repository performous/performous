#pragma once

#include "inotegraphscaler.hh"

class DynamicNoteGraphScaler : public INoteGraphScaler {
public:
	~DynamicNoteGraphScaler() = default;

	void initialize(VocalTrack const&) override;
	NoteGraphDimension calculate(VocalTrack const&, Notes::const_iterator const&, double time) const override;
};

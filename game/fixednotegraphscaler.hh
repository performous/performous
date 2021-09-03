#pragma once

#include "inotegraphscaler.hh"

class FixedNoteGraphScaler : public INoteGraphScaler {
public:
    ~FixedNoteGraphScaler() = default;

    void initialize(VocalTrack const&) override;
    NoteGraphDimension calculate(VocalTrack const&, Notes::const_iterator const&, double time) const override;

private:
    NoteGraphDimension m_dimension;
};

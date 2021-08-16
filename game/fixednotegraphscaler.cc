#include "fixednotegraphscaler.hh"

void FixedNoteGraphScaler::initialize(VocalTrack const& vocal) {
    auto& result = m_dimension;

    result.min1 = vocal.noteMax;
    result.max1 = vocal.noteMin;
    result.min2 = vocal.noteMax;
    result.max2 = vocal.noteMin;

    for (auto it = vocal.notes.begin(); it != vocal.notes.end(); ++it) {
        if (it->type == Note::SLEEP) continue;
        if (it->note < result.min1) result.min1 = it->note;
        if (it->note > result.max1) result.max1 = it->note;
        if (it->note < result.min2) result.min2 = it->note;
        if (it->note > result.max2) result.max2 = it->note;
    }
}

NoteGraphDimension FixedNoteGraphScaler::calculate(VocalTrack const&, const Notes::const_iterator&, double) const {    
    return m_dimension;
}


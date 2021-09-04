#include "dynamicnotegraphscaler.hh"

void DynamicNoteGraphScaler::initialize(VocalTrack const&) {
}

NoteGraphDimension DynamicNoteGraphScaler::calculate(VocalTrack const& vocal, const Notes::const_iterator& current, double time) const {
	auto result = NoteGraphDimension();

	result.min1 = vocal.noteMax;
	result.max1 = vocal.noteMin;
	result.min2 = vocal.noteMax;
	result.max2 = vocal.noteMin;

	for (auto it = current; it != vocal.notes.end() && it->begin < time + 15.0; ++it) {
		if (it->type == Note::Type::SLEEP) continue;
		if (it->note < result.min1) result.min1 = it->note;
		if (it->note > result.max1) result.max1 = it->note;
		if (it->begin > time + 8.0) continue;
		if (it->note < result.min2) result.min2 = it->note;
		if (it->note > result.max2) result.max2 = it->note;
	}

	return result;
}

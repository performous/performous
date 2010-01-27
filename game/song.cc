#include "song.hh"

#include "songparser.hh"
#include "util.hh"
#include <limits>
#include <algorithm>

void Song::reload(bool errorIgnore) {
	loadStatus = NONE;
	notes.clear();
	track_map.clear();
	beats.clear();
	midifilename.clear();
	category.clear();
	genre.clear();
	edition.clear();
	title.clear();
	artist.clear();
	collateByTitle.clear();
	collateByTitleOnly.clear();
	collateByArtist.clear();
	collateByArtistOnly.clear();
	text.clear();
	creator.clear();
	music.clear();
	cover.clear();
	background.clear();
	video.clear();
	noteMin = std::numeric_limits<int>::max();
	noteMax = std::numeric_limits<int>::min();
	videoGap = 0.0;
	start = 0.0;
	preview_start = getNaN();
	beginTime = endTime = getNaN();
	m_scoreFactor = 0.0;
	b0rkedTracks = false;
	try { SongParser(*this); } catch (...) { if (!errorIgnore) throw; }
	collateUpdate();
}

void Song::dropNotes() {
	// Singing
	if (!notes.empty()) {
		notes.clear();
		notes.push_back(Note()); // Dummy note to indicate there is a track
	}
	// Instruments
	if (!track_map.empty()) {
		for (TrackMap::iterator it = track_map.begin(); it != track_map.end(); ++it)
			it->second.nm.clear();
	}
	// Dancing
	if (!danceTracks.empty()) {
		for (DanceTracks::iterator it = danceTracks.begin(); it != danceTracks.end(); ++it)
			it->second.clear();
	}
	b0rkedTracks = false;
	loadStatus = HEADER;
}

void Song::collateUpdate() {
	collateByTitle = collate(title + artist) + '\0' + filename;
	collateByTitleOnly = collate(title);
	collateByArtist = collate(artist + title) + '\0' + filename;
	collateByArtistOnly = collate(artist);
}

std::string Song::collate(std::string const& str) {
	return unicodeCollate(str);
}

namespace {
	// Cannot simply take double as its second argument because of a C++ defect
	bool noteEndLessThan(Note const& a, Note const& b) { return a.end < b.end; }
}

Song::Status Song::status(double time) const {
	Note target; target.end = time;
	Notes::const_iterator it = std::lower_bound(notes.begin(), notes.end(), target, noteEndLessThan);
	if (it == notes.end()) return FINISHED;
	if (it->begin > time + 4.0) return INSTRUMENTAL_BREAK;
	return NORMAL;
}


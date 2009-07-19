#include "song.hh"

#include "songparser.hh"
#include <iostream>

void Song::reload(bool errorIgnore) {
	notes.clear();
	tracks.clear();
	beats.clear();
	category.clear();
	genre.clear();
	edition.clear();
	title.clear();
	artist.clear();
	collateByTitle.clear();
	collateByArtist.clear();
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
	m_scoreFactor = 0.0;
	try { SongParser(*this); } catch (...) { if (!errorIgnore) throw; }
	collateUpdate();
}

void Song::collateUpdate() {
	collateByTitle = collate(title + artist) + '\0' + filename;
	collateByArtist = collate(artist + title) + '\0' + filename;
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


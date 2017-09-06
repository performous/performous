#include "song.hh"
#include "config.hh"
#include "songparser.hh"
#include "util.hh"
#include <limits>
#include <algorithm>
extern "C" {
#include AVFORMAT_INCLUDE
#include AVCODEC_INCLUDE
}

void Song::reload(bool errorIgnore) {
	loadStatus = NONE;
	vocalTracks.clear();
	instrumentTracks.clear();
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
	videoGap = 0.0;
	start = 0.0;
	preview_start = getNaN();
	hasBRE = false;
	b0rked.clear();
	try { SongParser(*this); } catch (...) { if (!errorIgnore) throw; }
	collateUpdate();
}

void Song::loadNotes(bool errorIgnore) {
	if (loadStatus == Song::FULL) return;
	try { SongParser(*this); } catch (...) { if (!errorIgnore) throw; }
}

void Song::dropNotes() {
	for (auto& trk: vocalTracks) trk.second.notes.clear();
	for (auto& trk: instrumentTracks) trk.second.nm.clear();
	for (auto& trk: danceTracks) trk.second.clear();
	b0rked.clear();
	loadStatus = HEADER;
}

void Song::collateUpdate() {
	collateByTitle = collate(title + artist) + '\0' + filename.string();
	collateByTitleOnly = collate(title);
	collateByArtist = collate(artist + title) + '\0' + filename.string();
	collateByArtistOnly = collate(artist);
}

std::string Song::collate(std::string const& str) {
	return unicodeCollate(str);
}

namespace {
	// Cannot simply take double as its second argument because of a C++ defect
	bool noteEndLessThan(Note const& a, Note const& b) { return a.end < b.end; }
}

Song::Status Song::status(double time) {
	Note target; target.end = time;
	Notes::const_iterator it = std::lower_bound(getVocalTrack().notes.begin(), getVocalTrack().notes.end(), target, noteEndLessThan);
	if (it == getVocalTrack().notes.end()) return FINISHED;
	if (it->begin > time + 4.0) return INSTRUMENTAL_BREAK;
	return NORMAL;
}

bool Song::getNextSection(double pos, SongSection &section) {
	for (auto& sect: songsections) {
		if (sect.begin > pos) {
			section = sect;
			return true;
		}
	}
	// returning false here will jump forward 5s (see screen_sing.cc)
	return false;
}

bool Song::getPrevSection(double pos, SongSection &section) {
	for (auto it = songsections.rbegin(); it != songsections.rend(); ++it) {
		// subtract 1 second so we can jump across a section
		if (it->begin < pos - 1.0) {
			section = *it;
			return true;
		}
	}
	// returning false here will jump backwards by 5s (see screen_sing.cc)
	return false;
}

std::ostream& operator<<(std::ostream& os, SongParserException const& e) {
	os << (e.silent() ? "songparser/debug: " : "songparser/warning: ") << e.file().string();
	if (e.line()) os << ":" << e.line();
	os << ":\n  " << e.what() << std::endl;
	return os;
}

void Song::insertVocalTrack(std::string vocalTrack, VocalTrack track) {
	eraseVocalTrack(vocalTrack);
	vocalTracks.insert(std::make_pair(vocalTrack, track));
}

void Song::eraseVocalTrack(std::string vocalTrack) {
	vocalTracks.erase(vocalTrack);
}

VocalTrack& Song::getVocalTrack(std::string vocalTrack) {
	VocalTracks::iterator it = vocalTracks.find(vocalTrack);
	if (it != vocalTracks.end()) {
		return it->second;
	} else {
		it = vocalTracks.find(TrackName::LEAD_VOCAL);
		if (it != vocalTracks.end()) return it->second;
		else if (!vocalTracks.empty()) return vocalTracks.begin()->second;
		else return dummyVocal;
	}
}

VocalTrack& Song::getVocalTrack(unsigned idx) {
	if (idx >= vocalTracks.size()) throw std::logic_error("Index out of bounds in Song::getVocalTrack");
	VocalTracks::iterator it = vocalTracks.begin();
	std::advance(it, idx);
	return it->second;
}

double Song::getDurationSeconds() {
	boost::mutex::scoped_lock l(m_mutex);
	if(m_duration == 0) {
		AVFormatContext *pFormatCtx = avformat_alloc_context();
		if (avformat_open_input(&pFormatCtx, music["background"].string().c_str(), NULL, NULL) == 0) {
			avformat_find_stream_info(pFormatCtx, NULL);
			m_duration = pFormatCtx->duration / AV_TIME_BASE;
			avformat_close_input(&pFormatCtx);
			avformat_free_context(pFormatCtx);
			return m_duration;
		}
		std::clog << "song/info: >>> Couldn't open file for calculating duration." << std::endl;
		return 0;
	} else { //duration is still in memmory that means we already loaded it
		return m_duration;
	}
return 0;
}




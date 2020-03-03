#include "song.hh"
#include "config.hh"
#include "screen_sing.hh"
#include "songparser.hh"
#include "unicode.hh"
#include "util.hh"

#include <algorithm>
#include <limits>

extern "C" {
#include AVFORMAT_INCLUDE
#include AVCODEC_INCLUDE
}
#ifdef USE_WEBSERVER
Song::Song(web::json::value const& song): dummyVocal(TrackName::LEAD_VOCAL), randomIdx(rand()) {
	std::string emptyStr = "";
	path = song.has_field(utility::conversions::to_string_t("TxtFileFolder")) ? fs::path(song.at(utility::conversions::to_string_t("TxtFileFolder")).as_string().substr(0, utility::conversions::to_utf8string(song.at(utility::conversions::to_string_t("TxtFileFolder")).as_string()).find_last_of("/\\"))) : emptyStr;
	filename = song.has_field(utility::conversions::to_string_t("TxtFile")) ? fs::path(song.at(utility::conversions::to_string_t("TxtFile")).as_string()) : emptyStr;
	artist = song.has_field(utility::conversions::to_string_t("Artist")) ? utility::conversions::to_utf8string(song.at(utility::conversions::to_string_t("Artist")).as_string()) : emptyStr;
	title = song.has_field(utility::conversions::to_string_t("Title")) ? utility::conversions::to_utf8string(song.at(utility::conversions::to_string_t("Title")).as_string()) : emptyStr;
	language = song.has_field(utility::conversions::to_string_t("Language")) ? utility::conversions::to_utf8string(song.at(utility::conversions::to_string_t("Language")).as_string()) : emptyStr;
	edition = song.has_field(utility::conversions::to_string_t("Edition")) ? utility::conversions::to_utf8string(song.at(utility::conversions::to_string_t("Edition")).as_string()) : emptyStr;
	creator = song.has_field(utility::conversions::to_string_t("Creator")) ? utility::conversions::to_utf8string(song.at(utility::conversions::to_string_t("Creator")).as_string()) : emptyStr;
	genre = song.has_field(utility::conversions::to_string_t("Genre")) ? utility::conversions::to_utf8string(song.at(utility::conversions::to_string_t("Genre")).as_string()) : emptyStr;
	cover = song.has_field(utility::conversions::to_string_t("Cover")) ? utility::conversions::to_utf8string(song.at(utility::conversions::to_string_t("Cover")).as_string()) : emptyStr;
	background = song.has_field(utility::conversions::to_string_t("Background")) ? utility::conversions::to_utf8string(song.at(utility::conversions::to_string_t("Background")).as_string()) : emptyStr;
	video = song.has_field(utility::conversions::to_string_t("VideoFile")) ? fs::path(song.at(utility::conversions::to_string_t("VideoFile")).as_string()) : emptyStr;
	videoGap = song.has_field(utility::conversions::to_string_t("VideoGap")) ? song.at(utility::conversions::to_string_t("VideoGap")).as_number().to_double() : 0.0;
	start = song.has_field(utility::conversions::to_string_t("Start")) ? song.at(utility::conversions::to_string_t("Start")).as_number().to_double() : 0.0;
	preview_start = song.has_field(utility::conversions::to_string_t("PreviewStart")) ? song.at(utility::conversions::to_string_t("PreviewStart")).as_number().to_double() : 0.0;
	m_duration = song.has_field(utility::conversions::to_string_t("Duration")) ? song.at(utility::conversions::to_string_t("Duration")).as_number().to_double() : 0.0;
	music["background"] = song.has_field(utility::conversions::to_string_t("SongFile")) ? fs::path(song.at(utility::conversions::to_string_t("SongFile")).as_string()) : emptyStr;
	music["vocals"] = song.has_field(utility::conversions::to_string_t("Vocals")) ? fs::path(song.at(utility::conversions::to_string_t("Vocals")).as_string()) : emptyStr;
	loadStatus = Song::LoadStatus::HEADER;
	
	if (song.has_field(utility::conversions::to_string_t("VocalTracks"))) {
		for (unsigned i = 0; i < song.at(utility::conversions::to_string_t("VocalTracks")).as_number().to_uint32(); i++) {
			std::string track = "DummyTrack" + std::to_string(i);
			insertVocalTrack(track, VocalTrack(track));
		}
	}
	
	if (song.has_field(utility::conversions::to_string_t("KeyboardTracks"))) {
			instrumentTracks.insert(make_pair(TrackName::KEYBOARD, InstrumentTrack(TrackName::KEYBOARD)));
	}
	
	if (song.has_field(utility::conversions::to_string_t("DrumTracks"))) {
			instrumentTracks.insert(make_pair(TrackName::DRUMS, InstrumentTrack(TrackName::DRUMS)));
	}		
	if (song.has_field(utility::conversions::to_string_t("DanceTracks"))) {
		DanceDifficultyMap danceDifficultyMap;
			danceTracks.insert(std::make_pair("dance-single", danceDifficultyMap));
	}		
	if (song.has_field(utility::conversions::to_string_t("GuitarTracks"))) {
			instrumentTracks.insert(std::make_pair(TrackName::GUITAR, InstrumentTrack(TrackName::GUITAR)));
	}
	if (song.has_field("BPM")) {
			m_bpms.push_back(BPM(0, 0, song.at("BPM").as_number().to_double()));
	}
	collateUpdate();
}
#endif

Song::Song(fs::path const& path, fs::path const& filename):
  dummyVocal(TrackName::LEAD_VOCAL), path(path), filename(filename), randomIdx(rand())
{
	SongParser(*this);
	collateUpdate();
}

void Song::reload(bool errorIgnore) {
	try { *this = Song(path, filename); } catch (...) { if (!errorIgnore) throw; }
}

void Song::loadNotes(bool errorIgnore) {
	if (loadStatus == LoadStatus::FULL) return;
	try { SongParser(*this); } catch (...) { if (!errorIgnore) throw; }
}

void Song::dropNotes() {
	for (auto& trk: vocalTracks) trk.second.notes.clear();
	for (auto& trk: instrumentTracks) trk.second.nm.clear();
	for (auto& trk: danceTracks) trk.second.clear();
	b0rked.clear();
	loadStatus = LoadStatus::HEADER;
}

void Song::collateUpdate() {
	songMetadata collateInfo {{"artist", artist}, {"title", title}};
	UnicodeUtil::collate(collateInfo);	
	
	collateByTitle = collateInfo["title"] + "__" + collateInfo["artist"] + "__" + filename.string();
	collateByTitleOnly = collateInfo["title"];
	
	collateByArtist = collateInfo["artist"] + "__" + collateInfo["title"] + "__" + filename.string();
	collateByArtistOnly = collateInfo["artist"];
}

Song::Status Song::status(double time, ScreenSing* song) {
	if (song->getMenu().isOpen()) return Status::NORMAL; // This should prevent querying getVocalTrack with an out-of-bounds/uninitialized index.
	if (vocalTracks.empty()) return Status::NORMAL;  // To avoid crash with non-vocal songs (dance, guitar) -- FIXME: what should we actually do?
	Note target; target.end = time;
	Notes s1, s2, notes;
	Notes::const_iterator it;
	if (song->singingDuet()) {
		s1 = getVocalTrack(0).notes;
		s2 = getVocalTrack(1).notes;
		std::merge(s1.begin(), s1.end(), s2.begin(), s2.end(), std::back_inserter(notes), Note::ltBegin);
	}
	else {
		notes = getVocalTrack(song->selectedVocalTrack()).notes;
	}
	it = std::lower_bound(notes.begin(), notes.end(), target, [](Note const& a, Note const& b) { return a.end < b.end; });
	if (it == notes.end()) return Status::FINISHED;
	if (it->begin > time + 4.0) return Status::INSTRUMENTAL_BREAK;
	return Status::NORMAL;
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

VocalTrack& Song::getVocalTrack(size_t idx) {
	if (idx >= vocalTracks.size()) {
		throw std::logic_error("Index " + std::to_string(idx) + " out of bounds in Song::getVocalTrack (size: " + std::to_string(vocalTracks.size()) +").");
		}
	VocalTracks::iterator it = vocalTracks.begin();
	std::advance(it, idx);
	return it->second;
}

double Song::getDurationSeconds() {
	if(m_duration == 0.0 || m_duration < 1.0) {
		AVFormatContext *pFormatCtx = avformat_alloc_context();
		if (avformat_open_input(&pFormatCtx, music["background"].string().c_str(), nullptr, nullptr) == 0) {
			avformat_find_stream_info(pFormatCtx, nullptr);
			m_duration = pFormatCtx->duration / AV_TIME_BASE;
			avformat_close_input(&pFormatCtx);
			avformat_free_context(pFormatCtx);
			return m_duration;
		}
		std::clog << "song/info: >>> Couldn't open file for calculating duration." << std::endl;
		return 0.0;
	} else { //duration is still in memmory that means we already loaded it
		return m_duration;
	}
}

std::string Song::str() const { return title + "  by  " + artist; }

std::string Song::strFull() const {
	return title + "\n" + artist + "\n" + genre + "\n" + edition + "\n" + path.string();
}

std::vector<std::string> Song::getVocalTrackNames() const {
	std::vector<std::string> result;
	for (auto const& kv: vocalTracks) result.push_back(kv.first);
	return result;
}


#include "song.hh"
#include "config.hh"
#include "screen_sing.hh"
#include "songparser.hh"
#include "unicode.hh"
#include "util.hh"

#include <algorithm>
#include <limits>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

Song::Song(nlohmann::json const& song): dummyVocal(TrackName::LEAD_VOCAL), randomIdx(rand()) {
	std::string temp;
	if (song.contains("TxtFileFolder")) {
		temp = UnicodeUtil::convertToUTF8(song["TxtFileFolder"].get<std::string>());
		path = temp.substr(0,temp.find_last_of("/\\"));
	}

	if (song.contains("TxtFile")) { 
		filename = UnicodeUtil::convertToUTF8(song["TxtFile"].get<std::string>());
	}

	if (song.contains("Artist")) {
		artist = UnicodeUtil::convertToUTF8(song["Artist"].get<std::string>());
	}
	if (song.contains("Title")) {
		title = UnicodeUtil::convertToUTF8(song["Title"].get<std::string>());
	}
	if (song.contains("Language")) {
		language = UnicodeUtil::convertToUTF8(song["Language"].get<std::string>());
	}
	if (song.contains("Edition")) {
		edition = UnicodeUtil::convertToUTF8(song["Edition"].get<std::string>());
	}
	if (song.contains("Creator")) {
		creator = UnicodeUtil::convertToUTF8(song["Creator"].get<std::string>());
	}
	if (song.contains("Genre")) {
		genre = UnicodeUtil::convertToUTF8(song["Genre"].get<std::string>());
	}
	if (song.contains("Cover")) {
		cover = UnicodeUtil::convertToUTF8(song["Cover"].get<std::string>());
	}
	if (song.contains("Background")) {
		background = UnicodeUtil::convertToUTF8(song["Background"].get<std::string>());
	}

	if (song.contains("VideoFile")) {
		video = UnicodeUtil::convertToUTF8(song["VideoFile"].get<std::string>());
	}
	videoGap = song.contains("VideoGap") ? song["VideoGap"].get<double>() : 0.0;
	start = song.contains("Start") ? song["Start"].get<double>() : 0.0;
	preview_start = song.contains("PreviewStart") ? song["PreviewStart"].get<double>() : 0.0;
	m_duration = song.contains("Duration") ? song["Duration"].get<double>() : 0.0;
	if (song.contains("SongFile")) {
		music["TrackName::BGMUSIC"] = UnicodeUtil::convertToUTF8(song["SongFile"].get<std::string>());
	}
	if (song.contains("Vocals")) {
		music["TrackName::LEAD_VOCAL"] = UnicodeUtil::convertToUTF8(song["Vocals"].get<std::string>());
	}
	if (song.contains("Preview")) {
		music["TrackName::PREVIEW"] = UnicodeUtil::convertToUTF8(song["Preview"].get<std::string>());
	}
	if (song.contains("Guitar")) {
		music["TrackName::GUITAR"] = UnicodeUtil::convertToUTF8(song["Guitar"].get<std::string>());
	}
	if (song.contains("Bass")) {
		music["TrackName::BASS"] = UnicodeUtil::convertToUTF8(song["Bass"].get<std::string>());
	}
	if (song.contains("Drums")) {
		music["TrackName::DRUMS"] = UnicodeUtil::convertToUTF8(song["Drums"].get<std::string>());
	}
	if (song.contains("Keyboard")) {
		music["TrackName::KEYBOARD"] = UnicodeUtil::convertToUTF8(song["Keyboard"].get<std::string>());
	}
	if (song.contains("Guitar_coop")) {
		music["TrackName::GUITAR_COOP"] = UnicodeUtil::convertToUTF8(song["Guitar_coop"].get<std::string>());
	}	
	loadStatus = Song::LoadStatus::HEADER;

	if (song.contains("VocalTracks")) {
		for (unsigned i = 0; i < song["VocalTracks"].get<unsigned>(); i++) {
			std::string track("DummyTrack" + std::to_string(i));
			insertVocalTrack(track, VocalTrack(track));
		}
	}

	if (song.contains("KeyboardTracks")) {
		instrumentTracks.insert(make_pair(TrackName::KEYBOARD, InstrumentTrack(TrackName::KEYBOARD)));
	}
	if (song.contains("DrumTracks")) {
		instrumentTracks.insert(make_pair(TrackName::DRUMS, InstrumentTrack(TrackName::DRUMS)));
	}
	if (song.contains("DanceTracks")) {
		DanceDifficultyMap danceDifficultyMap;
		danceTracks.insert(std::make_pair("dance-single", danceDifficultyMap));
	}
	
	if (song.contains("GuitarTracks")) {
		instrumentTracks.insert(std::make_pair(TrackName::GUITAR, InstrumentTrack(TrackName::GUITAR)));
	}
	if (song.contains("BPM")) {
			m_bpms.push_back(BPM(0, 0, song["BPM"].get<double>()));
	}
	collateUpdate();
}

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
	} else { //duration is still in memory that means we already loaded it
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


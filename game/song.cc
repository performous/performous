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
	path = song.contains("TxtFileFolder") ? fs::path(song.at("TxtFileFolder").get<std::string>()) : "";
	filename = song.contains("TxtFile") ? fs::path(song.at("TxtFile").get<std::string>()) : "";
	artist = song.contains("Artist") ? song.at("Artist").get<std::string>() : "";
	title = song.contains("Title") ? song.at("Title").get<std::string>() : "";
	language = song.contains("Language") ? song.at("Language").get<std::string>() : "";
	edition = song.contains("Edition") ? song.at("Edition").get<std::string>() : "";
	creator = song.contains("Creator") ? song.at("Creator").get<std::string>() : "";
	genre = song.contains("Genre") ? song.at("Genre").get<std::string>() : "";
	cover = song.contains("Cover") ? song.at("Cover").get<std::string>() : "";
	background = song.contains("Background") ? song.at("Background").get<std::string>() : "";
	video = song.contains("VideoFile") ? fs::path(song.at("VideoFile").get<std::string>()) : "";
	midifilename = song.contains("MidFile") ? fs::path(song.at("MidFile").get<std::string>()) : "";
	videoGap = song.contains("VideoGap") ? song.at("VideoGap").get<double>() : 0.0;
	start = song.contains("Start") ? song.at("Start").get<double>() : 0.0;
	preview_start = song.contains("PreviewStart") ? song.at("PreviewStart").get<double>() : 0.0;
	m_duration = song.contains("Duration") ? song.at("Duration").get<double>() : 0.0;
	music[TrackName::BGMUSIC] = song.contains("SongFile") ? fs::path(song.at("SongFile").get<std::string>()) : "";
	music[TrackName::LEAD_VOCAL] = song.contains("Vocals") ? fs::path(song.at("Vocals").get<std::string>()) : "";
	music[TrackName::PREVIEW] = song.contains("Preview") ? fs::path(song.at("Preview").get<std::string>()) : "";
	music[TrackName::GUITAR] = song.contains("Guitar") ? fs::path(song.at("Guitar").get<std::string>()) : "";
	music[TrackName::BASS] = song.contains("Bass") ? fs::path(song.at("Bass").get<std::string>()) : "";
	music[TrackName::DRUMS] = song.contains("Drums") ? fs::path(song.at("Drums").get<std::string>()) : "";
	music[TrackName::KEYBOARD] = song.contains("Keyboard") ? fs::path(song.at("Keyboard").get<std::string>()) : "";
	music[TrackName::GUITAR_COOP] = song.contains("Guitar_coop") ? fs::path(song.at("Guitar_coop").get<std::string>()) : "";
	music[TrackName::GUITAR_RHYTHM] = song.contains("Guitar_rhythm") ? fs::path(song.at("Guitar_rhythm").get<std::string>()) : "";
	loadStatus = Song::LoadStatus::HEADER;

	if (song.contains("VocalTracks")) {
		for (size_t i = 0; i < song.at("VocalTracks").get<size_t>(); i++) {
			std::string track = "DummyTrack" + std::to_string(i);
			insertVocalTrack(track, VocalTrack(track));
		}
	}

	if (song.contains("KeyboardTracks")) {
		instrumentTracks.insert(make_pair(TrackName::KEYBOARD, InstrumentTrack(TrackName::KEYBOARD)));
	}

	if (song.contains("DrumTracks") && song.at("DrumTracks").get<size_t>() > 0) {
		instrumentTracks.insert(make_pair(TrackName::DRUMS, InstrumentTrack(TrackName::DRUMS)));
	}
	if (song.contains("DanceTracks") && song.at("DanceTracks").get<size_t>() > 0) {
		DanceDifficultyMap danceDifficultyMap;
		danceTracks.insert(std::make_pair("dance-single", danceDifficultyMap));
	}
	if (song.contains("GuitarTracks")) {
		instrumentTracks.insert(std::make_pair(TrackName::GUITAR, InstrumentTrack(TrackName::GUITAR)));
	}
	if (song.contains("BPM")) {
		m_bpms.push_back(BPM(0, 0, song.at("BPM").get<double>()));
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
	Notes* notes = nullptr;
	Notes::const_iterator it;

	if (song->singingDuet()) {
		notes = &getVocalTrack(SongParserUtil::DUET_BOTH).notes;
	}
	else {
		notes = &getVocalTrack(song->selectedVocalTrack()).notes;
	}
	it = std::lower_bound(notes->begin(), notes->end(), target, [](Note const& a, Note const& b) { return a.end < b.end; });
	if (it == notes->end()) return Status::FINISHED;
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

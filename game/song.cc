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
	path = song.contains("txtFileFolder") ? fs::path(song.at("txtFileFolder").get<std::string>()) : "";
	filename = song.contains("txtFile") ? fs::path(song.at("txtFile").get<std::string>()) : "";
	artist = song.contains("artist") ? song.at("artist").get<std::string>() : "";
	title = song.contains("title") ? song.at("title").get<std::string>() : "";
	language = song.contains("language") ? song.at("language").get<std::string>() : "";
	edition = song.contains("edition") ? song.at("edition").get<std::string>() : "";
	creator = song.contains("creator") ? song.at("creator").get<std::string>() : "";
	genre = song.contains("genre") ? song.at("genre").get<std::string>() : "";
	cover = song.contains("cover") ? song.at("cover").get<std::string>() : "";
	background = song.contains("background") ? song.at("background").get<std::string>() : "";
	video = song.contains("videoFile") ? fs::path(song.at("videoFile").get<std::string>()) : "";
	midifilename = song.contains("midiFile") ? fs::path(song.at("midiFile").get<std::string>()) : "";
	videoGap = song.contains("videoGap") ? song.at("videoGap").get<double>() : 0.0;
	start = song.contains("start") ? song.at("start").get<double>() : 0.0;
	preview_start = song.contains("previewStart") ? song.at("previewStart").get<double>() : 0.0;
	m_duration = song.contains("duration") ? song.at("duration").get<double>() : 0.0;
	music[TrackName::BGMUSIC] = song.contains("songFile") ? fs::path(song.at("songFile").get<std::string>()) : "";
	music[TrackName::LEAD_VOCAL] = song.contains("vocals") ? fs::path(song.at("vocals").get<std::string>()) : "";
	music[TrackName::PREVIEW] = song.contains("preview") ? fs::path(song.at("preview").get<std::string>()) : "";
	music[TrackName::GUITAR] = song.contains("guitar") ? fs::path(song.at("guitar").get<std::string>()) : "";
	music[TrackName::BASS] = song.contains("bass") ? fs::path(song.at("bass").get<std::string>()) : "";
	music[TrackName::DRUMS] = song.contains("drums") ? fs::path(song.at("drums").get<std::string>()) : "";
	music[TrackName::KEYBOARD] = song.contains("keyboard") ? fs::path(song.at("keyboard").get<std::string>()) : "";
	music[TrackName::GUITAR_COOP] = song.contains("guitarCoop") ? fs::path(song.at("guitarCoop").get<std::string>()) : "";
	music[TrackName::GUITAR_RHYTHM] = song.contains("guitarRhythm") ? fs::path(song.at("guitarRhythm").get<std::string>()) : "";
	loadStatus = Song::LoadStatus::HEADER;

	if (song.contains("vocalTracks")) {
		for (size_t i = 0; i < song.at("vocalTracks").get<size_t>(); i++) {
			std::string track = "DummyTrack" + std::to_string(i);
			insertVocalTrack(track, VocalTrack(track));
		}
	}

	if (song.contains("keyboardTracks") && song.at("keyboardTracks").get<bool>()) {
		instrumentTracks.insert(make_pair(TrackName::KEYBOARD, InstrumentTrack(TrackName::KEYBOARD)));
	}

	if (song.contains("drumTracks") && song.at("drumTracks").get<bool>()) {
		instrumentTracks.insert(make_pair(TrackName::DRUMS, InstrumentTrack(TrackName::DRUMS)));
	}
	if (song.contains("danceTracks") && song.at("danceTracks").get<bool>()) {
		DanceDifficultyMap danceDifficultyMap;
		danceTracks.insert(std::make_pair("dance-single", danceDifficultyMap));
	}
	if (song.contains("guitarTracks") && song.at("guitarTracks").get<bool>()) {
		instrumentTracks.insert(std::make_pair(TrackName::GUITAR, InstrumentTrack(TrackName::GUITAR)));
	}
	if (song.contains("bpm")) {
		m_bpms.push_back(BPM(0, 0, song.at("bpm").get<double>()));
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

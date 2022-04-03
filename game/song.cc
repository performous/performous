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
#ifdef USE_WEBSERVER
Song::Song(web::json::value const& song): dummyVocal(TrackName::LEAD_VOCAL), randomIdx(rand()) {
	path = song.has_field("TxtFileFolder") ? fs::path(song.at("TxtFileFolder").as_string().substr(0, song.at("TxtFileFolder").as_string().find_last_of("/\\"))) : "";
	filename = song.has_field("TxtFile") ? fs::path(song.at("TxtFile").as_string()) : "";
	artist = song.has_field("Artist") ? song.at("Artist").as_string() : "";
	title = song.has_field("Title") ? song.at("Title").as_string() : "";
	language = song.has_field("Language") ? song.at("Language").as_string() : "";
	edition = song.has_field("Edition") ? song.at("Edition").as_string() : "";
	creator = song.has_field("Creator") ? song.at("Creator").as_string() : "";
	genre = song.has_field("Genre") ? song.at("Genre").as_string() : "";
	cover = song.has_field("Cover") ? song.at("Cover").as_string() : "";
	background = song.has_field("Background") ? song.at("Background").as_string() : "";
	video = song.has_field("VideoFile") ? fs::path(song.at("VideoFile").as_string()) : "";
	midifilename = song.has_field("MidFile") ? fs::path(song.at("MidFile").as_string()) : "";
	videoGap = song.has_field("VideoGap") ? song.at("VideoGap").as_number().to_double() : 0.0;
	start = song.has_field("Start") ? song.at("Start").as_number().to_double() : 0.0;
	preview_start = song.has_field("PreviewStart") ? song.at("PreviewStart").as_number().to_double() : 0.0;
	m_duration = song.has_field("Duration") ? song.at("Duration").as_number().to_double() : 0.0;
	music[TrackName::BGMUSIC] = song.has_field("SongFile") ? fs::path(song.at("SongFile").as_string()) : "";
	music[TrackName::LEAD_VOCAL] = song.has_field("Vocals") ? fs::path(song.at("Vocals").as_string()) : "";
	music[TrackName::PREVIEW] = song.has_field("Preview") ? fs::path(song.at("Preview").as_string()) : "";
	music[TrackName::GUITAR] = song.has_field("Guitar") ? fs::path(song.at("Guitar").as_string()) : "";
	music[TrackName::BASS] = song.has_field("Bass") ? fs::path(song.at("Bass").as_string()) : "";
	music[TrackName::DRUMS] = song.has_field("Drums") ? fs::path(song.at("Drums").as_string()) : "";
	music[TrackName::KEYBOARD] = song.has_field("Keyboard") ? fs::path(song.at("Keyboard").as_string()) : "";
	music[TrackName::GUITAR_COOP] = song.has_field("Guitar_coop") ? fs::path(song.at("Guitar_coop").as_string()) : "";
	music[TrackName::GUITAR_RHYTHM] = song.has_field("Guitar_rhythm") ? fs::path(song.at("Guitar_rhythm").as_string()) : "";
	loadStatus = Song::LoadStatus::HEADER;
	
	if (song.has_field("VocalTracks")) {
		for (unsigned i = 0; i < song.at("VocalTracks").as_number().to_uint32(); i++) {
			std::string track = "DummyTrack" + std::to_string(i);
			insertVocalTrack(track, VocalTrack(track));
		}
	}
	
	if (song.has_field("KeyboardTracks")) {
			instrumentTracks.insert(make_pair(TrackName::KEYBOARD, InstrumentTrack(TrackName::KEYBOARD)));
	}
	
	if (song.has_field("DrumTracks") && song..at("DrumTracks").as_number() > 0) {
			instrumentTracks.insert(make_pair(TrackName::DRUMS, InstrumentTrack(TrackName::DRUMS)));
	}		
	if (song.has_field("DanceTracks") && song..at("DanceTracks").as_number() > 0) {
		DanceDifficultyMap danceDifficultyMap;
			danceTracks.insert(std::make_pair("dance-single", danceDifficultyMap));
	}		
	if (song.has_field("GuitarTracks")) {
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


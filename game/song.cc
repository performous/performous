#include "song.hh"
#include "config.hh"
#include "screen_sing.hh"
#include "songparser.hh"
#include "unicode.hh"
#include "util.hh"

#include <algorithm>
#include <limits>
#include <optional>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

Song::Song(nlohmann::json const& song) : dummyVocal(TrackName::VOCAL_LEAD), randomIdx(rand()) {
    path = getJsonEntry<std::string>(song, "txtFileFolder").value_or("");
    filename = getJsonEntry<std::string>(song, "txtFile").value_or("");
    artist = getJsonEntry<std::string>(song, "artist").value_or("");
    title = getJsonEntry<std::string>(song, "title").value_or("");
    language = getJsonEntry<std::string>(song, "language").value_or("");
    edition = getJsonEntry<std::string>(song, "edition").value_or("");
    creator = getJsonEntry<std::string>(song, "creator").value_or("");
    providedBy = getJsonEntry<std::string>(song, "providedBy").value_or("");
    comment = getJsonEntry<std::string>(song, "comment").value_or("");
    genre = getJsonEntry<std::string>(song, "genre").value_or("");
    cover = getJsonEntry<std::string>(song, "cover").value_or("");
    background = getJsonEntry<std::string>(song, "background").value_or("");
    video = getJsonEntry<std::string>(song, "videoFile").value_or("");
    midifilename = getJsonEntry<std::string>(song, "midiFile").value_or("");
    videoGap = getJsonEntry<double>(song, "videoGap").value_or(0.0);
    start = getJsonEntry<double>(song, "start").value_or(0.0);
    preview_start = getJsonEntry<double>(song, "previewStart").value_or(0.0);
    m_duration = getJsonEntry<double>(song, "duration").value_or(0.0);
    music[TrackName::BGMUSIC] = getJsonEntry<std::string>(song, "songFile").value_or("");
    music[TrackName::VOCAL_LEAD] = getJsonEntry<std::string>(song, "vocals").value_or("");
    music[TrackName::VOCAL_BACKING] = getJsonEntry<std::string>(song, "vocalsBacking").value_or("");
    music[TrackName::PREVIEW] = getJsonEntry<std::string>(song, "preview").value_or("");
    music[TrackName::GUITAR] = getJsonEntry<std::string>(song, "guitar").value_or("");
    music[TrackName::BASS] = getJsonEntry<std::string>(song, "bass").value_or("");
    music[TrackName::DRUMS] = getJsonEntry<std::string>(song, "drums").value_or("");
    music[TrackName::DRUMS_SNARE] = getJsonEntry<std::string>(song, "drumsSnare").value_or("");
    music[TrackName::DRUMS_CYMBALS] = getJsonEntry<std::string>(song, "drumsCymbals").value_or("");
    music[TrackName::DRUMS_TOMS] = getJsonEntry<std::string>(song, "drumsToms").value_or("");
    music[TrackName::KEYBOARD] = getJsonEntry<std::string>(song, "keyboard").value_or("");
    music[TrackName::GUITAR_COOP] = getJsonEntry<std::string>(song, "guitarCoop").value_or("");
    music[TrackName::GUITAR_RHYTHM] = getJsonEntry<std::string>(song, "guitarRhythm").value_or("");
    loadStatus = static_cast<Song::LoadStatus>(getJsonEntry<int>(song, "loadStatus").value_or(1));
    //loadStatus = Song::LoadStatus::HEADER;

	collateByTitle = getJsonEntry<std::string>(song, "collateByTitle").value_or("");
	collateByTitleOnly = getJsonEntry<std::string>(song, "collateByTitleOnly").value_or("");
	collateByArtist = getJsonEntry<std::string>(song, "collateByArtist").value_or("");
	collateByArtistOnly = getJsonEntry<std::string>(song, "collateByArtistOnly").value_or("");

    for (size_t i = 0; i < getJsonEntry<size_t>(song, "vocalTracks").value_or(0); i++) {
        std::string track = "DummyTrack" + std::to_string(i);
        insertVocalTrack(track, VocalTrack(track));
    }

    if (getJsonEntry<bool>(song, "keyboardTracks").value_or(false)) {
        instrumentTracks.insert(make_pair(TrackName::KEYBOARD, InstrumentTrack(TrackName::KEYBOARD)));
    }

    if (getJsonEntry<bool>(song, "drumTracks").value_or(false)) {
        instrumentTracks.insert(make_pair(TrackName::DRUMS, InstrumentTrack(TrackName::DRUMS)));
        instrumentTracks.insert(make_pair(TrackName::DRUMS_SNARE, InstrumentTrack(TrackName::DRUMS_SNARE)));
        instrumentTracks.insert(make_pair(TrackName::DRUMS_CYMBALS, InstrumentTrack(TrackName::DRUMS_CYMBALS)));
        instrumentTracks.insert(make_pair(TrackName::DRUMS_TOMS, InstrumentTrack(TrackName::DRUMS_TOMS)));
    }
    if (getJsonEntry<bool>(song, "danceTracks").value_or(false)) {
        DanceDifficultyMap danceDifficultyMap;
        danceTracks.insert(std::make_pair("dance-single", danceDifficultyMap));
    }
    if (getJsonEntry<bool>(song, "guitarTracks").value_or(false)) {
        instrumentTracks.insert(std::make_pair(TrackName::GUITAR, InstrumentTrack(TrackName::GUITAR)));
    }
    if (song.contains("bpm")) {
        m_bpms.push_back(BPM(0, 0, song.at("bpm").get<float>()));
    }
    collateUpdate();
}

Song::Song(fs::path const& filename):
  dummyVocal(TrackName::VOCAL_LEAD), path(filename.parent_path()), filename(filename), randomIdx(rand())
{
    SongParser(*this);
    collateUpdate();
}

void Song::reload(bool errorIgnore) {
    try { 
        *this = Song(filename); 
    }
    catch (...) { 
        if (!errorIgnore) 
            throw; 
    }
}

void Song::loadNotes(bool errorIgnore) {
    if (loadStatus == LoadStatus::FULL) return;
    try { SongParser(*this); }
    catch (...) { if (!errorIgnore) throw; }
}

void Song::dropNotes() {
    for (auto& trk : vocalTracks) trk.second.notes.clear();
    for (auto& trk : instrumentTracks) trk.second.nm.clear();
    for (auto& trk : danceTracks) trk.second.clear();
    b0rked.clear();
    loadStatus = LoadStatus::HEADER;
}

void Song::collateUpdate() {
    songMetadata collateInfo{ {"artist", artist}, {"title", title} };
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

bool Song::getNextSection(double pos, SongSection& section) {
    for (auto& sect : songsections) {
        if (sect.begin > pos) {
            section = sect;
            return true;
        }
    }
    // returning false here will jump forward 5s (see screen_sing.cc)
    return false;
}

bool Song::getPrevSection(double pos, SongSection& section) {
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

bool Song::isBroken() const {
    return m_broken;
}

void Song::setBroken(bool broken) {
	m_broken = broken;
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
    }
    else {
        it = vocalTracks.find(TrackName::VOCAL_LEAD);
        if (it != vocalTracks.end()) return it->second;
        else if (!vocalTracks.empty()) return vocalTracks.begin()->second;
        else return dummyVocal;
    }
}

VocalTrack& Song::getVocalTrack(unsigned idx) {
    if (idx >= static_cast<unsigned>(vocalTracks.size())) {
        return dummyVocal;
    }
    else {
        VocalTracks::iterator it = vocalTracks.begin();
        std::advance(it, idx);
        return it->second;
    }
}

double Song::getDurationSeconds() {
    if (m_duration == 0.0 || m_duration < 1.0) {
        AVFormatContext* pFormatCtx = avformat_alloc_context();
        if (avformat_open_input(&pFormatCtx, music[TrackName::BGMUSIC].string().c_str(), nullptr, nullptr) == 0) {
            avformat_find_stream_info(pFormatCtx, nullptr);
            m_duration = static_cast<double>(pFormatCtx->duration) / static_cast<double>(AV_TIME_BASE);
            avformat_close_input(&pFormatCtx);
            avformat_free_context(pFormatCtx);
            return m_duration;
        }
        std::clog << "song/info: >>> Couldn't open file for calculating duration." << std::endl;
        return 0.0;
    }
    else { //duration is still in memmory that means we already loaded it
        return m_duration;
    }
}

double Song::getPreviewStart() {
	if (std::isnan(preview_start)) {
		preview_start = ((type == Type::INI || getDurationSeconds() < 50.0) ? 5.0 : 30.0);  // 5 s for band mode, 30 s for others
	}
	return preview_start;
}

std::string Song::str() const { return title + "  by  " + artist; }

std::string Song::strFull() const {
    return title + "\n" + artist + "\n" + genre + "\n" + edition + "\n" + path.string();
}

std::vector<std::string> Song::getVocalTrackNames() const {
    std::vector<std::string> result;
    for (auto const& kv : vocalTracks) result.push_back(kv.first);
    return result;
}

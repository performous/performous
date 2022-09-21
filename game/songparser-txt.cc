#include "songparser.hh"
#include "unicode.hh"
#include "fs.hh"
#include "util.hh"

#include <algorithm>
#include <stdexcept>

/// @file
/// Functions used for parsing the UltraStar TXT song format

using namespace SongParserUtil;

/// 'Magick' to check if this file looks like correct format
bool SongParser::txtCheck(std::string const& data) const {
	return data[0] == '#' && data[1] >= 'A' && data[1] <= 'Z';
}

/// Parse header data for Songs screen
void SongParser::txtParseHeader() {
	Song& s = m_song;
	std::string line;
	s.insertVocalTrack(TrackName::VOCAL_LEAD, VocalTrack(TrackName::VOCAL_LEAD)); // Dummy note to indicate there is a track
	while (getline(line) && txtParseField(line)) {}
	if (s.title.empty() || s.artist.empty()) throw std::runtime_error("Required header fields missing");
	if (!fs::exists(s.music[TrackName::BGMUSIC]))
	{
		s.loadStatus = Song::LoadStatus::ERROR;
		std::clog << "songparser/error: Required MP3 '" << s.music[TrackName::BGMUSIC].string() << "' file isn't available." << std::endl;
	}
	if (m_bpm != 0.0f) addBPM(0, m_bpm);
}

/// Parse notes
void SongParser::txtParse() {
	std::string line;
	m_curSinger = CurrentSinger::P1;
	if (!m_song.vocalTracks.empty()) { m_song.vocalTracks.clear(); }
	m_song.insertVocalTrack(TrackName::VOCAL_LEAD, VocalTrack(TrackName::VOCAL_LEAD));
	m_song.insertVocalTrack(DUET_P2, VocalTrack(DUET_P2));
	while (getline(line) && txtParseField(line)) {} // Parse the header again
	txtResetState();
	while (txtParseNote(line) && getline(line)) {} // Parse notes
	// Workaround for the terminating : 1 0 0 line, written by some converters
	// FIXME: Should we do this for all tracks?

	for (auto const& name: { TrackName::VOCAL_LEAD, DUET_P2 }) {
		Notes& notes = m_song.getVocalTrack(name).notes;
		auto it = notes.rbegin();
		if (!notes.empty() && it->type != Note::Type::SLEEP && it->begin == it->end) notes.pop_back();
		if (notes.empty()) m_song.eraseVocalTrack(name);
	}

	if (m_song.hasDuet()) {
		bool skip;
		Notes s1, s2, merged, finalDuet;
		s1 = m_song.getVocalTrack(TrackName::VOCAL_LEAD).notes;
		s2 = m_song.getVocalTrack(SongParserUtil::DUET_P2).notes;
		std::merge(s1.begin(), s1.end(), s2.begin(), s2.end(), std::back_inserter(merged), Note::ltBegin);
		VocalTracks const& tracks = m_song.vocalTracks;
		std::string duetName = tracks.at(TrackName::VOCAL_LEAD).name + " & " + tracks.at(SongParserUtil::DUET_P2).name;
		m_song.insertVocalTrack(SongParserUtil::DUET_BOTH, duetName);
		VocalTrack& duetTrack = m_song.getVocalTrack(SongParserUtil::DUET_BOTH);
		Notes& duetNotes = duetTrack.notes;
		duetNotes.clear();

		for (auto currentNote: merged) {
			skip = false;
			if (!finalDuet.empty()) {
				if (currentNote.type == Note::Type::SLEEP) {
					auto prevToLast = ++(finalDuet.rbegin());
					if (prevToLast->type == Note::Type::SLEEP) {
						std::clog << "songparser/info: Phrase formed by a single syllable is most likely our fault, We'll skip the break." << std::endl;
						skip = true;
					}
				}
				else {
					if (Note::overlapping(finalDuet.back(),currentNote)) {
						std::clog << "songparser/info: Will try to fix overlap (most likely between both singers) with a linebreak." << std::endl;
						Note lineBreak = Note();
						lineBreak.type = Note::Type::SLEEP;
						double beatDur = getBPM(m_song, finalDuet.back().begin).step;
						double newEnd = (currentNote.begin - 2*beatDur);
						lineBreak.begin = lineBreak.end = newEnd;
						if (finalDuet.back().type != Note::Type::SLEEP) {
							finalDuet.back().end = newEnd;
							if (currentNote.type == Note::Type::SLEEP) { skip = true; }
							if (!skip) { finalDuet.push_back(lineBreak); }
						}
					}
				}
			}
			if (!skip) { finalDuet.push_back(currentNote); }
		}
		auto finalNote = std::unique(finalDuet.begin(), finalDuet.end(), Note::equal);
		finalDuet.erase(finalNote, finalDuet.end());
		duetNotes.swap(finalDuet);
		duetTrack.noteMin = std::min(m_song.getVocalTrack(TrackName::VOCAL_LEAD).noteMin, m_song.getVocalTrack(SongParserUtil::DUET_P2).noteMin);
		duetTrack.noteMax = std::max(m_song.getVocalTrack(TrackName::VOCAL_LEAD).noteMax, m_song.getVocalTrack(SongParserUtil::DUET_P2).noteMax);
	}
}

bool SongParser::txtParseField(std::string const& line) {
	if (line.empty()) return true;
	if (line[0] != '#') return false;
	std::string::size_type pos = line.find(':');
	if (pos == std::string::npos) throw std::runtime_error("Invalid txt format, should be #key:value");
	std::string key = UnicodeUtil::toUpper(trim(line.substr(1, pos - 1)));
	std::string value = trim(line.substr(pos + 1));
	if (value.empty()) return true;

	// Parse header data that is stored in SongParser rather than in song (and thus needs to be read every time)
	if (key == "BPM") assign(m_bpm, value);
	else if (key == "RELATIVE") assign(m_relative, value);
	else if (key == "GAP") { assign(m_gap, value); m_gap *= 1e-3; }
	else if (key == "DUETSINGERP1" || key == "P1") m_song.insertVocalTrack(TrackName::VOCAL_LEAD, VocalTrack(value.substr(value.find_first_not_of(" "))));
	// Strong hint that this is a duet, so it will be readily displayed with two singers in browser and properly filtered
	else if (key == "DUETSINGERP2" || key == "P2") m_song.insertVocalTrack(DUET_P2, VocalTrack(value.substr(value.find_first_not_of(" "))));

	if (m_song.loadStatus >= Song::LoadStatus::HEADER) return true;  // Only re-parsing now, skip any other data

	// Parse header data that is directly stored in m_song
	if (key == "TITLE") m_song.title = value.substr(value.find_first_not_of(" :"));
	else if (key == "ARTIST") m_song.artist = value.substr(value.find_first_not_of(" "));
	else if (key == "EDITION") m_song.edition = value.substr(value.find_first_not_of(" "));
	else if (key == "GENRE") m_song.genre = value.substr(value.find_first_not_of(" "));
	else if (key == "CREATOR") m_song.creator = value.substr(value.find_first_not_of(" "));
	else if (key == "COVER") m_song.cover = absolute(value, m_song.path);
	else if (key == "MP3") m_song.music[TrackName::BGMUSIC] = absolute(value, m_song.path);
	else if (key == "VOCALS") m_song.music[TrackName::VOCAL_LEAD] = absolute(value, m_song.path);
	else if (key == "VIDEO") m_song.video = absolute(value, m_song.path);
	else if (key == "BACKGROUND") m_song.background = absolute(value, m_song.path);
	else if (key == "START") assign(m_song.start, value);
	else if (key == "VIDEOGAP") assign(m_song.videoGap, value);
	else if (key == "PREVIEWSTART") assign(m_song.preview_start, value);
	else if (key == "LANGUAGE") m_song.language = value.substr(value.find_first_not_of(" "));
	return true;
}

bool SongParser::txtParseNote(std::string line) {
	if (line.empty() || line == "\r") return true;
	if (line[0] == '#') throw std::runtime_error("Key found in the middle of notes");
	if (line[line.size() - 1] == '\r') line.erase(line.size() - 1);
	if (line[0] == 'E') return false;
	std::istringstream iss(line);
	if (line[0] == 'B') {
		unsigned int ts;
		float bpm;
		iss.ignore();
		if (!(iss >> ts >> bpm)) throw std::runtime_error("Invalid BPM line format");
		addBPM(ts, bpm);
		return true;
	}
	if (line[0] == 'P') {
		if (m_relative) // FIXME?
			throw std::runtime_error("Relative note timing not supported with multiple singers");
		if (line.size() < 2) throw std::runtime_error("Invalid player info line [too short]: " + line);
		else if (line[1] == '1') m_curSinger = CurrentSinger::P1;
		else if (line[1] == '2') m_curSinger = CurrentSinger::P2;
		else if (line[1] == '3') m_curSinger = CurrentSinger::BOTH;
		else if (line.size() < 3) throw std::runtime_error("Invalid player info line [too short]: " + line);
		else if (line[2] == '1') m_curSinger = CurrentSinger::P1;
		else if (line[2] == '2') m_curSinger = CurrentSinger::P2;
		else if (line[2] == '3') m_curSinger = CurrentSinger::BOTH;
		else throw std::runtime_error("Invalid player info line [malformed]: " + line);
		txtResetState();
		return true;
	}
	Note n;
	n.type = Note::Type(iss.get());
	unsigned int ts = m_txt.prevts;
	switch (n.type) {
		case Note::Type::NORMAL:
		case Note::Type::RAP:
		case Note::Type::FREESTYLE:
		case Note::Type::GOLDEN:
		case Note::Type::GOLDEN2:
		{
			unsigned int length = 0;
			if (!(iss >> ts >> length >> n.note)) throw std::runtime_error("Invalid note line format");
			if (length < 1) std::clog << "songparser/info: Notes must have positive durations." << std::endl;
			n.notePrev = n.note; // No slide notes in TXT yet.
			if (m_relative) ts += m_txt.relativeShift;
			if (iss.get() == ' ') std::getline(iss, n.syllable);
			n.end = tsTime(ts + length);
		}
		break;
		case Note::Type::SLEEP:
		{
			unsigned int end;
			if (!(iss >> ts >> end)) end = ts;
			if (m_relative) {
				ts += m_txt.relativeShift;
				end += m_txt.relativeShift;
				m_txt.relativeShift = end;
			}
			n.end = tsTime(end);
		}
		break;
		case Note::Type::SLIDE:
		case Note::Type::TAP:
		case Note::Type::HOLDBEGIN:
		case Note::Type::HOLDEND:
		case Note::Type::ROLL:
		case Note::Type::MINE:
		case Note::Type::LIFT:
		default:
			throw std::runtime_error("Unknown note type");
	}
	n.begin = tsTime(ts);
	VocalTrack& vocal = m_song.getVocalTrack(
	  (m_curSinger == CurrentSinger::P1) || (m_curSinger == CurrentSinger::BOTH)
	  ? TrackName::VOCAL_LEAD : DUET_P2);
	Notes& notes = vocal.notes;
	if (m_relative && notes.empty()) m_txt.relativeShift = ts;
	m_txt.prevts = ts;
	// FIXME: These work-arounds don't work for P3 (both singers) case
	if (n.begin < m_txt.prevtime) {
		// Oh no, overlapping notes (b0rked file)
		// Can't do this because too many songs are b0rked: throw std::runtime_error("Note overlaps with previous note");
		if (notes.size() >= 1) {
			Note& p = notes.back();
			if (p.begin > n.begin) {
				std::string msg = "Skipped overlapping notes:\n" + p.syllable + ", " + n.syllable + "\n";
				m_song.b0rked += msg;
				std::clog << "songparser/notice: " + m_song.filename.string() + ": " + msg << std::endl;
				return true;
			}
		} else throw std::runtime_error("The first note has negative timestamp");
	}
	double prevtime = m_txt.prevtime;
	m_txt.prevtime = n.end;
	if (n.type != Note::Type::SLEEP && n.end > n.begin) {
		vocal.noteMin = std::min(vocal.noteMin, n.note);
		vocal.noteMax = std::max(vocal.noteMax, n.note);
	}
	if (n.type == Note::Type::SLEEP) {
		if (notes.empty()) return true; // Ignore sleeps at song beginning
		else {
			Note& p = notes.back();
			n.begin = n.end = prevtime; // Normalize sleep notes

			if (p.type == Note::Type::SLEEP) return true; // Ignore consecutive sleeps
		}
	}
	notes.push_back(n);
	if (m_curSinger == CurrentSinger::BOTH) { m_song.getVocalTrack(DUET_P2).notes.push_back(n); }
	return true;
}

void SongParser::txtResetState() {
	m_txt = TXTState();
	m_song.m_bpms.clear();
	if (m_bpm != 0.0f) { addBPM (0, m_bpm); }
}


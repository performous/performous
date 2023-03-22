#include "songparser.hh"

#include "fs.hh"
#include "log.hh"
#include "unicode.hh"
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
void SongParser::txtParseHeader(Song& song) {
	std::string line;
	song.insertVocalTrack(TrackName::VOCAL_LEAD, VocalTrack(TrackName::VOCAL_LEAD)); // Dummy note to indicate there is a track
	while (getline(line) && txtParseField(song, line)) {}
	if (song.title.empty() || song.artist.empty()) throw SongParserException(song, "Required header fields missing", 0);
	if (!fs::exists(song.music[TrackName::BGMUSIC]))
	{
		song.loadStatus = Song::LoadStatus::PARSERERROR;
		SpdLogger::error(LogSystem::SONGPARSER, "TXT Parser ({}) -- Required song file is not available at path={}", song.filename, song.music[TrackName::BGMUSIC].string());
	}
	if (m_bpm != 0.0f)
		addBPM(song, 0, m_bpm);
}

/// Parse notes
void SongParser::txtParse(Song& song) {
	std::string line;
	m_curSinger = CurrentSinger::P1;
	if (!song.vocalTracks.empty()) { song.vocalTracks.clear(); }
	song.insertVocalTrack(TrackName::VOCAL_LEAD, VocalTrack(TrackName::VOCAL_LEAD));
	song.insertVocalTrack(DUET_P2, VocalTrack(DUET_P2));
	while (getline(line) && txtParseField(song, line)) {} // Parse the header again
	txtResetState(song);
	while (txtParseNote(song, line) && getline(line)) {} // Parse notes
	// Workaround for the terminating : 1 0 0 line, written by some converters
	// FIXME: Should we do this for all tracks?

	for (auto const& name: { TrackName::VOCAL_LEAD, DUET_P2 }) {
		Notes& notes = song.getVocalTrack(name).notes;
		auto it = notes.rbegin();
		if (!notes.empty() && it->type != Note::Type::SLEEP && it->begin == it->end) notes.pop_back();
		if (notes.empty()) song.eraseVocalTrack(name);
	}

	if (song.hasDuet()) {
		bool skip;
		Notes s1, s2, merged, finalDuet;
		s1 = song.getVocalTrack(TrackName::VOCAL_LEAD).notes;
		s2 = song.getVocalTrack(SongParserUtil::DUET_P2).notes;
		std::merge(s1.begin(), s1.end(), s2.begin(), s2.end(), std::back_inserter(merged), Note::ltBegin);
		VocalTracks const& tracks = song.vocalTracks;
		std::string duetName = tracks.at(TrackName::VOCAL_LEAD).name + " & " + tracks.at(SongParserUtil::DUET_P2).name;
		song.insertVocalTrack(SongParserUtil::DUET_BOTH, duetName);
		VocalTrack& duetTrack = song.getVocalTrack(SongParserUtil::DUET_BOTH);
		Notes& duetNotes = duetTrack.notes;
		duetNotes.clear();

		for (auto currentNote: merged) {
			skip = false;
			if (!finalDuet.empty()) {
				if (currentNote.type == Note::Type::SLEEP) {
					auto prevToLast = ++(finalDuet.rbegin());
					if (prevToLast->type == Note::Type::SLEEP) {
						SpdLogger::info(LogSystem::SONGPARSER, "TXT Parser ({}) -- Phrase formed by a single syllable is most likely our fault, We'll skip the break.", song.filename);
						skip = true;
					}
				}
				else {
					if (Note::overlapping(finalDuet.back(),currentNote)) {
						SpdLogger::info(LogSystem::SONGPARSER, "TXT Parser ({}) -- Will try to fix overlap (most likely between both singers) with a linebreak.", song.filename);
						Note lineBreak = Note();
						lineBreak.type = Note::Type::SLEEP;
						double beatDur = getBPM(song, finalDuet.back().begin).step;
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
		duetTrack.noteMin = std::min(song.getVocalTrack(TrackName::VOCAL_LEAD).noteMin, song.getVocalTrack(SongParserUtil::DUET_P2).noteMin);
		duetTrack.noteMax = std::max(song.getVocalTrack(TrackName::VOCAL_LEAD).noteMax, song.getVocalTrack(SongParserUtil::DUET_P2).noteMax);
	}
}

bool SongParser::txtParseField(Song& song, std::string const& line) {
	if (line.empty()) return true;
	if (line[0] != '#') return false;
	std::string::size_type pos = line.find(':');
	if (pos == std::string::npos) throw SongParserException(song, "Invalid txt format, should be #key:value", m_linenum);
	std::string key = UnicodeUtil::toUpper(trim(line.substr(1, pos - 1)));
	std::string value = trim(line.substr(pos + 1));
	if (value.empty()) return true;

	// Parse header data that is stored in SongParser rather than in song (and thus needs to be read every time)
	if (key == "BPM") assign(m_bpm, value);
	else if (key == "RELATIVE") assign(m_relative, value);
	else if (key == "GAP") { assign(m_gap, value); m_gap *= 1e-3; }
	else if (key == "DUETSINGERP1" || key == "P1") song.insertVocalTrack(TrackName::VOCAL_LEAD, VocalTrack(value.substr(value.find_first_not_of(" "))));
	// Strong hint that this is a duet, so it will be readily displayed with two singers in browser and properly filtered
	else if (key == "DUETSINGERP2" || key == "P2") song.insertVocalTrack(DUET_P2, VocalTrack(value.substr(value.find_first_not_of(" "))));

	if (song.loadStatus >= Song::LoadStatus::HEADER) return true;  // Only re-parsing now, skip any other data

	// Parse header data that is directly stored in song
	if (key == "TITLE") song.title = value.substr(value.find_first_not_of(" :"));
	else if (key == "ARTIST") song.artist = value.substr(value.find_first_not_of(" "));
	else if (key == "EDITION") song.edition = value.substr(value.find_first_not_of(" "));
	else if (key == "GENRE") song.genre = value.substr(value.find_first_not_of(" "));
	else if (key == "CREATOR") song.creator = value.substr(value.find_first_not_of(" "));
	else if (key == "COVER") song.cover = absolute(value, song.path);
	else if (key == "MP3" || key == "AUDIO") song.music[TrackName::BGMUSIC] = absolute(value, song.path);
	else if (key == "INSTRUMENTAL") song.music[TrackName::INSTRUMENTAL] = absolute(value, song.path);
	else if (key == "VOCALS") song.music[TrackName::VOCAL_LEAD] = absolute(value, song.path);
	else if (key == "VIDEO") song.video = absolute(value, song.path);
	else if (key == "BACKGROUND") song.background = absolute(value, song.path);
	else if (key == "START") assign(song.start, value);
	else if (key == "END") assign(song.end, value);
	else if (key == "YEAR") assign(song.year, value);
	else if (key == "VIDEOGAP") assign(song.videoGap, value);
	else if (key == "PREVIEWSTART") assign(song.preview_start, value);
	else if (key == "LANGUAGE") song.language = value.substr(value.find_first_not_of(" "));
	else if (key == "PROVIDEDBY") song.providedBy = value.substr(value.find_first_not_of(" "));
	else if (key == "COMMENT") song.comment = value.substr(value.find_first_not_of(" "));
	else if (key == "TAGS") song.tags = value.substr(value.find_first_not_of(" "));
	return true;
}

bool SongParser::txtParseNote(Song& song, std::string line) {
    const int MAX_STARTBEAT = 262144; // 2^18, about 2 hours on an average song (depends on BPM)
    const int MAX_LENGTH = 2048; // A very long note
	if (line.empty() || line == "\r") return true;
	if (line[0] == '#') throw SongParserException(song, "Key found in the middle of notes", m_linenum);
	if (line[line.size() - 1] == '\r') line.erase(line.size() - 1);
	if (line[0] == 'E') return false;
	std::istringstream iss(line);
	if (line[0] == 'B') {
		int ts;
		float bpm;
		iss.ignore();
		if (!(iss >> ts >> bpm) || ts < 0 || ts > MAX_STARTBEAT)
        	throw SongParserException(song, "Invalid BPM line format", m_linenum);
		addBPM(song, static_cast<unsigned int>(ts), bpm);
		return true;
	}
	if (line[0] == 'P') {
		if (m_relative) // FIXME?
			throw SongParserException(song, "Relative note timing not supported with multiple singers", m_linenum);
		if (line.size() < 2) throw SongParserException(song, fmt::format("Invalid player info line [too short]: {}", line), m_linenum);
		else if (line[1] == '1') m_curSinger = CurrentSinger::P1;
		else if (line[1] == '2') m_curSinger = CurrentSinger::P2;
		else if (line[1] == '3') m_curSinger = CurrentSinger::BOTH;
		else if (line.size() < 3) throw SongParserException(song, fmt::format("Invalid player info line [too short]: {}", line), m_linenum);
		else if (line[2] == '1') m_curSinger = CurrentSinger::P1;
		else if (line[2] == '2') m_curSinger = CurrentSinger::P2;
		else if (line[2] == '3') m_curSinger = CurrentSinger::BOTH;
		else throw SongParserException(song, fmt::format("Invalid player info line [malformed]: {}", line), m_linenum);
		txtResetState(song);
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
		case Note::Type::GOLDENRAP:
		{
			int readTs = 0;  // read as signed int to check for negative values
			int readLength = 0;
			unsigned int length = 0;
			if (!(iss >> readTs >> readLength >> n.note) || readTs < 0 || readTs > MAX_STARTBEAT || readLength < 0 || readLength > MAX_LENGTH)
				throw SongParserException(song, "Invalid note line format", m_linenum);
			ts = static_cast<unsigned int>(readTs);
			length = static_cast<unsigned int>(readLength);
			if (length < 1) {
				SpdLogger::info(LogSystem::SONGPARSER, "TXT Parser ({}, line {}) -- Notes must have positive durations.", song.filename, m_linenum);
				length = 1;
			}
			n.notePrev = n.note; // No slide notes in TXT yet.
			if (m_relative) ts += m_txt.relativeShift;
			if (iss.get() == ' ') std::getline(iss, n.syllable);
			n.end = tsTime(song, ts + length);
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
			n.end = tsTime(song, end);
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
	n.begin = tsTime(song, ts);
	VocalTrack& vocal = song.getVocalTrack(
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
				std::string msg{fmt::format("Skipped overlapping notes: {} and {}.", p.syllable, n.syllable)};
				fmt::format_to(std::back_inserter(song.b0rked), fmt::runtime("{}{}"), !song.b0rked.empty() ? "\n" : "", msg);
				SpdLogger::notice(LogSystem::SONGPARSER, "TXT Parser ({}) -- {}.", song.filename, msg);
				return true;
			}
		} else throw SongParserException(song, "The first note has a negative timestamp");
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
	if (m_curSinger == CurrentSinger::BOTH) { song.getVocalTrack(DUET_P2).notes.push_back(n); }
	return true;
}

void SongParser::txtResetState(Song& song) {
	m_txt = TXTState();
	song.m_bpms.clear();
	if (m_bpm != 0.0f) { addBPM (song, 0, m_bpm); }
}

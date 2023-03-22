#include "songparser.hh"

#include <boost/algorithm/string.hpp>
#include <stdexcept>
#include "midifile.hh"

/// @file
/// Functions used for parsing MIDI files (FoF and other song formats)

using namespace SongParserUtil;

const std::string HARMONIC_1 = "Harmonic 1";
const std::string HARMONIC_2 = "Harmonic 2";
const std::string HARMONIC_3 = "Harmonic 3";

namespace {
	bool isVocalTrack(std::string name) {
		if(name == TrackName::VOCAL_LEAD) return true;
		else if(name == TrackName::VOCAL_BACKING) return true;
		else if(name == HARMONIC_1) return true;
		else if(name == HARMONIC_2) return true;
		else if(name == HARMONIC_3) return true;
		return false;
	}
	/// Change the MIDI track name to Performous track name
	/// Return false if not valid
	bool mangleTrackName(std::string& name) {
		if (name == "T1 GEMS") { // Some old MIDI files have a track named T1 GEMS
			name = TrackName::GUITAR; return true;
		}
		else if (name.substr(0, 5) != "PART ") return false;
		else name.erase(0, 5);
		if (name == "GUITAR COOP") name = TrackName::GUITAR_COOP;
		else if (name == "RHYTHM") name = TrackName::GUITAR_RHYTHM;
		else if (name == "DRUM") name = TrackName::DRUMS;
		else if (name == "DRUMS") name = TrackName::DRUMS;
		else if (name == "BASS") name = TrackName::BASS;
		else if (name == "KEYS") return false; // TODO: RB3 5 lane keyboard track
		else if (name == "GUITAR") name = TrackName::GUITAR;
		else if (name == "VOCALS") name = TrackName::VOCAL_LEAD;
		else if (name == "HARM1") name = HARMONIC_1;
		else if (name == "HARM2") name = HARMONIC_2;
		else if (name == "HARM3") name = HARMONIC_3;
		// expert stuffs
		else if (name == "REAL_KEYS_X") return false; // TODO: RB3 pro keyboard expert track
		else if (name == "REAL_KEYS_H") return false; // TODO: RB3 pro keyboard hard track
		else if (name == "REAL_KEYS_M") return false; // TODO: RB3 pro keyboard medium track
		else if (name == "REAL_KEYS_E") return false; // TODO: RB3 pro keyboard easy track
		else if (name == "REAL_BASS") return false; // TODO: RB3 pro bass track
		else if (name == "REAL_GUITAR") return false; // TODO: RB3 pro guitar 17 frets (Mustang) track
		else if (name == "REAL_GUITAR_22") return false; // TODO: RB3 pro guitar 22 frets (Squier) track
		else return false;
		return true;
	}
}

void SongParser::midParseHeader(Song& song) {
	if (!song.vocalTracks.empty()) {
		song.vocalTracks.clear();
	}
	if (!song.instrumentTracks.empty()) {
		song.instrumentTracks.clear();
	}
	// Parse tracks from midi
	MidiFileParser midi(song.midifilename);
	for (MidiFileParser::Tracks::const_iterator it = midi.tracks.begin(); it != midi.tracks.end(); ++it) {
		// Figure out the track name
		std::string name = it->name;
		if (mangleTrackName(name)) ; // Beautify the track name
		else if (midi.tracks.size() == 1) name = TrackName::GUITAR; // Original (old) FoF songs only have one track
		else continue; // not a valid track
		// Add dummy notes to tracks so that they can be seen in song browser
		if (isVocalTrack(name)) song.insertVocalTrack(name, VocalTrack(name));
		else {
			for (auto const& elem: it->notes) {
				// If a track has not enough notes on any level, ignore it
				if (elem.second.size() > 3) { song.instrumentTracks.insert(make_pair(name,InstrumentTrack(name))); break; }
			}
		}
	}
	addBPM(song, 0, static_cast<float>(6e7 / midi.tempochanges.front().value));
	std::clog << "songparser-mid/debug: Got a bpm: " << (6e7 / midi.tempochanges.front().value) << std::endl;
}

/// Parse notes
void SongParser::midParse(Song& song) {
	song.instrumentTracks.clear();

	MidiFileParser midi(song.midifilename);
	int reversedNoteCount = 0;
	for (std::uint32_t ts = 0, end = midi.ts_last + midi.division; ts < end; ts += midi.division) song.beats.push_back(midi.get_seconds(ts) + song.start);
	for (MidiFileParser::Tracks::const_iterator it = midi.tracks.begin(); it != midi.tracks.end(); ++it) {
		// Figure out the track name
		std::string name = it->name;
		if (mangleTrackName(name)) ; // Beautify the track name
		else if (midi.tracks.size() == 1) name = TrackName::GUITAR; // Original (old) FoF songs only have one track
		else continue; // not a valid track
		if (!isVocalTrack(name)) {
			// Process non-vocal tracks
			double trackEnd = 0.0;
			song.instrumentTracks.insert(make_pair(name,InstrumentTrack(name)));
			NoteMap& nm2 = song.instrumentTracks.find(name)->second.nm;
			for (auto const& elem: it->notes) {
				Durations& dur = nm2[elem.first];
				MidiFileParser::Notes const& notes = elem.second;
				for (auto const& note: notes) {
					double beg = midi.get_seconds(note.begin) + song.start;
					double end = midi.get_seconds(note.end) + song.start;
					if (end == 0) continue; // Note with no ending
					if (beg > end) { // Reversed note
						if (beg - end > 0.001) { reversedNoteCount++; continue; }
						else end = beg; // Allow 1ms error to counter rounding etc errors
					}
					dur.push_back(Duration(beg, end));
					if (trackEnd < end) trackEnd = end;
				}
			}
			// Discard empty tracks
			// Note: some songs have notes at the very beginning (but are otherwise empty)
			if (trackEnd < 1.0) song.instrumentTracks.erase(name);
		} else {
			// Process vocal tracks
			VocalTrack vocal(name);
			for (auto const& lyric: it->lyrics) {
				Note n;
				n.begin = midi.get_seconds(lyric.begin) + song.start;
				n.end = midi.get_seconds(lyric.end) + song.start;
				n.notePrev = n.note = lyric.note;
				n.type = n.note > 100 ? Note::Type::SLEEP : Note::Type::NORMAL;
				if(n.note == 116 || n.note == 103 || n.note == 124)
					continue; // managed in the next loop (GOLDEN/FREESTYLE notes)
				else if(n.note > 100) // is it always 105 ?
					n.type = Note::Type::SLEEP;
				else
					n.type = Note::Type::NORMAL;
				{
					n.syllable = UnicodeUtil::convertToUTF8(lyric.lyric);
				}
				std::string& syl = n.syllable;
				if (n.type != Note::Type::SLEEP) {
					if (!syl.empty()) {
						bool erase = false;
						// Examine note styles (specified by the last character of the syllable)
						{
							char& ch = *syl.rbegin();
							if (ch == '#') { n.type = Note::Type::FREESTYLE; erase = true; }
							if (ch == '^') { n.type = Note::Type::GOLDEN; erase = true; }
							if (ch == '+') { n.type = Note::Type::SLIDE; ch = '~'; }
						}
						if (erase) syl.erase(syl.size() - 1);
						// Add spaces between words, remove hyphens
						{
							char ch = *syl.rbegin();
							if (ch == '-') syl.erase(syl.size() - 1);
							else if (ch == '=') { *syl.rbegin() = '-'; }
							else if (ch != '~') syl += ' ';
						}
					}
					// Special processing for slides (which depend on the previous note)
					if (n.type == Note::Type::SLIDE) {
						auto prev = vocal.notes.rbegin();
						while (prev != vocal.notes.rend() && prev->type == Note::Type::SLEEP) ++prev;
						if (prev == vocal.notes.rend()) throw std::runtime_error("The song begins with a slide note");
						eraseLast(prev->syllable); // Erase the space if there is any
						{
							// insert new sliding note
							Note inter;
							inter.begin = prev->end;
							inter.end = n.begin;
							inter.notePrev = prev->note;
							inter.note = n.note;
							inter.type = Note::Type::SLIDE;
							inter.syllable = std::string("~");
							vocal.noteMin = std::min(vocal.noteMin, inter.note);
							vocal.noteMax = std::max(vocal.noteMax,inter.note);
							vocal.notes.push_back(inter);
						}
						{
							// modifying current note to be normal again
							n.type = Note::Type::NORMAL;
						}
					}
					vocal.noteMin = std::min(vocal.noteMin, n.note);
					vocal.noteMax = std::max(vocal.noteMax, n.note);
					vocal.notes.push_back(n);
				} else if (!vocal.notes.empty() && vocal.notes.back().type != Note::Type::SLEEP) {
					eraseLast(vocal.notes.back().syllable);
					vocal.notes.push_back(n);
				}
			}
			for (auto const& lyric: it->lyrics) {
				if(lyric.note == 116 || lyric.note == 103 || lyric.note == 124) {
					for (auto& n: vocal.notes) {
						if (n.begin == midi.get_seconds(lyric.begin) + song.start && n.type == Note::Type::NORMAL) {
							if (lyric.note == 124) {
								n.type = Note::Type::FREESTYLE;
							} else {
								n.type = Note::Type::GOLDEN;
							}
							break;
						}
					}
				}
			}
			song.insertVocalTrack(name, vocal);
		}
	}
	// Figure out if we have BRE in the song
	for (MidiFileParser::CommandEvents::const_iterator it = midi.cmdevents.begin(); it != midi.cmdevents.end(); ++it) {
		if (*it == "[section big_rock_ending]") song.hasBRE = true;
	}
	// Output some warning
	if (reversedNoteCount > 0) {
		std::ostringstream oss;
		oss << "songparser/notice: Skipping " << reversedNoteCount << " reversed note(s) in " << song.midifilename.string();
		std::clog << oss.str() << std::endl; // More likely to be atomic when written as one string
	}
	// copy midi sections to song section
	// design goals: (1) keep midi parser free of dependencies on song (2) store data in song as parsers are discarded before song
	// one option would be to pass a song reference to the midi parser however, that conflicts with goal (1)
	for (auto& sect: midi.midisections) {
		song.songsections.emplace_back(sect.name, sect.begin);
	}
}



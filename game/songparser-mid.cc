#include "songparser.hh"

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <stdexcept>
#include "midifile.hh"

/// @file
/// Functions used for parsing MIDI files (FoF and other song formats)

using namespace SongParserUtil;

const std::string HARMONIC_1 = "Harmonic 1";
const std::string HARMONIC_2 = "Harmonic 2";
const std::string HARMONIC_3 = "Harmonic 3";

namespace {
	void testAndAdd(Song& s, std::string const& trackid, std::string const& filename) {
		std::string f = s.path + filename;
		if (boost::filesystem::exists(f)) s.music[trackid] = f;
	}
	bool isVocalTrack(std::string name) {
		if(name == TrackName::LEAD_VOCAL) return true;
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
		else if (name == "VOCALS") name = TrackName::LEAD_VOCAL;
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

void SongParser::midParseHeader() {
	Song& s = m_song;
	// Parse tracks from midi
	MidiFileParser midi(s.path + "/" + s.midifilename);
	for (MidiFileParser::Tracks::const_iterator it = midi.tracks.begin(); it != midi.tracks.end(); ++it) {
		// Figure out the track name
		std::string name = it->name;
		if (mangleTrackName(name)) ; // Beautify the track name
		else if (midi.tracks.size() == 1) name = TrackName::GUITAR; // Original (old) FoF songs only have one track
		else continue; // not a valid track
		// Add dummy notes to tracks so that they can be seen in song browser
		if (isVocalTrack(name)) s.insertVocalTrack(name, VocalTrack(name));
		else {
			for (MidiFileParser::NoteMap::const_iterator it2 = it->notes.begin(); it2 != it->notes.end(); ++it2) {
				// If a track has not enough notes on any level, ignore it
				if (it2->second.size() > 3) { s.instrumentTracks.insert(make_pair(name,InstrumentTrack(name))); break; }
			}
		}
	}
}

/// Parse notes
void SongParser::midParse() {
	Song& s = m_song;
	s.instrumentTracks.clear();

	MidiFileParser midi(s.path + "/" + s.midifilename);
	int reversedNoteCount = 0;
	for (uint32_t ts = 0, end = midi.ts_last + midi.division; ts < end; ts += midi.division) s.beats.push_back(midi.get_seconds(ts)+s.start);
	for (MidiFileParser::Tracks::const_iterator it = midi.tracks.begin(); it != midi.tracks.end(); ++it) {
		// Figure out the track name
		std::string name = it->name;
		if (mangleTrackName(name)) ; // Beautify the track name
		else if (midi.tracks.size() == 1) name = TrackName::GUITAR; // Original (old) FoF songs only have one track
		else continue; // not a valid track
		if (!isVocalTrack(name)) {
			// Process non-vocal tracks
			int durCount = 0;
			s.instrumentTracks.insert(make_pair(name,InstrumentTrack(name)));
			NoteMap& nm2 = s.instrumentTracks.find(name)->second.nm;
			for (MidiFileParser::NoteMap::const_iterator it2 = it->notes.begin(); it2 != it->notes.end(); ++it2) {
				Durations& dur = nm2[it2->first];
				MidiFileParser::Notes const& notes = it2->second;
				for (MidiFileParser::Notes::const_iterator it3 = notes.begin(); it3 != notes.end(); ++it3) {
					double beg = midi.get_seconds(it3->begin)+s.start;
					double end = midi.get_seconds(it3->end)+s.start;
					if (end == 0) continue; // Note with no ending
					if (beg > end) { // Reversed note
						if (beg - end > 0.001) { reversedNoteCount++; continue; }
						else end = beg; // Allow 1ms error to counter rounding etc errors
					}
					dur.push_back(Duration(beg, end));
					durCount++;
				}
			}
			// If a track has only 20 or less notes it's most likely b0rked.
			// This number has been extracted from broken song tracks, but
			// it is probably DIFFICULTYCOUNT * different_frets.
			if (durCount <= 20) {
				s.b0rkedTracks = true;
				nm2.clear();
				std::ostringstream oss;
				oss << "Track " << name << " is broken in ";
				oss << s.path << s.midifilename << std::endl;
				std::clog << "songparser/warning: " << oss.str(); // More likely to be atomic when written as one string
				s.instrumentTracks.erase(name);
			}
		} else {
			// Process vocal tracks
			VocalTrack vocal(name);
			for (MidiFileParser::Lyrics::const_iterator it2 = it->lyrics.begin(); it2 != it->lyrics.end(); ++it2) {
				Note n;
				n.begin = midi.get_seconds(it2->begin)+s.start;
				n.end = midi.get_seconds(it2->end)+s.start;
				n.notePrev = n.note = it2->note;
				n.type = n.note > 100 ? Note::SLEEP : Note::NORMAL;
				if(n.note == 116 || n.note == 103 || n.note == 124)
					continue; // managed in the next loop (GOLDEN/FREESTYLE notes)
				else if(n.note > 100) // is it always 105 ?
					n.type = Note::SLEEP;
				else
					n.type = Note::NORMAL;
				{
					std::stringstream ss(it2->lyric);
					convertToUTF8(ss);
					n.syllable = ss.str();
				}
				std::string& syl = n.syllable;
				if (n.type != Note::SLEEP) {
					if (!syl.empty()) {
						bool erase = false;
						// Examine note styles (specified by the last character of the syllable)
						{
							char& ch = *syl.rbegin();
							if (ch == '#') { n.type = Note::FREESTYLE; erase = true; }
							if (ch == '^') { n.type = Note::GOLDEN; erase = true; }
							if (ch == '+') { n.type = Note::SLIDE; ch = '~'; }
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
					if (n.type == Note::SLIDE) {
						Notes::reverse_iterator prev = vocal.notes.rbegin();
						while (prev != vocal.notes.rend() && prev->type == Note::SLEEP) ++prev;
						if (prev == vocal.notes.rend()) throw std::runtime_error("The song begins with a slide note");
						eraseLast(prev->syllable); // Erase the space if there is any
						{
							// insert new sliding note
							Note inter;
							inter.begin = prev->end;
							inter.end = n.begin;
							inter.notePrev = prev->note;
							inter.note = n.note;
							inter.type = Note::SLIDE;
							inter.syllable = std::string("~");
							vocal.noteMin = std::min(vocal.noteMin, inter.note);
							vocal.noteMax = std::max(vocal.noteMax,inter.note);
							vocal.notes.push_back(inter);
						}
						{
							// modifying current note to be normal again
							n.type = Note::NORMAL;
						}
					}
					vocal.noteMin = std::min(vocal.noteMin, n.note);
					vocal.noteMax = std::max(vocal.noteMax, n.note);
					vocal.notes.push_back(n);
				} else if (!vocal.notes.empty() && vocal.notes.back().type != Note::SLEEP) {
					eraseLast(vocal.notes.back().syllable);
					vocal.notes.push_back(n);
				}
			}
			for (MidiFileParser::Lyrics::const_iterator it2 = it->lyrics.begin(); it2 != it->lyrics.end(); ++it2) {
				if(it2->note == 116 || it2->note == 103 || it2->note == 124) {
					for(Notes::iterator it3 = vocal.notes.begin() ; it3 != vocal.notes.end(); ++it3) {
						if(it3->begin == midi.get_seconds(it2->begin)+s.start && it3->type == Note::NORMAL) {
							if(it2->note == 124) {
								it3->type = Note::FREESTYLE;
							} else {
								it3->type = Note::GOLDEN;
							}
							break;
						}
					}
				}
			}
			s.insertVocalTrack(name, vocal);
		}
	}
	// Figure out if we have BRE in the song
	for (MidiFileParser::CommandEvents::const_iterator it = midi.cmdevents.begin(); it != midi.cmdevents.end(); ++it) {
		if (*it == "[section big_rock_ending]") s.hasBRE = true;
	}
	// Output some warning
	if (reversedNoteCount > 0) {
		std::ostringstream oss;
		oss << "Skipping " << reversedNoteCount << " reversed note(s) in ";
		oss << s.path << s.midifilename << std::endl;
		std::clog << "songparser/warning: " << oss.str(); // More likely to be atomic when written as one string
	}
	// copy midi sections to song section
	// design goals: (1) keep midi parser free of dependencies on song (2) store data in song as parsers are discarded before song
	// one option would be to pass a song reference to the midi parser however, that conflicts with goal (1)
	for (std::vector<MidiFileParser::MidiSection>::iterator it= midi.midisections.begin(); it != midi.midisections.end(); ++it) {
		Song::SongSection tmp(it->name, it->begin);
		s.songsections.push_back(tmp);
	}

}



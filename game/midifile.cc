#include "midifile.hh"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>

#define MIDI_DEBUG_LEVEL 0


/**
 * @short The MidiStream class reads midifile for MidiFileParser.
 */

class MidiStream {
  public:

	/** Constructor.
	 *
	 * Creates MidiStream object that reads given midiFile.
	 *
	 * @param file MidiFile to be read
	 */
	MidiStream(std::string const& file) {
		std::ifstream ifs(file.c_str(), std::ios::binary);
		f << ifs.rdbuf();
		f.exceptions(std::ios::failbit);
	}

	/// read bytes
	std::string read_bytes(size_t bytes);

	class Riff {
	  public:
		MidiStream& ms;
		std::string name;
		size_t pos;
		size_t size;
		size_t offset;
		Riff(MidiStream& ms);
		~Riff();
		bool has_more_data() { return offset < size; }
		uint8_t read_uint8() { consume(1); return ms.f.get(); }
		uint16_t read_uint16() { consume(2); return ms.read_uint16(); }
		uint32_t read_uint32() { consume(4); return ms.read_uint32(); }
		uint32_t read_varlen();
		template <typename T> T read(T& value) {
			consume(sizeof(T));
			value = 0;
			for (int i = sizeof(T) - 1; i >= 0; --i) value |= ms.f.get() << (8 * i);
			return value;
		}
		std::string read_bytes(size_t size) { consume(size); return ms.read_bytes(size); }
		void ignore(size_t size) { consume(size); ms.f.ignore(size); }
		void seek_back(size_t offset = 1);
	  private:
		void consume(size_t bytes);
	};

private:

	std::stringstream f;
	uint16_t read_uint16() { return f.get() << 8 | f.get(); }
	uint32_t read_uint32() { return f.get() << 24 | f.get() << 16 | f.get() << 8 | f.get(); }

};

namespace { bool is_not_alpha(char c) { return (c < 'A' || c > 'Z') && (c < 'a' || c > 'z'); } }

MidiStream::Riff::Riff(MidiStream& ms): ms(ms), name(ms.read_bytes(4)), size(ms.read_uint32()), offset(0) {
	if (std::find_if(name.begin(), name.end(), is_not_alpha) != name.end()) throw std::runtime_error("Invalid RIFF chunk name");
	pos = ms.f.tellg();
}

MidiStream::Riff::~Riff() {
#if MIDI_DEBUG_LEVEL > 0
	if (has_more_data()) std::cout << "WARNING: Only " << offset << " of " << size << " bytes read of RIFF chunk " << name << std::endl;
#endif
	ms.f.seekg(pos + size);
}

uint32_t MidiStream::Riff::read_varlen() {
	unsigned long value = 0;
	size_t a = 0;
	unsigned char c;
	do {
		if (++a > 4) throw std::runtime_error("Too long varlen sequence");
		consume(1);
		c = ms.f.get();
		value = (value << 7) | (c & 0x7F);
	} while (c & 0x80);
	return value;
}

std::string MidiStream::read_bytes(size_t bytes) {
	std::string data;
	for(size_t i=0; i < bytes; ++i) data += f.get();
	return data;
}

void MidiStream::Riff::consume(size_t bytes) {
	if (size - offset < bytes) throw std::runtime_error("Read past the end of RIFF chunk " + name);
	ms.f.seekg(pos + offset);
	offset += bytes;
}

void MidiStream::Riff::seek_back(size_t o) {
	if (offset < o) throw std::runtime_error("Seek past the beginning of RIFF chunk " + name);
	offset -= o;
	ms.f.seekg(pos + offset);
}


MidiFileParser::MidiFileParser(std::string name):
  format(0), division(0), ts_last(0)
{
	MidiStream stream(name.c_str());
	size_t ntracks = parse_header(stream);
	if (format > 0) {
		// First track is a control track
		read_track(stream);
		--ntracks;
	}
	for (size_t i = 0; i < ntracks; ++i) tracks.push_back(read_track(stream));
}

uint16_t MidiFileParser::parse_header(MidiStream& stream) {
	MidiStream::Riff riff(stream);
	if (riff.name != "MThd") throw std::runtime_error("Header not found");
	if (riff.read(format) > 1) throw std::runtime_error("Unsupported MIDI format (only 0 and 1 are supported)");
	uint16_t ntracks = riff.read_uint16();
	if ((format == 0 && ntracks != 1) || (format == 1 && ntracks < 2)) throw std::runtime_error("Invalid number of tracks");
	riff.read(division);
	if (division & 0x8000) throw std::runtime_error("SMPTE type divisions not supported");
#if MIDI_DEBUG_LEVEL > 1
	std::cout << "Division: " << division << std::endl;
#endif
	return ntracks;
}

MidiFileParser::Track MidiFileParser::read_track(MidiStream& stream) {
	MidiStream::Riff riff(stream);
	if (riff.name != "MTrk") throw std::runtime_error("Chunk MTrk not found");
	Track track;
	uint32_t miditime = 0;
	uint8_t runningstatus = 0;
	bool end = false;
	while (!end) {
		miditime += riff.read_varlen();
		uint8_t event = riff.read_uint8();

		if (event & 0x80) {
			// Store current status, with exceptions:
			// * Not stored for RealTime Category messages (0xF8..0xFF)
			// * Running status cleared for System Common Category (0xF0..0xF7)
			if (event < 0xF8) runningstatus = (event < 0xF0 ? event : 0);
		} else {
			riff.seek_back();
			if (!runningstatus) throw std::runtime_error("Invalid MIDI file (first MIDI Event of a track wants running status)");
			event = runningstatus;
		}

		if (event == 0xFF) {
			// Meta event
			uint8_t type = riff.read_uint8();
			std::string data = riff.read_bytes(riff.read_varlen());
			switch (type) {
			  case 0x01:
			  	// Lyrics are hidden here, only [text] are orders
			  	if( data[0] != '[' ) m_lyric = data;
#if MIDI_DEBUG_LEVEL > 2
				std::cout << "Text: " << data << std::endl;
#endif
				break;
			  case 0x03:
				track.name = data;
#if MIDI_DEBUG_LEVEL > 1
				std::cout << "Track name: " << data << std::endl;
#endif
				break;
			  case 0x05:
				m_lyric = data;
#if MIDI_DEBUG_LEVEL > 2
				std::cout << "Lyric: " << data << std::endl;
#endif
				break;
			  case 0x2F: end = true; break;
			  case 0x51:
				if (data.size() != 3) throw std::runtime_error("Invalid tempo change event");
				add_tempo_change(miditime, static_cast<unsigned char>(data[0]) << 16 | static_cast<unsigned char>(data[1]) << 8 | static_cast<unsigned char>(data[2])); break;
			  default:
#if MIDI_DEBUG_LEVEL > 1
				std::cout << "Unhandled meta event  type=" << int(type) << " (" << data.size() << " bytes)" << std::endl;
#endif
				break;
			}
		} else if (event==0xF0 || event==0xF7) {
			// System exclusive event
			uint32_t size = riff.read_varlen();
			riff.ignore(size);
#if MIDI_DEBUG_LEVEL > 1
			std::cout << "System exclusive event ignored (" << size << " bytes)" << std::endl;
#endif
		} else {
			// Midi event
			uint8_t arg1 = riff.read_uint8();
			uint8_t arg2 = riff.read_uint8();
			process_midi_event(track, event >> 4, arg1, arg2, miditime);
		}
	}
	if (miditime > ts_last) ts_last = miditime;
	return track;
}

void MidiFileParser::add_tempo_change(uint32_t miditime, uint32_t tempo) {
	if (tempo == 0) throw std::runtime_error("Invalid MIDI file (tempo is zero)");
	if (tempochanges.empty()) {
		if (miditime > 0) throw std::runtime_error("Invalid MIDI file (tempo not set at the beginning)");
	} else {
		if (tempochanges.back().miditime >= miditime) throw std::runtime_error("Invalid MIDI file (unexpected tempo change)");
	}
#if MIDI_DEBUG_LEVEL > 2
	std::cout << "Tempo change at miditime=" << miditime << ":  " << tempo << " us/QN  " << 6e7 / tempo << " BPM" << std::endl;
#endif
	tempochanges.push_back(TempoChange(miditime, tempo));
}

void MidiFileParser::cout_midi_event(uint8_t t, uint8_t arg1, uint8_t arg2, uint32_t miditime) {
	std::cout << "Midi event:" << std::setw(12) << miditime << std::fixed << std::setprecision(2) << std::setw(12) << get_seconds(miditime) << "  ";
	switch (t) {
	  case 0x8: std::cout << "note off   pitch=" << int(arg1) << " velocity=" << int(arg2); break;
	  case 0x9: std::cout << "note on	pitch=" << int(arg1) << " velocity=" << int(arg2); break;
	  case 0xA: std::cout << "aftertouch pitch=" << int(arg1) << " value=" << int(arg2); break;
	  case 0xB: std::cout << "controller num=" << int(arg1) << " value=" << int(arg2); break;
	  case 0xC: std::cout << "program change num=" << int(arg1); break;
	  case 0xD: std::cout << "channel value =" << int(arg1); break;
	  case 0xE: std::cout << "pitch bend value=" << (arg2 << 8 | arg1); break;
	  default: std::cout << "UNKNOWN EVENT=0x" << std::hex << int(t) << std::dec << ")"; break;
	}
	std::cout << std::endl;
}

uint64_t MidiFileParser::get_us(uint32_t miditime) {
	if (tempochanges.empty()) throw std::runtime_error("Unable to calculate note duration without tempo");
	uint64_t time = 0;
	std::vector<TempoChange>::iterator i = tempochanges.begin();
	// TODO: cache previous
	for (; i + 1 != tempochanges.end() && (i + 1)->miditime < miditime; ++i) {
		time += static_cast<uint64_t>(i->value) * ((i + 1)->miditime - i->miditime);
	}
	time += static_cast<uint64_t>(i->value) * (miditime - i->miditime);
	return time / division;
}

void MidiFileParser::process_midi_event(Track& track, uint8_t t, uint8_t arg1, uint8_t arg2, uint32_t miditime) {
#if MIDI_DEBUG_LEVEL > 3
	cout_midi_event(t, arg1, arg2, miditime);
#endif

	std::vector<Note>& pitch(track.notes[arg1]);
	// common track management
	if (t == 8 || (t == 9 && arg2 == 0)) {
		if (pitch.empty() || pitch.back().end != 0) {
			// Note end event with no corresponding beginning
		} else {
			pitch.back().end = miditime;
		}
	} else {
		pitch.push_back(Note(miditime));
	}
	// special management for lyrics
	if (track.name == "PART VOCALS") {
		// Discard note effects
		if( arg1 < 20 ) return;
		if (t == 8 || (t == 9 && arg2 == 0)) {
			// end of note (note off or note on with zero velocity)
			if( !m_lyric.empty()  ) {
				// here we should update the last note lyric with the current m_lyric
				track.lyrics.back().lyric = m_lyric;
				// here we should update the last note end time with the miditime
				track.lyrics.back().end = miditime;
			} else {
				if( track.lyrics.back().lyric.empty() ) {
					// bad notes, removing it
					track.lyrics.pop_back();
				}
			}
			m_lyric.clear();
		} else {
			// beginning of note then
			// here we should add a lyric with the start time at miditime
			track.lyrics.push_back(LyricNote("", arg1, miditime, miditime));
		}
	}
}


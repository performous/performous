/* This file is part of SilkyStrings 
 * Copyright (C) 2006  Olli Salli, Tuomas Perälä, Ville Virkkala
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "midiFileParser.h"

#include "midiStream.h"
#include <iomanip>
#include <stdexcept>

MidiFileParser::MidiFileParser(std::string name):
  format(0), division(0)
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
    std::cout << "Division: " << division << std::endl;
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
			  case 0x01: std::cout << "Text: " << data << std::endl; break;
			  case 0x03:
				track.name = data;
				std::cout << "Track name: " << data << std::endl;
				break;
			  case 0x2F: end = true; break;
			  case 0x51:
				if (data.size() != 3) throw std::runtime_error("Invalid tempo change event");
				add_tempo_change(miditime, static_cast<unsigned char>(data[0]) << 16 | static_cast<unsigned char>(data[1]) << 8 | static_cast<unsigned char>(data[2])); break;
			  default: std::cout << "Unhandled meta event  type=" << int(type) << " (" << data.size() << " bytes)" << std::endl;
			}
		} else if (event==0xF0 || event==0xF7) {
			// System exclusive event
			uint32_t size = riff.read_varlen();
			riff.ignore(size);
			std::cout << "System exclusive event ignored (" << size << " bytes)" << std::endl;
		} else {
			// Midi event
			uint8_t arg1 = riff.read_uint8();
			uint8_t arg2 = riff.read_uint8();
			process_midi_event(track, event >> 4, arg1, arg2, miditime);
		}
	}
	return track;
}

void MidiFileParser::add_tempo_change(uint32_t miditime, uint32_t tempo) {
	if (tempo == 0) throw std::runtime_error("Invalid MIDI file (tempo is zero)");
	if (tempochanges.empty()) {
		if (miditime > 0) throw std::runtime_error("Invalid MIDI file (tempo not set at the beginning)");
	} else {
		if (tempochanges.back().miditime >= miditime) throw std::runtime_error("Invalid MIDI file (unexpected tempo change)");
	}
	std::cout << "Tempo change at miditime=" << miditime << ":  " << tempo << " us/QN  " << 6e7 / tempo << " BPM" << std::endl;
	tempochanges.push_back(TempoChange(miditime, tempo));
}

void MidiFileParser::cout_midi_event(uint8_t t, uint8_t arg1, uint8_t arg2, uint32_t miditime) {
	std::cout << "Midi event:" << std::setw(12) << miditime << std::fixed << std::setprecision(2) << std::setw(12) << get_seconds(miditime) << "  ";
	switch (t) {
	  case 0x8: std::cout << "note off   pitch=" << int(arg1) << " velocity=" << int(arg2); break;
	  case 0x9: std::cout << "note on    pitch=" << int(arg1) << " velocity=" << int(arg2); break;
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
	// cout_midi_event(t, arg1, arg2, miditime);
	std::vector<Note>& pitch(track.notes[arg1]);
	if (t == 8 || (t == 9 && arg2 == 0)) {
		// Note off event or note on with velocity 0 => note ends
		if (pitch.empty() || pitch.back().end != 0) {
			std::cout << "WARNING: Note end event with no corresponding beginning" << std::endl;
		} else {
			pitch.back().end = miditime;
		}	
	} else if (t == 9) {
		pitch.push_back(Note(miditime));
	}
}

	/*
	if((t==8||t==9) && (int(arg1)>=difficulty && int(arg1)<=(difficulty+4))) {
		int fret=(int)arg1-difficulty+1;
		MidiEvent event(fret, t, getSeconds(miditime));
		midis.push_back(event);
	}
	*/


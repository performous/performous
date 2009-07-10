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

#include <iostream>
#include <map>
#include <vector>
#include "chord.h"

class MidiStream;

/**
 * The Parser class, that contains needed information of given midi-file
 */

class MidiFileParser{

  public:

	/** Constructor
	 *
	 * Creates a MidiFileParser which contains	information of given midifile.
	 *
	 * @param name Name of midifile, which want to be read
	 */
	MidiFileParser(std::string name);

//  private:

	struct TempoChange {
		uint32_t miditime;
		uint32_t value;
		TempoChange(uint32_t miditime, uint32_t value): miditime(miditime), value(value) {}
	};
	std::vector<TempoChange> tempochanges;
	typedef uint8_t Pitch;
	struct Note {
		uint32_t begin;
		uint32_t end;
		Note(uint32_t begin, uint32_t end = 0): begin(begin), end(end) {}
	};
	struct Track {
		std::string name;
		std::map<Pitch, std::vector<Note> > notes;
		Track(std::string const& name = "default"): name(name) {}
	};
	std::vector<Track> tracks;
	uint16_t parse_header(MidiStream&);
	Track read_track(MidiStream&);
	void cout_midi_event(uint8_t type, uint8_t arg1, uint8_t arg2, uint32_t miditime);
	void process_midi_event(Track& track, uint8_t type, uint8_t arg1, uint8_t arg2, uint32_t miditime);
	uint64_t get_us(uint32_t miditime);
	double get_seconds(uint32_t miditime) { return 1e-6 * get_us(miditime); }
	void add_tempo_change(uint32_t miditime, uint32_t tempo);
	uint16_t format;

	/** Ticks per beat == number of divisions per every quarter note **/
	uint16_t division;
};


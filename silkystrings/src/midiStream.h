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

#include <fstream>
#include <string>

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
	MidiStream(std::string file);

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

	std::ifstream f;
	uint16_t read_uint16() { return f.get() << 8 | f.get(); }
	uint32_t read_uint32() { return f.get() << 24 | f.get() << 16 | f.get() << 8 | f.get(); }

};

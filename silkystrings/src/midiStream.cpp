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

#include "midiStream.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>

MidiStream::MidiStream(std::string file): f(file.c_str()) { f.exceptions(std::ios::failbit); }

namespace { bool is_not_alpha(char c) { return (c < 'A' || c > 'Z') && (c < 'a' || c > 'z'); } }

MidiStream::Riff::Riff(MidiStream& ms): ms(ms), name(ms.read_bytes(4)), size(ms.read_uint32()), offset(0) {
	if (std::find_if(name.begin(), name.end(), is_not_alpha) != name.end()) throw std::runtime_error("Invalid RIFF chunk name");
	pos = ms.f.tellg();
}

MidiStream::Riff::~Riff() {
	if (has_more_data()) std::cout << "WARNING: Only " << offset << " of " << size << " bytes read of RIFF chunk " << name << std::endl;
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
		value = value << 7 | c & 0x7F;
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

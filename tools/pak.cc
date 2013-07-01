#include "pak.h"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>

std::ostream& operator<<(std::ostream& os, std::pair<std::string, PakFile> const& f) {
	std::stringstream ss;
	ss << std::setbase(16) << std::setfill('0');
	ss << "0x" << std::setw(8) << f.second.offset << ' ';
	ss << "0x" << std::setw(8) << f.second.offset + f.second.size << ' ';
	ss << f.first << "  " << std::setbase(10);
	if (f.second.zlibmode) ss << f.second.zlibsize << " bytes compressed into "; 
	ss << f.second.size << " bytes";
	return os << ss.rdbuf() << std::endl;
}

std::ostream& operator<<(std::ostream& os, Pak::files_t const& files) {
	std::copy(files.begin(), files.end(), std::ostream_iterator<std::pair<std::string, PakFile> >(os));
	return os;
}

namespace {
	template <unsigned Bytes> unsigned readLE(std::istream& is) {
		unsigned val = 0;
		for (unsigned i = 0; i < Bytes; ++i) val |= (unsigned char)is.get() << i * 8;
		return val;
	}
	template <unsigned Bytes> unsigned readBE(std::istream& is) {
		unsigned val = 0;
		for (unsigned i = 0; i < Bytes; ++i) val = (val << 8) | (unsigned char)is.get();
		return val;
	}
}

Pak::Pak(std::string const& filename) {
	std::ifstream f(filename.c_str(), std::ios::binary);
	if (!f.is_open()) throw std::runtime_error("Could not open PAK file " + filename);
	f.exceptions(std::ios::failbit);
	enum Format { PAK, PAK_WITH_CRC, PKF, PKD } format;
	{
		char magic[8];
		f.read(magic, sizeof(magic));
		if (!std::memcmp(magic, "SceeWhPC", sizeof(magic))) format = PAK;
		else if (!std::memcmp(magic, "SceeWhPk", sizeof(magic))) format = PAK_WITH_CRC;
		else if (!std::memcmp(magic, "PACKAGE ", sizeof(magic))) format = PKD;
		else if (!std::memcmp(magic, "\x7e\x26\x4c\x33\x24\x53\x9b\xd0", sizeof(magic))) format = PKF;
		else throw std::runtime_error("Not a valid PAK/PKF/PKD file (" + std::string(magic) + ")");
	}
	switch (format) {
		case PKF: throw std::runtime_error("SingStar PS3 encrypted pkd format is not yet supported.");
		case PKD:
		{
			// We do not know what these bytes stand for but they always seem to be the same
			if (readBE<4>(f) != 0x01000000 || readBE<2>(f) != 0x0007) throw std::runtime_error("Unexpected header bytes.");
			unsigned headerEnd = readBE<4>(f);
			while (f.tellg() < headerEnd) {
				PakFile file(filename);
				file.crc = readBE<4>(f);  // Random digits (maybe CRC32)
				std::string name;
				std::getline(f, name, '\0');
				file.offset = readBE<4>(f);
				file.size = readBE<4>(f);
				std::replace(name.begin(), name.end(), '\\', '/');
				std::size_t pos = f.tellg();
				if (file.size > 12) {
					f.seekg(file.offset);
					char magic[4];
					f.read(magic, sizeof(magic));
					if (!std::memcmp(magic, "ZLIB", sizeof(magic))) {
						file.zlibmode = readBE<4>(f);
						file.zlibsize = readBE<4>(f);
						file.offset += 12;
						file.size -= 12;
					}
					f.seekg(pos);
				}
				m_files.insert(std::make_pair(name, file));
			}
			break;
		}
		case PAK: case PAK_WITH_CRC:
		{
			f.seekg(0x114);
			std::vector<std::string> ext;
			while(1) {
				char tmp[4];
				f.read(tmp,4);
				if(tmp[0] == '\0')
					break;
				ext.push_back(std::string(tmp));
			}
			f.seekg(0x198);
			std::string name;
			PakFile file(filename);
			while ((file.offset = readLE<3>(f)) > 0) {
				name = name.substr(0, readLE<1>(f));  // Previous filename used as template
				file.offset *= 0x800;
				char string_length = readLE<1>(f);
				f.ignore(2);
				file.size = readLE<4>(f);
				if (format == PAK_WITH_CRC) file.crc = readLE<4>(f);
				char tmp[256];
				f.read(tmp, string_length - 1);
				name += std::string(tmp, string_length - 1);
				unsigned ext_idx = readLE<1>(f);
				char toto[2];
				toto[0] = '0' + ext.size();
				toto[1] = '\0';
				if (ext_idx) name += std::string(".") + (ext_idx <= ext.size() ? ext[ext_idx-1] : std::string(toto));
				std::replace(name.begin(), name.end(), '\\', '/');
				m_files.insert(std::make_pair(name, file));
			}
			break;
		}
	}
}

PakFile const& Pak::operator[](std::string const& filename) const {
	files_t::const_iterator r = m_files.find(filename);
	if (r == m_files.end()) throw std::runtime_error("File not found: " + filename);
	return r->second;
}


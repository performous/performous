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
	ss << "0x" << std::setw(8) << f.second.size << ' ';
	return os << ss.rdbuf() << f.first << std::endl;
}

std::ostream& operator<<(std::ostream& os, Pak::files_t const& files) {
	std::copy(files.begin(), files.end(), std::ostream_iterator<std::pair<std::string, PakFile> >(os));
	return os;
}

namespace {
	template <unsigned Bytes> unsigned readLE(std::istream& is) {
		unsigned val = 0;
		for (unsigned i = 0; i < Bytes; ++i) val |= is.get() << i * 8;
		return val;
	}
}

Pak::Pak(std::string const& filename) {
	std::ifstream f(filename.c_str(), std::ios::binary);
	if (!f.is_open()) throw std::runtime_error("Could not open PAK file");
	f.exceptions(std::ios::failbit);
	bool enable_crc;
	{
		char magic[8];
		f.read(magic, sizeof(magic));
		if (!std::memcmp(magic, "SceeWhPC", sizeof(magic))) enable_crc = true;
		else if (!std::memcmp(magic, "SceeWhPk", sizeof(magic))) enable_crc = false;
		else throw std::runtime_error("Not a valid PAK file (" + std::string(magic) + ")");
	}
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
		if (enable_crc) file.crc = readLE<4>(f);
		char tmp[256];
		f.read(tmp, string_length - 1);
		name += std::string(tmp, string_length - 1);
		unsigned ext_idx = readLE<1>(f);
		char toto[2];
		toto[1] = '0' + ext.size();
		toto[2] = '\0';
		if (ext_idx) name += std::string(".") + (ext_idx <= ext.size() ? ext[ext_idx-1] : std::string(toto));
		std::replace(name.begin(), name.end(), '\\', '/');
		m_files.insert(std::make_pair(name, file));
	}
}

PakFile const& Pak::operator[](std::string const& filename) const {
	files_t::const_iterator r = m_files.find(filename);
	if (r == m_files.end()) throw std::runtime_error("File not found: " + filename);
	return r->second;
}


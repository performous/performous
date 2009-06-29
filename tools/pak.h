#pragma once

#include <boost/cstdint.hpp>
#include <string>
#include <fstream>
#include <map>
#include <vector>
#include <stdexcept>

struct PakFile {
	PakFile(std::string const& pakfilename): pakname(pakfilename), offset(), size(), crc() {}
	std::string pakname;
	unsigned int offset;
	unsigned int size;
	unsigned int crc;
	void get(std::vector<char>& buf, unsigned int pos = 0, unsigned int s = 0) const {
		if (!s) s = size - pos;
		if (pos + s > size) throw std::logic_error("Trying to read past end of file");
		buf.resize(s);
		std::ifstream f(pakname.c_str(), std::ios::binary);
		f.seekg(offset + pos);
		f.read(&buf[0], s);
	}
};

class Pak {
  public:
	typedef std::map<std::string, PakFile> files_t;
	Pak(std::string const& filename);
	files_t const& files() const { return m_files; }
	PakFile const& operator[](std::string const& filename) const;
  private:
	files_t m_files;
};

std::ostream& operator<<(std::ostream& os, PakFile const& f);
std::ostream& operator<<(std::ostream& os, Pak::files_t const& files);


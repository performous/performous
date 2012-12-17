#pragma once

#include "zlib.h"
#include <boost/cstdint.hpp>
#include <string>
#include <fstream>
#include <map>
#include <vector>
#include <stdexcept>

struct PakFile {
	PakFile(std::string const& pakfilename): pakname(pakfilename), offset(), size(), crc(), zlibmode(), zlibsize() {}
	std::string pakname;
	unsigned offset;
	unsigned size;
	unsigned crc;
	unsigned zlibmode;
	unsigned zlibsize;
	void get(std::vector<char>& buf, unsigned int pos = 0, unsigned int s = 0) const {
		if (!s) s = size - pos;
		if (pos + s > size) throw std::logic_error("Trying to read past end of file");
		buf.resize(s);
		std::ifstream f(pakname.c_str(), std::ios::binary);
		f.seekg(offset + pos);
		f.read(&buf[0], s);
		if (zlibmode) {
			if (pos != 0 || s != size) throw std::logic_error("Cannot seek in zlib deflated files");
			std::vector<char> buf2(zlibsize);
			z_stream strm = z_stream();
			strm.avail_in = buf.size(); strm.next_in = reinterpret_cast<Bytef*>(&buf[0]);
			strm.avail_out = buf2.size(); strm.next_out = reinterpret_cast<Bytef*>(&buf2[0]);
			inflateInit(&strm);
			int ret = inflate(&strm, Z_SYNC_FLUSH);
			inflateEnd(&strm);
			if (ret == Z_STREAM_END) buf2.swap(buf);
			else if (strm.msg) throw std::runtime_error(std::string("Zlib inflate failed: ") + strm.msg);
		}
		// TODO: Unduplicate code with itg_pck! (e.g. use PakFile structure there as well)
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


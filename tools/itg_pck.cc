#include <boost/filesystem.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

template <typename It> void test(It it, std::string text) {
	if (!std::equal(text.begin(), text.end(), it)) throw std::runtime_error("PCKF header missing");
}

template <typename It> std::string read(It it, unsigned chars) {
	std::string ret;
	while (chars--) ret += *it++;
	return ret.substr(0, ret.find_last_not_of('\0') + 1);
}

template <typename It> unsigned read32(It it) {
	unsigned ret = 0;
	for (unsigned i = 0; i < 4; ++i) ret |= static_cast<unsigned char>(*it++) << i * 8;
	return ret;
}

namespace fs = boost::filesystem;

struct File {
	fs::path name;
	unsigned size;
	unsigned sizeCompressed;
	unsigned mode; // 0 = stored, 1 = compressed
	unsigned offset;
};

#include <zlib.h>

struct Extract {
	std::ifstream& archive;
	Extract(std::ifstream& archive): archive(archive) {}
	void operator()(File const& file) {
		std::cout << "  " << file.name << " " << file.size << " at " << file.offset << std::endl;
		if (file.mode == 1) std::cout << "    " << file.size << "->" << file.sizeCompressed << std::endl;
		std::vector<char> buffer(file.sizeCompressed);
		archive.seekg(file.offset);
		archive.read(&buffer[0], buffer.size());
		std::string ext;
		if (file.mode == 1) {
			std::vector<char> buf2(file.size);
			z_stream strm = {};
			strm.avail_in = buffer.size();
			strm.next_in = reinterpret_cast<Bytef*>(&buffer[0]);
			strm.avail_out = buf2.size();
			strm.next_out = reinterpret_cast<Bytef*>(&buf2[0]);
			
			inflateInit2(&strm, -15);
			int ret = inflate(&strm, Z_SYNC_FLUSH);
			inflateEnd(&strm);
			std::cout << ret << std::endl;
			if (ret == Z_STREAM_END) buf2.swap(buffer);
			else {
				std::ostringstream oss;
				oss << ".comp-" << file.size;
				ext = oss.str();
			}
			if (strm.msg) std::cerr << "    Error: " << strm.msg << std::endl;
		}
		// Make directory
		makeDir(file.name);
		std::ofstream of((file.name.string() + ext).c_str(), std::ios::binary);
		of.write(&buffer[0], buffer.size());
	}
	void makeDir(fs::path p) {
		p.remove_filename();
		if (p.empty()) return;
		makeDir(p);
		fs::create_directory(p);
	}
};

int main(int argc, char** argv) {
	if (argc != 2) throw std::logic_error("Need pck filename as argument");
	std::ifstream f(argv[1], std::ios::binary);
	f.exceptions(std::ios::failbit);
	std::istreambuf_iterator<char> it(f), end;
	// PCKF header:
	// 17 bytes "PCKFdefault base "
	// Name (title) of the package, padded with zeroes until offset 0x84
	// Little-endian u32 file count
	test(it, "PCKFdefault base ");
	std::string title = read(it, 0x73);
	std::cout << "Title: " << title << std::endl;
	unsigned filecount = read32(it);
	std::cout << filecount << " files:" << std::endl;
	std::vector<File> files;
	while (files.size() < filecount) {
		// File entry: (array of filecount file entries)
		// Little-endian u32: size of compressed/stored data
		// Little-endian u32: size of uncompressed data
		// Little-endian u32: position in pck file (in bytes)
		// Little-endian u32: name length (in bytes)
		// char[namelen]: file name with path, uses '/' as path separator
		// Little-endian u32: compression mode (0 = stored, 1 = deflated)
		File file;
		file.sizeCompressed = read32(it);
		file.size = read32(it);
		file.offset = read32(it);
		unsigned namelen = read32(it);
		file.mode = read32(it);
		file.name = fs::path(title) / read(it, namelen);
		files.push_back(file);
	}
	std::cout << "Header ends at " << f.tellg() << std::endl;
	std::for_each(files.begin(), files.end(), Extract(f));
}


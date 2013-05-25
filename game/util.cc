#include "util.hh"

#include <fstream>
#include <stdexcept>

BinaryBuffer readFile(fs::path const& path) {
	BinaryBuffer ret;
	std::ifstream f(path.c_str(), std::ios::binary);
	f.seekg(0, std::ios::end);
	ret.resize(f.tellg());
	f.seekg(0);
	f.read(reinterpret_cast<char*>(ret.data()), ret.size());
	if (!f) throw std::runtime_error("File cannot be read: " + path.string());
	return ret;
}


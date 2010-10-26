// @file Tool for extracting 'Disney Sing It' archive/archive.log files

#include <boost/filesystem.hpp>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {
	void usage(char const* progname) {
		std::cerr << "Usage: " << progname << " archive --extract [files]" << std::endl;
		std::cerr << "       " << progname << " archive --dump file" << std::endl;
		std::cerr << "       " << progname << " archive --list" << std::endl;
	}

	struct File {
		size_t offset;
		size_t size;
	};

	typedef std::map<std::string, File> Files;

	Files readFiles(std::string archive) {
		Files files;
		archive += ".log";
		std::ifstream archlog(archive.c_str(), std::ios::binary);
		File f = { 4, 0 };
		for (std::string name; archlog >> name >> f.size; f.offset += f.size) files[name] = f;
		if (!archlog.eof()) throw std::runtime_error("Error reading " + archive);
		return files;
	}

	void extract(std::ifstream& arch, Files::const_iterator it, std::ostream& output) {
		File const& f = it->second;
		std::vector<char> buf(f.size);
		arch.seekg(f.offset);
		arch.read(&buf[0], buf.size());
		output.write(&buf[0], buf.size());
	}

	struct Extract {
		Extract(std::ifstream& arch, Files const& files): m_arch(arch), m_files(files) {}
		/// Extract one file
		void operator()(std::string const& filename) {
			Files::const_iterator it = m_files.find(filename);
			if (it == m_files.end()) throw std::runtime_error("File not found in archive");
			operator()(it);
		}
		/// Extract all files
		void operator()() {
			for (Files::const_iterator it = m_files.begin(); it != m_files.end(); ++it) operator()(it);
		}
		/// Extract one file by iterator
		void operator()(Files::const_iterator it) {
			std::string filename = it->first;
			// Remove path elements from m_path until it matches the filename's beginning
			while (m_path != filename.substr(0, m_path.size())) {
				std::string::size_type pos = m_path.rfind('/');
				if (pos == std::string::npos) m_path.clear();
				else m_path.erase(pos);
			}
			// Try to create new folders as required
			for (std::string::size_type pos; (pos = filename.find('/', m_path.size() + 1)) != std::string::npos;) {
				m_path = filename.substr(0, pos);
				boost::filesystem::create_directory(m_path);
			}
			// Extract the file
			std::ofstream f(filename.c_str(), std::ios::binary);
			if (!f.is_open()) throw std::runtime_error("Unable to create file: " + filename);
			std::cout << filename << std::flush;
			extract(m_arch, it, f);
			std::cout << std::endl;
		}
	  private:
	  	std::ifstream& m_arch;
	  	Files const& m_files;
		std::string m_path;
	};

	std::ostream& operator<<(std::ostream& os, Files const& files) {
		std::stringstream ss;
		ss << std::setbase(16) << std::setfill('0');
		for (Files::const_iterator it = files.begin(); it != files.end(); ++it) {
			File const& f = it->second;
			ss << "0x" << std::setw(8) << f.offset << ' ';
			ss << "0x" << std::setw(8) << f.size << ' ';
			ss << it->first << std::endl;
		}
		return os << ss.rdbuf() << std::flush;
	}
}

int main(int argc, char** argv) {
	std::ios::sync_with_stdio(false);
	if( argc < 3 ) { usage(argv[0]); return EXIT_FAILURE; }
	try {
		std::ifstream arch(argv[1], std::ios::binary);
		if (!arch) throw std::runtime_error("Unable to open " + std::string(argv[1]));
		Files const files = readFiles(argv[1]);
		if (files.empty()) throw std::runtime_error("No files found in archive");
		if (!strcmp(argv[2],"--list")) std::cout << files;
		else if (!strcmp(argv[2],"--dump")) {
			if (argc != 4) { usage(argv[0]); return EXIT_FAILURE; }
			Files::const_iterator it = files.find(argv[3]);
			if (it == files.end()) throw std::runtime_error("File not found in archive");
			extract(arch, it, std::cout);
		} else if (!strcmp(argv[2],"--extract")) {
			Extract extractor(arch, files);
			if (argc == 3) extractor();
			else std::for_each(argv + 3, argv + argc, extractor);
		} else { usage(argv[0]); return EXIT_FAILURE; }
	} catch (std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}


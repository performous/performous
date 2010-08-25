#pragma once

#include "fs.hh"

#include <fstream>

#include <boost/cstdint.hpp>

/** Helper functions that determinate file type **/
namespace filemagic {
	/*
	 * All internal helper functions start with lowercase names.
	 * All functions with CAPS only names are file type identification
	 * functions, they take the full path/file name of the file as argument.
	 *
	 * In general if a file type has a known "magic number" that's the most
	 * accurate test. Testing by extension alone places a lot of trust in
	 * users. (note: don't worry about reading the file a few times, the OS
	 * will most likely cache the whole file in RAM, even if we only read
	 * the first few bytes. Ideally the OS will "read-ahead" the whole file
	 * just in time for the actual processing of it, while we were busy
	 * determining how we should process it)
	 */

	/** Compare the first prefix_size bytes of the file file name with an expected magic number **/
	template <size_t prefix_size>
	bool signaturePrefix(fs::path const& filename, boost::uint8_t const (& bytes)[prefix_size], const std::streamoff offset=0) {
		uint8_t buf[prefix_size];
		std::ifstream file /*(filename.string().c_str(), std::ios::binary)*/;
		// note: explicit open needed to get an exception on error since we can't set the exception mask in the ctor.
		file.exceptions( std::ifstream::eofbit | std::ifstream::failbit | std::ifstream::badbit );

		// since C++ doesn't specify the bit size of it's data types we must
		// make the following assertion
		//   sizeof(char) = 1*n = sizeof(int8_t)*n for integer n >= 1
		// (N.B: char is almost always exactly 1 (or 2) byte, but
		// theoretically it could be an arbitrary number of bits!)
		try {
			file.open(filename.string().c_str(), std::ifstream::in | std::ifstream::binary);
			file.seekg(offset * sizeof(char)/sizeof(uint8_t));
			file.read(reinterpret_cast<char *>(&buf), prefix_size * sizeof(char)/sizeof(uint8_t) );
			file.close();
		} catch( ... ) {
			// any error here makes it impossible for us to continue, thus, we won't find the prefix we seek.
			return false;
		}

#		ifdef DEBUG_FILEMAGIC_PREFIX
		std::clog << "filemagic/debug: prefix check: want: " << std::hex;
		for(unsigned int i=0; i < prefix_size; i++)
			std::clog << (unsigned int)(bytes[i]) << ",";
		std::clog << std::endl << "filemagic/debug: prefix check:  got: ";
#		endif
		for(unsigned int i=offset; i < prefix_size; i++){
#			ifdef DEBUG_FILEMAGIC_PREFIX
			std::clog << (unsigned int)(buf[i]) << ",";
#			endif
			if(buf[i] != bytes[i]) return false;
		}
#		ifdef DEBUG_FILEMAGIC_PREFIX
		std::clog::endl << "filemagic/debug: prefix check: match!" << std::endl;
#		endif
		return true;
	}

	/** Checks if the file is an SVG **/
	inline bool SVG(fs::path const& filename) {
		// The SVG format doesn't specify any magic number prefix. I think we could
		// partially parse the file, i.e. skip over any legal white-space tokens and
		// expect a '<' token followed by one of [a-zA-Z!?].
		// For now, just check the extension an assume it's not lying.

		// Get file extension in lower case
		std::string ext = filename.extension();
		// somehow this does not convert the extension to lower case:
		//std::for_each(ext.begin(), ext.end(), static_cast<int(*)(int)>(std::tolower));
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower );

		return ext == ".svg" /* || signaturePrefix<1>(filename, {'<') */;
	}

	/** Checks if the file is an PNG **/
	inline bool PNG(fs::path const& filename) {
		// prefix as describe at <http://www.garykessler.net/library/file_sigs.html>
		const uint8_t magic[8]={0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
		return /*ext == ".png" ||*/ signaturePrefix<8>(filename, magic);
	}

	/** Checks if the file is an PNG **/
	inline bool JPEG(fs::path const& filename) {
		// <FF, D8, FF> seems to be the longest common prefix for all JPEG types listed at:
		// <http://www.garykessler.net/library/file_sigs.html>
		// But wikipedia cites <FF, D8> (actually, looking closer the prefix is APPn which
		// is <FF, Ex> where x appears to be "variable length").
		const uint8_t magic[3] = {0xFF, 0xD8, 0xFF};
		return /*ext == ".jpg" || ext == ".jpeg" || ext == ".jpe" || ext == ".jfif" ||*/ signaturePrefix<3>(filename, magic);
	}

}

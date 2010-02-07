#include <boost/crc.hpp>
#pragma once

#include <string>
#include <stdexcept>
#include <zlib.h>

class ChcDecode {
  public:
	ChcDecode() {
		for( unsigned int i = 0 ; i < 4 ; i++ ) {
			key_crc[i] = 0;
		}
	}
	// TODO: move buffer uncypher inside load instead of getMelody
	void load(std::string key[4]) {
		for( unsigned int i = 0 ; i < 4 ; i++ ) {
			boost::crc_32_type crc;
			crc.process_bytes(key[i].c_str(), key[i].size()+1);
			key_crc[i] = crc.checksum();
		}
	}
	std::string getMelody(char *buffer, unsigned int buffer_size, unsigned int id) {
		unsigned int *chc_buffer = (unsigned int*)buffer;

		if( buffer_size%8 != 0 ) throw std::runtime_error("CHC file is not 8 bytes padded");

		for(unsigned int i = 0 ; i < buffer_size/8 ; i++) {
			decrypt(&chc_buffer[i*2], key_crc);
		}

		unsigned int songs = chc_buffer[0];

		if( songs > 100 ) throw std::runtime_error("CHC key probably wrong (too many songs)");

		unsigned int start = 0;
		uLongf packsize = 0;
		uLong size = 0;
		for( unsigned int i = 0 ; i < songs ; i++) {
			if( chc_buffer[1+i*4] == id ) {
				start = chc_buffer[2+i*4];
				packsize = chc_buffer[3+i*4];
				size = chc_buffer[4+i*4];
				break;
			}
		}
		if( packsize == 0 ) throw std::runtime_error("Melody not found in CHC file");

		Bytef *dest = new Bytef[size];
		uncompress(dest, &size, (Bytef*)buffer+start, packsize);
		std::string result((char*)dest, (unsigned int)size);
		delete[] dest;

		return result;
	}
  private:
	unsigned int key_crc[4];
	void decrypt(unsigned int *v, unsigned int k[4]) {
		unsigned int v0 = v[0], v1 = v[1], i;
		unsigned int sum   = 0xC6EF3720;
		unsigned int delta = 0x9e3779b9;

		for(i = 0; i<32; i++) { // basic cycle start
			v1 -= (v0<<4) + (k[2] ^ v0) + (sum ^ (v0>>5)) + k[3];
			v0 -= (v1<<4) + (k[0] ^ v1) + (sum ^ (v1>>5)) + k[1];
			sum -= delta;
		} // end cycle
		v[0] = v0; v[1] = v1;
	}
};

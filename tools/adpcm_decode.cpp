#include "adpcm.h"
#include "pak.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>

unsigned short decode_channels = 2;

void process(Adpcm& adpcm, std::vector<char> const& data, std::ostream& outfile) {
	std::vector<short> pcm(adpcm.chunkFrames() * decode_channels);
	adpcm.decodeChunk(&data[0], &pcm[0]);
	outfile.write(reinterpret_cast<char*>(&pcm[0]), pcm.size() * sizeof(short));
}

void writeWavHeader(std::ostream& outfile, unsigned ch, unsigned sr, unsigned samples) {
	unsigned bps = ch * 2; // Bytes per sample
	unsigned datasize = bps * samples;
	unsigned size = datasize + 0x2C;
	outfile.write("RIFF" ,4); // RIFF chunk
	{ unsigned int tmp=size-0x8 ; outfile.write((char*)(&tmp),4); } // RIFF chunk size
	outfile.write("WAVEfmt ",8); // WAVEfmt header
	{ int   tmp=0x00000010 ; outfile.write((char*)(&tmp),4); } // Always 0x10
	{ short tmp=0x0001     ; outfile.write((char*)(&tmp),2); } // Always 1
	{ short tmp = ch; outfile.write((char*)(&tmp),2); } // Number of channels
	{ int   tmp = sr; outfile.write((char*)(&tmp),4); } // Sample rate
	{ int   tmp = bps * sr; outfile.write((char*)(&tmp),4); } // Bytes per second
	{ short tmp = bps; outfile.write((char*)(&tmp),2); } // Bytes per frame
	{ short tmp = 16; outfile.write((char*)(&tmp),2); } // Bits per sample
	outfile.write("data",4); // data chunk
	{ int   tmp = datasize; outfile.write((char*)(&tmp),4); }
}

int main(int argc, char** argv) {
	std::string in, out, pak;

	if( argc == 3 ) {
		in = argv[1];
		out = argv[2];
	} else if( argc == 4 ) {
		pak = argv[1];
		in = argv[2];
		out = argv[3];
	} else {
		std::cout << "Usage: " << argv[0] << " [archive.pak] input.mib output.wav" << std::endl;
		return EXIT_FAILURE;
	}
	// FIXME: read from music.mih
	const unsigned sr = 48000;
	const unsigned interleave = 0xB800;
	Adpcm adpcm(interleave, decode_channels);
	std::ofstream outf;
	if (out != "-") outf.open(out.c_str(), std::ios::binary);
	std::ostream& outfile = (out != "-" ? outf : std::cout);
	std::vector<char> data(adpcm.chunkBytes());
	if (pak.empty()) {
		std::ifstream infile(in.c_str(), std::ios::binary);
		writeWavHeader(outfile, 2, sr, sr * 1000 /* FIXME: calculate real length */);
		while (infile.read(&data[0], adpcm.chunkBytes()) && infile.seekg(adpcm.chunkBytes(), std::ios::cur)) {
			process(adpcm, data, outfile);
		}
	} else {
		Pak p(pak);
		PakFile const& infile(p[in]);
		writeWavHeader(outfile, 2, sr, infile.size / (adpcm.chunkBytes() * 2) * adpcm.chunkFrames());
		for (unsigned pos = 0, end; (end = pos + 2 * adpcm.chunkBytes()) <= infile.size; pos = end) {
			infile.get(data, pos, end - pos);
			process(adpcm, data, outfile);
		}
	}
}


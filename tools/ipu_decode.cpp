#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

#define FLAG_IDP         3
#define FLAG_DTD         4
#define FLAG_AS         16
#define FLAG_IVF        32
#define FLAG_QST        64
#define FLAG_MPEG_TYPE 128

#define FPS             25

struct IPUHeader {
	unsigned int file_size;
	unsigned short width;
	unsigned short height;
	unsigned int frames;
};

struct IPUFrame {
	unsigned int id;
	char mpeg1;
	char qst;
	char ivf;
	char as;
	char dtd;
	char idp;
};

int intSwitch( int in ) {
	int result=0;
	result |= (0x000000ff&in)<<24;
	result |= (0x0000ff00&in)<<8;
	result |= (0x00ff0000&in)>>8;
	result |= (0xff000000&in)>>24;
	return result;
}

void writeMpegHeader( std::ofstream &mpegfile, char mpeg1, short width, short height ) {
	unsigned int header;
	header = 0xB3010000;
	mpegfile.write((char*)&header,sizeof(int));
	header = intSwitch(width * (1 << 20) + height * (1 << 8) + (1 << 4) + 3);
	mpegfile.write((char*)&header,sizeof(int));
	header = 0x00E1CE01;
	mpegfile.write((char*)&header,sizeof(int));
	if( !mpeg1 ) {
		short tmp=0x0000;
		header = 0xb5010000;
		mpegfile.write((char*)&header,sizeof(int));
		header = 0x01008A14;
		mpegfile.write((char*)&header,sizeof(int));
		mpegfile.write((char*)&tmp,sizeof(short));
		header = 0xb5010000;
		mpegfile.write((char*)&header,sizeof(int));
		header = 0x04050523;
		mpegfile.write((char*)&header,sizeof(int));
		header = intSwitch(width * (1<<18) + (1<<17) + height * (1<<3));
		mpegfile.write((char*)&header,sizeof(int));
	}
}

void writeGopHeader(std::ofstream& mpegfile, unsigned int frame, char mpeg1, char idp, char qst, char ivf, char as) {
	unsigned int header;
	int hours   = frame / FPS / 60 / 60;
	int minutes = (frame % (FPS * 60 * 60)) / FPS / 60;
	int seconds = (frame % (FPS * 60)) / FPS;
	int ffs     = frame % FPS;

	header = 0xb8010000;
	mpegfile.write((char*)&header,sizeof(int));
	header = intSwitch(hours * (1<<26) + minutes * (1<<20) + (1<<19) + seconds * (1<<13) + ffs * (1<<7) + (1<<6));
	mpegfile.write((char*)&header,sizeof(int));
	header = 0x00010000;
	mpegfile.write((char*)&header,sizeof(int));
	header = 0xf8ff0f00;
	mpegfile.write((char*)&header,sizeof(int));
	if( !mpeg1 ) {
		char tmp = 0x80;
		header = 0xb5010000;
		mpegfile.write((char*)&header,sizeof(int));
		header = intSwitch(0x8ffff000 + (idp*(1<<10)+3*(1<<8)+(1<<6)+1+qst+ivf+as));
		mpegfile.write((char*)&header,sizeof(int));
		mpegfile.write(&tmp,sizeof(char));
	}
	// Slice Header
	short tmp = 0x0000;
	mpegfile.write((char*)&tmp,sizeof(short));
	header = 0x010C0101;
	mpegfile.write((char*)&header,sizeof(int));
}

void writeGopFooter( std::ofstream &mpegfile ) {
	unsigned int header = 0x00000000;
	char tmp = 0x00;
	mpegfile.write((char*)&header,sizeof(int));
	mpegfile.write((char*)&header,sizeof(int));
	mpegfile.write((char*)&tmp,sizeof(char));
}

int main(int argc, char** argv) {
	if (argc < 3) {
		std::cout << "Usage: " << argv[0] << " input output" << std::endl;
		return 1;
	}

	std::ifstream ipufile(argv[1], std::ios::binary);
	std::ofstream mpegfile(argv[2], std::ios::binary);
	if (!ipufile.is_open()) throw std::runtime_error("Could not open IPU file");

	// Read IPU header
	IPUHeader ipu;
	ipufile.seekg(0x4, std::ios::cur);
	ipufile.read((char*)&ipu, sizeof(ipu));
	ipu.file_size += 8; // Add header size (ipum and filesize)

	std::cout << "FileSize: " << ipu.file_size << std::endl;
	std::cout << "Geometry: " << ipu.width << "x" << ipu.height << std::endl;
	std::cout << "Frames:   " << ipu.frames << std::endl;

	for (unsigned int i = 0; i < ipu.frames; ++i) {
		std::vector<unsigned char> buffer_in, buffer_out;
		char flags = ipufile.get();

		IPUFrame frame;
		frame.id    = i;
		frame.mpeg1 = flags&FLAG_MPEG_TYPE;
		frame.qst   = flags&FLAG_MPEG_TYPE;
		frame.ivf   = flags&FLAG_IVF;
		frame.as    = flags&FLAG_AS;
		frame.dtd   = flags&FLAG_DTD;
		frame.idp   = flags&FLAG_IDP;

		if (frame.id == 0) writeMpegHeader(mpegfile, frame.mpeg1, ipu.width, ipu.height);

		writeGopHeader(mpegfile, frame.id, frame.mpeg1,frame.idp,frame.qst,frame.ivf,frame.as);
		
		if (frame.ivf) std::cerr << "Intra VLC Format not supported" << std::endl;

		for (unsigned j = 0; j < 4; ++j) buffer_in.push_back(ipufile.get());

		for (unsigned j = 3;; ++j) {
			unsigned char ch;
			if (buffer_in[j] == 0xb0 &&
			  buffer_in[j-1] == 0x01 &&
			  buffer_in[j-2] == 0x00 &&
			  buffer_in[j-3] == 0x00)
			  break;
			ipufile.read((char*)&ch,sizeof(char));
			buffer_in.push_back(ch);
		}

		//std::cout << frame.id << ": " << buffer_in.size() << std::endl;
		// FIXME: No implementation seemsto be written for this (and no data is ever output):
		//convertFrame(buffer_in,buffer_out,ipu,frame);
		// mpegfile.write(buffer_out); // pseudo-code

		writeGopFooter(mpegfile);
	}
}


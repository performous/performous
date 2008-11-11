#include "ipuconv.hh"
#include <cstdio>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
	if (argc<3){
		std::printf("\nConverts an Singstar IPU-movie into an MPEG-Video\n20080103 - hawkear@gmx.de\n\n"
			"Usage:     %s <INFILE> <OUTFILE>\n\n"
			"Example:   %s movie.ipu myvideo.m2v\n\n",argv[0],argv[0]);
		exit(0);
	}
	std::vector<char> indata;
	std::ifstream infile(argv[1], std::ios::binary);
	infile.seekg(0, std::ios::end);
	indata.resize(infile.tellg());
	infile.seekg(0);
	infile.read(&indata[0], indata.size());
	try {
		IPUConv(indata, argv[2]);
	} catch (std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
}

